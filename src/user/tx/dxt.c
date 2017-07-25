/*
 * dxt.c
 * 	Implementation of Data Exchange Transfer Module
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>

#include "util_nw.h"
#include "nid_shared.h"
#include "tx_shared.h"
#include "nid_log.h"
#include "lstn_if.h"
#include "txn_if.h"
#include "ds_if.h"
#include "pp2_if.h"
#include "dxt_if.h"
#include "dxa_if.h"
#include "dxp_if.h"

struct dxt_private {
	pthread_mutex_t		p_lck;
	pthread_mutex_t		p_in_mlck;
	pthread_mutex_t		p_out_mlck;
	pthread_mutex_t		p_wait_ack_mlck;
	pthread_cond_t		p_out_cond;
	struct lstn_interface	*p_lstn;
	struct txn_interface	*p_txn;
	struct ds_interface	*p_ds;
	struct pp2_interface	*p_pp2;
	struct list_head	p_in_head;
	struct list_head	p_out_head;
	struct list_head	p_out_data_head;
	struct list_head	p_wait_ack_head;
	struct list_head	p_wait_ack_data_head;
	struct dxa_interface	*p_dxa;
	struct dxp_interface	*p_dxp;
	int			p_csfd;
	int			p_dsfd;
	int			p_in_ds;
	int			p_in_data_ds;
	uint32_t		p_req_size;
	int			p_freeze;
	char			p_dxt_name[NID_MAX_UUID];
	uint8_t			p_is_working;
	uint8_t			p_to_stop;
	uint8_t			p_send_stop;		// 5 threads stop flags
	uint8_t			p_send_data_stop;
	uint8_t			p_recv_data_stop;
	uint8_t			p_recv_ctrl_stop;
	uint8_t			p_send_ack_stop;
	uint8_t			p_send_frozen;		// 5 threads freezed flags
	uint8_t			p_send_data_frozen;
	uint8_t			p_recv_data_frozen;
	uint8_t			p_recv_ctrl_frozen;
	uint8_t			p_send_ack_frozen;
	void			*p_handle;
	nid_tx_callback	p_dxt_callback;
	uint64_t		p_dsend_seq;	// max seq of send data message
	uint64_t		p_send_seq;	// max seq of send control message
	uint64_t		p_ack_seq;	// max seq of ack message send out
	uint64_t		p_recv_seq;	// max seq of received control message
	uint64_t		p_recv_dseq;	// max dseq received data message
	uint32_t		p_cmsg_left;	// un-processed bytes in control message receive buffer
	char			*p_cmsg_pos;		// cur pos in receive control message stream
};

static int
__dxt_no_thread(struct dxt_interface *dxt_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	return (priv_p->p_send_ack_stop && priv_p->p_send_data_stop &&
			priv_p->p_send_stop && priv_p->p_recv_data_stop &&
			priv_p->p_recv_ctrl_stop);
}

static int
dxt_all_frozen(struct dxt_interface *dxt_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	int rc;

	rc = (priv_p->p_send_ack_frozen && priv_p->p_send_data_frozen &&
			priv_p->p_send_frozen && priv_p->p_recv_data_frozen &&
			priv_p->p_recv_ctrl_frozen);

	/*
	 * if not all threads frozen, it may means send message threads fall in wait_cond
	 * wake it up
	 */
	if(!rc) {
		pthread_mutex_lock(&priv_p->p_out_mlck);
		pthread_cond_broadcast(&priv_p->p_out_cond);
		pthread_mutex_unlock(&priv_p->p_out_mlck);
	}

	return rc;
}

static void
__dxt_trigger_recover(struct dxt_interface *dxt_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	pthread_mutex_lock(&priv_p->p_lck);
	if(priv_p->p_freeze) {
		pthread_mutex_unlock(&priv_p->p_lck);
		return;
	}
	priv_p->p_freeze = 1;
	priv_p->p_is_working = 0;
	pthread_mutex_unlock(&priv_p->p_lck);

	if(priv_p->p_dxa) {
		// when dxt used by dxa module, it cannot destroy, unless dxa destroy it
		struct dxa_interface *dxa_p = priv_p->p_dxa;

		// notify dxa recovery connections and pause dxt send/receive action
		dxa_p->dx_op->dx_mark_recover_dxt(dxa_p);
	} else if(priv_p->p_dxp) {
		struct dxp_interface *dxp_p = priv_p->p_dxp;

		// pause dxt receive action
		dxp_p->dx_op->dx_mark_recover_dxt(dxp_p);
	}
}

#ifdef SEQ_ACK_ENABLE
static int
__dxt_dnode_exist(struct dxt_interface *dxt_p, struct tx_node *tx_dnp)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct tx_node *txnp;

	list_for_each_entry(txnp, struct tx_node, &priv_p->p_wait_ack_data_head, tn_list) {
		if(txnp == tx_dnp) {
			// tx_dnp in wait ack data list, it means data has been send out
			return 1;
		}
	}
	return 0;
}

static void
__dxt_update_seq(struct dxt_interface *dxt_p, uint64_t seq)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct tx_node *txnp, *txnp1, *txnp_ref;

	// remove node from send list (both data and control lists) that not bigger than seq
	pthread_mutex_lock(&priv_p->p_wait_ack_mlck);
	list_for_each_entry_safe(txnp, txnp1, struct tx_node, &priv_p->p_wait_ack_head, tn_list) {
		struct umessage_tx_hdr *umsg_hdr_p = txnp->tn_data;
		if(umsg_hdr_p->um_seq <= seq) {
			// before remove message node, we must ensure associated data message already
			// send out
			txnp_ref = txnp->tn_ref_node;
			if(txnp_ref && !__dxt_dnode_exist(dxt_p, txnp_ref)) {
				continue;
			}

			// message seq smaller than cur ack seq, remove it from control wait ack list,
			// then release its memory
			list_del(&txnp->tn_list);

			// free control message memory
			pp2_p->pp2_op->pp2_put(pp2_p, txnp->tn_data);
			txn_p->tn_op->tn_put_node(txn_p, txnp);

			// if have associated data message, remove it from data wait ack list,
			// then release its memory
			if(txnp_ref) {
				list_del(&txnp_ref->tn_list);
				pp2_p->pp2_op->pp2_put(pp2_p, txnp_ref->tn_data);
				txn_p->tn_op->tn_put_node(txn_p, txnp_ref);
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_wait_ack_mlck);
}
#endif

static void *
_dxt_send_control_thread(void *p)
{
	char *log_header = "_dxt_send_control_thread";
	struct dxt_interface *dxt_p = (struct dxt_interface *)p;
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct tx_node *tnp, *tnp2;
	struct list_head req_head, wait_ack_head;
	ssize_t nwrite;
	int rc;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: start for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: start for dxp...", log_header);
	}

	INIT_LIST_HEAD(&req_head);
	INIT_LIST_HEAD(&wait_ack_head);

next_try:
	if (priv_p->p_to_stop)
		goto out;

	// make sure, dxt not stay in freeze state, it's no need to send in freeze state
	if(priv_p->p_freeze) {
		if(!priv_p->p_send_frozen) {
			priv_p->p_send_frozen = 1;
		}
		sleep(DXT_RETRY_INTERVAL);
		goto next_try;
	}

	// only add req to req_head when req_head is empty,
	pthread_mutex_lock(&priv_p->p_out_mlck);
	if(list_empty(&req_head)) {
		while (!priv_p->p_to_stop && list_empty(&priv_p->p_out_head) && !priv_p->p_freeze) {
			pthread_cond_wait(&priv_p->p_out_cond, &priv_p->p_out_mlck);
		}
		if(priv_p->p_freeze || priv_p->p_to_stop || list_empty(&priv_p->p_out_head)) {
			pthread_mutex_unlock(&priv_p->p_out_mlck);
			goto next_try;
		}
		list_splice_tail_init(&priv_p->p_out_head, &req_head);
	}
	pthread_mutex_unlock(&priv_p->p_out_mlck);

	list_for_each_entry_safe(tnp, tnp2, struct tx_node, &req_head, tn_list) {
		// jump out send ASAP when dxt in freeze state or need to stop
		if (priv_p->p_to_stop) {
			break;
		}

		rc = 0;
		struct umessage_tx_hdr *msg_hdr_p = tnp->tn_data;
		nwrite = util_nw_write_timeout_rc(priv_p->p_csfd, tnp->tn_data_pos, tnp->tn_len, TX_RW_TIMEOUT, &rc);
		if(nwrite > 0 && (size_t)nwrite == tnp->tn_len) {
			// remove node when send out whole message success
			list_del(&tnp->tn_list);

			// ack message will drop after send out,
			// other message type have keep in list until get ack when ack enable
			if(msg_hdr_p->um_type == UMSG_TYPE_ACK) {
				// update ack seq
				priv_p->p_ack_seq = msg_hdr_p->um_seq;

				pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
				txn_p->tn_op->tn_put_node(txn_p, tnp);
				nid_log_debug("%s: send %luth ack message success", log_header, msg_hdr_p->um_seq);
			} else {
				nid_log_debug("%s: send %luth control message success", log_header, msg_hdr_p->um_seq);

#ifdef SEQ_ACK_ENABLE
				// only non-ack message need to wait ack message
				list_add_tail(&tnp->tn_list, &wait_ack_head);
#else
				// when disable auto ack feature, release memory
				pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
				txn_p->tn_op->tn_put_node(txn_p, tnp);
#endif
			}

		} else {
			if(rc < 0) {
				nid_log_warning("%s: failed to send control message from socket %d", log_header, priv_p->p_dsfd);
				__dxt_trigger_recover(dxt_p);
				break;
			}

			// if part of data has been send out, adjust data buffer pos for next send action
			if(nwrite > 0) {
				tnp->tn_data_pos += nwrite;
				tnp->tn_len -= nwrite;
			}

			goto next_try;
		}
	}

	/*
	 * push back un-send reqs to send queue, because it may need to drop all of reqs not
	 * send out latter
	 */
	pthread_mutex_lock(&priv_p->p_out_mlck);
	if (!list_empty(&req_head)) {
		list_splice_init(&req_head, &priv_p->p_out_head);
	}
	pthread_mutex_unlock(&priv_p->p_out_mlck);

#ifdef SEQ_ACK_ENABLE
	// add success send to wait ack list
	pthread_mutex_lock(&priv_p->p_wait_ack_mlck);
	if (!list_empty(&wait_ack_head)) {
		list_splice_tail_init(&wait_ack_head, &priv_p->p_wait_ack_head);
	}
	pthread_mutex_unlock(&priv_p->p_wait_ack_mlck);
#endif

	goto next_try;

out:
	// before quit thread, we must free memory owned by local node list
	list_for_each_entry_safe(tnp, tnp2, struct tx_node, &req_head, tn_list) {
		list_del(&tnp->tn_list);
		pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
		txn_p->tn_op->tn_put_node(txn_p, tnp);
	}

	list_for_each_entry_safe(tnp, tnp2, struct tx_node, &wait_ack_head, tn_list) {
		list_del(&tnp->tn_list);
		pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
		txn_p->tn_op->tn_put_node(txn_p, tnp);
	}

	priv_p->p_send_stop = 1;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: terminate for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: terminate for dxp...", log_header);
	}

	return NULL;
}

static void *
_dxt_recv_data_thread(void *data)
{
	char *log_header = "_dxt_recv_data_thread";
	struct dxt_interface *dxt_p = (struct dxt_interface *)data;
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	int in_ds_index = priv_p->p_in_data_ds;
	uint32_t to_read = 0, has_buffer = 0;
	int nread = 0;
	char *p;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: start for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: start for dxp...", log_header);
	}

next_read:
	if (priv_p->p_to_stop)
		goto out;

	if(priv_p->p_freeze) {
		if(!priv_p->p_recv_data_frozen) {
			priv_p->p_recv_data_frozen = 1;
		}
		sleep(DXT_RETRY_INTERVAL);
		goto next_read;
	}

	if(!has_buffer) {
		p = ds_p->d_op->d_get_buffer(ds_p, in_ds_index, &to_read, NULL);

		if(!p) {
			nid_log_warning("%s: out of page!", log_header);
			assert(0);
		}
		has_buffer = 1;
	}

	nread = util_nw_read_stop(priv_p->p_dsfd, p, to_read, 0, &priv_p->p_to_stop);

	// if nread == 0 here, it means socket has been closed by peer, because when select indict
	// fd has activity, it will read data in usual
	if (nread <= 0) {
		nid_log_warning("%s: failed to read data socket, try recovery ...", log_header);
		__dxt_trigger_recover(dxt_p);

		// buffer get from ds can be re-used after recover, no need to reset has_buffer flag
		goto next_read;
	}

	ds_p->d_op->d_confirm(ds_p, in_ds_index, nread);
	priv_p->p_recv_dseq += nread;

	nid_log_debug("%s: recv_size = %lu", log_header, priv_p->p_recv_dseq);
	has_buffer = 0;

	goto next_read;

out:
	priv_p->p_recv_data_stop = 1;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: terminate for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: terminate for dxp...", log_header);
	}

	return NULL;
}

static void *
_dxt_send_data_thread(void *p)
{
	char *log_header = "_dxt_send_data_thread";
	struct dxt_interface *dxt_p = (struct dxt_interface *)p;
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct tx_node *tnp, *tnp2;
	struct list_head req_head, wait_ack_head;
	ssize_t nwrite;
	int rc;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: start for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: start for dxp...", log_header);
	}

	INIT_LIST_HEAD(&req_head);
	INIT_LIST_HEAD(&wait_ack_head);

next_try:
	if (priv_p->p_to_stop)
		goto out;

	// make sure, dxt not stay in freeze state, it's no need to send in freeze state
	if(priv_p->p_freeze) {
		if(!priv_p->p_send_data_frozen) {
			priv_p->p_send_data_frozen = 1;
		}
		sleep(DXT_RETRY_INTERVAL);
		goto next_try;
	}

	// only add req to req_head when req_head is empty,
	pthread_mutex_lock(&priv_p->p_out_mlck);
	if(list_empty(&req_head)) {
		while (!priv_p->p_to_stop && list_empty(&priv_p->p_out_data_head) && !priv_p->p_freeze) {
			pthread_cond_wait(&priv_p->p_out_cond, &priv_p->p_out_mlck);
		}
		if(priv_p->p_freeze || priv_p->p_to_stop || list_empty(&priv_p->p_out_data_head)) {
			pthread_mutex_unlock(&priv_p->p_out_mlck);
			goto next_try;
		}
		list_splice_tail_init(&priv_p->p_out_data_head, &req_head);
	}
	pthread_mutex_unlock(&priv_p->p_out_mlck);

	if (priv_p->p_to_stop) {
		goto next_try;
	}

	list_for_each_entry_safe(tnp, tnp2, struct tx_node, &req_head, tn_list) {
		// jump out send ASAP when dxt in freeze state or need to stop
		if (priv_p->p_to_stop) {
			break;
		}

		rc = 0;
		nwrite = util_nw_write_timeout_rc(priv_p->p_dsfd, tnp->tn_data_pos, tnp->tn_len, TX_RW_TIMEOUT, &rc);
		if(nwrite > 0 && (size_t)nwrite == tnp->tn_len) {
			// remove node when send out whole message success
			list_del(&tnp->tn_list);

#ifdef SEQ_ACK_ENABLE
			// we can only drop data when we get ack message, here we must keep it
			list_add_tail(&tnp->tn_list, &wait_ack_head);
#else
			// when disable auto ack feature, release memory
			pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
			txn_p->tn_op->tn_put_node(txn_p, tnp);
#endif

			priv_p->p_dsend_seq += nwrite;

			nid_log_debug("%s: send %lu bytes data message", log_header, priv_p->p_dsend_seq);
		} else {
			if(rc < 0) {
				nid_log_warning("%s: failed to send data message from socket %d ...", log_header, priv_p->p_dsfd);
				__dxt_trigger_recover(dxt_p);
				break;
			}

			// if part of data has been send out, adjust data buffer pos for next send action
			if(nwrite > 0) {
				tnp->tn_data_pos += nwrite;
				tnp->tn_len -= nwrite;
				priv_p->p_dsend_seq += nwrite;
			}

			goto next_try;
		}
	}

	/*
	 * push back un-send reqs to send queue, because it may need to drop all of reqs not
	 * send out latter
	 */
	pthread_mutex_lock(&priv_p->p_out_mlck);
	if (!list_empty(&req_head)) {
		list_splice_init(&req_head, &priv_p->p_out_data_head);
	}
	pthread_mutex_unlock(&priv_p->p_out_mlck);

#ifdef SEQ_ACK_ENABLE
	// add success send to wait ack list
	pthread_mutex_lock(&priv_p->p_wait_ack_mlck);
	if (!list_empty(&wait_ack_head)) {
		list_splice_tail_init(&wait_ack_head, &priv_p->p_wait_ack_data_head);
	}
	pthread_mutex_unlock(&priv_p->p_wait_ack_mlck);
#endif

	goto next_try;

out:
	// before quit thread, we must free memory owned by local node list
	list_for_each_entry_safe(tnp, tnp2, struct tx_node, &req_head, tn_list) {
		list_del(&tnp->tn_list);
		pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
		txn_p->tn_op->tn_put_node(txn_p, tnp);
	}

	list_for_each_entry_safe(tnp, tnp2, struct tx_node, &wait_ack_head, tn_list) {
		list_del(&tnp->tn_list);
		pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
		txn_p->tn_op->tn_put_node(txn_p, tnp);
	}

	priv_p->p_send_data_stop = 1;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: terminate for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: terminate for dxp...", log_header);
	}

	return NULL;
}

#ifdef SEQ_ACK_ENABLE
static void
_dxt_send_ack_msg(struct dxt_interface *dxt_p, uint64_t ack_seq)
{
	uint32_t cmd_len = sizeof(struct umessage_tx_hdr);
	struct umessage_tx_hdr *ack_msg_hdr;
	char *send_buf;

	// send back ack message
	send_buf = dxt_p->dx_op->dx_get_buffer(dxt_p, cmd_len);
	memset(send_buf, 0, cmd_len);
	ack_msg_hdr = (struct umessage_tx_hdr *)send_buf;
	ack_msg_hdr->um_type = UMSG_TYPE_ACK;
	ack_msg_hdr->um_seq = ack_seq;
	ack_msg_hdr->um_level = UMSG_LEVEL_TX;
	dxt_p->dx_op->dx_send(dxt_p, send_buf, cmd_len, NULL, 0);
}

// the thread used to scan receive control message, and check if can send back ack message
static void *
_dxt_confirm_ack_thread(void *p)
{
	char *log_header = "_dxt_confirm_ack_thread";
	struct dxt_interface *dxt_p = (struct dxt_interface *)p;
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: start for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: start for dxp...", log_header);
	}

next_try:
	if (priv_p->p_to_stop)
		goto out;

	// make sure, dxt not stay in freeze state, it's no need to send in freeze state
	if(priv_p->p_freeze) {
		if(!priv_p->p_send_ack_frozen) {
			priv_p->p_send_ack_frozen = 1;
		}
		sleep(DXT_RETRY_INTERVAL);
		goto next_try;
	}

	if(priv_p->p_ack_seq < priv_p->p_recv_seq) {
		if(priv_p->p_dxa) {
			nid_log_debug("%s: active side send back ack msg for seq %lu...",
					log_header, priv_p->p_recv_seq);
		} else if(priv_p->p_dxp) {
			nid_log_debug("%s: passive side send back ack msg for seq %lu...",
					log_header, priv_p->p_recv_seq);
		}
		// send back ack mssage
		_dxt_send_ack_msg(dxt_p, priv_p->p_recv_seq);
	}

	// we check data ready in 0.2s interval
	usleep(200*1000);
	goto next_try;

out:
	priv_p->p_send_ack_stop = 1;
	if(priv_p->p_dxa) {
		nid_log_warning("%s: terminate for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: terminate for dxp...", log_header);
	}

	return NULL;
}
#endif


/*
 * Algorithm:
 * 	1> create receiving thread and continue to work as sending thread.
 * 	2> send control requests.
 */
static void
dxt_do_channel(struct dxt_interface *dxt_p)
{
	char *log_header = "dxt_do_channel";
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp;
	struct list_head req_head;
	pthread_attr_t attr;
	pthread_t thread_id;
	int in_ds_index;
	uint32_t to_read = 0, has_buffer = 0, got_msg;
	int nread = 0;
	char *p;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: start for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: start for dxp...", log_header);
	}

	assert(priv_p);

	priv_p->p_cmsg_left = 0;
	priv_p->p_cmsg_pos = NULL;

	priv_p->p_in_ds = ds_p->d_op->d_create_worker(ds_p, NULL, 0, NULL);
	in_ds_index = priv_p->p_in_ds;
	priv_p->p_in_data_ds = ds_p->d_op->d_create_worker(ds_p, NULL, 0, NULL);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _dxt_send_control_thread, dxt_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _dxt_send_data_thread, dxt_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _dxt_recv_data_thread, dxt_p);

#ifdef SEQ_ACK_ENABLE
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _dxt_confirm_ack_thread, dxt_p);
#endif

	INIT_LIST_HEAD(&req_head);
	priv_p->p_is_working = 1;

next_read:
	if (priv_p->p_to_stop)
		goto out;

	if(priv_p->p_freeze) {
		if(!priv_p->p_recv_ctrl_frozen) {
			priv_p->p_recv_ctrl_frozen = 1;
		}
		sleep(DXT_RETRY_INTERVAL);
		goto next_read;
	}

	got_msg = 0;
	if(!has_buffer) {
		p = ds_p->d_op->d_get_buffer(ds_p, in_ds_index, &to_read, NULL);
		if(!p) {
			nid_log_warning("%s: out of page!", log_header);
			assert(0);
		}
		has_buffer = 1;
	}
	nread = util_nw_read_stop(priv_p->p_csfd, p, to_read, 0, &priv_p->p_to_stop);

	// if nread == 0 here, it means socket has been closed by peer
	if (nread <= 0) {
		nid_log_warning("%s: failed to read cotrol message socket, try recovery ...", log_header);
		__dxt_trigger_recover(dxt_p);

		// buffer get from ds can be re-used after recover, no need to reset has_buffer flag
		goto next_read;
	}

	ds_p->d_op->d_confirm(ds_p, in_ds_index, nread);

	if (!priv_p->p_cmsg_pos) {
		assert(!priv_p->p_cmsg_left);
		priv_p->p_cmsg_pos = p;
		priv_p->p_cmsg_left = nread;
	} else {
		priv_p->p_cmsg_left += nread;
	}

	while (priv_p->p_cmsg_left >= UMSG_TX_HEADER_LEN) {
		struct umessage_tx_hdr *msg_hdr =  (struct umessage_tx_hdr *)priv_p->p_cmsg_pos;
		if(msg_hdr->um_len > priv_p->p_cmsg_left) {
			break;
		}

		// remove message in wait ack list
		if(msg_hdr->um_type == UMSG_TYPE_ACK) {
#ifdef SEQ_ACK_ENABLE
			__dxt_update_seq(dxt_p, msg_hdr->um_seq);
#endif
		} else {
			priv_p->p_recv_seq = msg_hdr->um_seq;
		}

		// if message for tx module, callback the process function, the callback function
		// will put request node back to node list again, we only need to insert when
		// message used for upper level
		if(__tx_get_pkg_level(msg_hdr) == UMSG_LEVEL_TX) {
			if(priv_p->p_dxa) {
				priv_p->p_dxt_callback(priv_p->p_dxa, msg_hdr);
			} else if(priv_p->p_dxp) {
				priv_p->p_dxt_callback(priv_p->p_dxp, msg_hdr);
			} else {
				nid_log_error("%s: no dxa or dxp registered", log_header);
				assert(0);
			}
			ds_p->d_op->d_put_buffer2(ds_p, in_ds_index, msg_hdr, msg_hdr->um_len , NULL);
		} else {
			lnp = lstn_p->ln_op->ln_get_node(lstn_p);
			lnp->ln_data = priv_p->p_cmsg_pos;

			list_add_tail(&lnp->ln_list, &req_head);
			got_msg++;
		}

		priv_p->p_cmsg_pos += msg_hdr->um_len;
		priv_p->p_cmsg_left -= msg_hdr->um_len;
	}

	if(got_msg) {
		pthread_mutex_lock(&priv_p->p_in_mlck);
		list_splice_tail_init(&req_head, &priv_p->p_in_head);
		pthread_mutex_unlock(&priv_p->p_in_mlck);
	}

	if (!priv_p->p_cmsg_left)
		priv_p->p_cmsg_pos = NULL;

	has_buffer = 0;

	goto next_read;

out:
	priv_p->p_recv_ctrl_stop = 1;
	if(priv_p->p_dxa) {
		nid_log_warning("%s: terminate for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: terminate for dxp...", log_header);
	}

	return;
}

/*
 * Algorithm:
 * 	get next control request
 */
static struct list_node *
dxt_get_request(struct dxt_interface *dxt_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct list_node *lnp = NULL;
	struct umessage_tx_hdr *msg_hdr;

	// when dxt in waiting to stop status, we should not provide request node to caller
	if(priv_p->p_to_stop)
		goto out;

	pthread_mutex_lock(&priv_p->p_in_mlck);
	if (!list_empty(&priv_p->p_in_head)) {
		lnp = list_first_entry(&priv_p->p_in_head, struct list_node, ln_list);

		/*
		 * if control message associated with a data message, here must ensure data message
		 * has already received. otherwise, not return any request to caller
		 *
		 */
		msg_hdr = lnp->ln_data;
		if((msg_hdr->um_dseq != 0) || (msg_hdr->um_dlen != 0)) {
			if(! dxt_p->dx_op->dx_is_data_ready(dxt_p, priv_p->p_in_data_ds,
					msg_hdr->um_dseq + msg_hdr->um_dlen)) {
				lnp = NULL;
				goto out;
			}
		}
		list_del(&lnp->ln_list);
	}
	pthread_mutex_unlock(&priv_p->p_in_mlck);

out:
	return lnp;
}

/*
 * Algorithm:
 * 	return one control request
 *
 * Note:
 * 	Not multithreading safe for ds
 */
static void
dxt_put_request(struct dxt_interface *dxt_p, struct list_node *lnp)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	int in_ds_index = priv_p->p_in_ds;

	ds_p->d_op->d_put_buffer2(ds_p, in_ds_index, lnp->ln_data, priv_p->p_req_size , NULL);
	lstn_p->ln_op->ln_put_node(lstn_p, lnp);
}

static void
dxt_get_request_list(struct dxt_interface *dxt_p, struct list_head *req_head)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct list_node *lnp, *lnp1;
	struct umessage_tx_hdr *msg_hdr;

	// when dxt in waiting to stop status, we should not provide request node to caller
	if(priv_p->p_to_stop)
		return;

	pthread_mutex_lock(&priv_p->p_in_mlck);
	if (!list_empty(&priv_p->p_in_head)) {
		list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_in_head, ln_list) {
			/*
			 * if control message associated with a data message, here must ensure data message
			 * has already received. otherwise, not return any request to caller
			 *
			 */
			msg_hdr = lnp->ln_data;
			if((msg_hdr->um_dseq != 0) || (msg_hdr->um_dlen != 0)) {
				if(! dxt_p->dx_op->dx_is_data_ready(dxt_p, priv_p->p_in_data_ds,
						msg_hdr->um_dseq + msg_hdr->um_dlen)) {
					break;
				}
			}
			list_del(&lnp->ln_list);
			list_add_tail(&lnp->ln_list, req_head);
		}
	}
	pthread_mutex_unlock(&priv_p->p_in_mlck);
}

static void
dxt_put_request_list(struct dxt_interface *dxt_p, struct list_head *req_head)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct ds_interface *ds_p = priv_p->p_ds;
	struct list_node *lnp, *lnp1;

	list_for_each_entry_safe(lnp, lnp1, struct list_node, req_head, ln_list) {
		list_del(&lnp->ln_list);
		ds_p->d_op->d_put_buffer2(ds_p, priv_p->p_in_ds, lnp->ln_data, priv_p->p_req_size , NULL);
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}
}


static void *
dxt_get_buffer(struct dxt_interface *dxt_p, uint32_t size)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;

	return pp2_p->pp2_op->pp2_get(pp2_p, size);
}

static ssize_t
dxt_send(struct dxt_interface *dxt_p, void *send_buf, size_t len, void *send_dbuf, size_t dlen)
{
	char *log_header = "dxt_send";
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct tx_node *tnp, *data_tnp = NULL;
	struct umessage_tx_hdr *msg_hdr;
	uint64_t dseq = 0;

	assert(send_buf || send_dbuf);

	// when dxt in waiting to stop status or freeze status, we should not put request to queue any more
	if(priv_p->p_to_stop || priv_p->p_freeze) {
		struct pp2_interface *pp2_p = priv_p->p_pp2;
		if(send_buf) {
			pp2_p->pp2_op->pp2_put(pp2_p, send_buf);
		}
		if(send_dbuf) {
			pp2_p->pp2_op->pp2_put(pp2_p, send_dbuf);
		}

		return -1;
	}

	// put tn_get_node out of lock for better performance
	if(send_dbuf) {
		assert(dlen > 0);
		data_tnp = txn_p->tn_op->tn_get_node(txn_p);
	}

	if(send_buf) {
		// control message size validate
		assert((len > 0) && (len % priv_p->p_req_size) == 0);
		tnp = txn_p->tn_op->tn_get_node(txn_p);
	}

	pthread_mutex_lock(&priv_p->p_out_mlck);
	if (send_dbuf) {
		data_tnp->tn_len = dlen;
		data_tnp->tn_data = send_dbuf;
		data_tnp->tn_data_pos = send_dbuf;
		data_tnp->tn_dseq = dseq;
		dseq += dlen;
	}

	if(send_buf) {
		tnp->tn_len = len;
		tnp->tn_data = send_buf;
		tnp->tn_data_pos = send_buf;
		tnp->tn_ref_node = data_tnp;
		msg_hdr = (struct umessage_tx_hdr *)send_buf;

		if(msg_hdr->um_type != UMSG_TYPE_ACK) {
			// update seq
			msg_hdr->um_seq = priv_p->p_send_seq++;
			if(data_tnp) {
				msg_hdr->um_dlen = dlen;
				msg_hdr->um_dseq = priv_p->p_dsend_seq;
				priv_p->p_dsend_seq += dlen;
			} else {
				msg_hdr->um_dlen = 0;
				msg_hdr->um_dseq = 0;
			}
			if(priv_p->p_dxa) {
				nid_log_debug("%s: send msg with seq %lu", log_header, msg_hdr->um_seq);
			}
		}
	}

	if(send_buf)
		list_add_tail(&tnp->tn_list, &priv_p->p_out_head);
	if (send_dbuf)
		list_add_tail(&data_tnp->tn_list, &priv_p->p_out_data_head);
	if(!priv_p->p_freeze)
		pthread_cond_broadcast(&priv_p->p_out_cond);
	pthread_mutex_unlock(&priv_p->p_out_mlck);

	return 0;
}

static int
dxt_is_data_ready(struct dxt_interface *dxt_p, int ds_index, uint64_t data_seq)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	return ds_p->d_op->d_ready(ds_p, ds_index, data_seq); 
}

static char *
dxt_data_position(struct dxt_interface *dxt_p, int ds_index, uint64_t data_seq, uint32_t *len)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	return ds_p->d_op->d_position_length(ds_p, ds_index, data_seq, len);	
}

static char *
dxt_get_name(struct dxt_interface *dxt_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	return priv_p->p_dxt_name;
}

static void *
dxt_get_handle(struct dxt_interface *dxt_p)
{
        struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
        return priv_p->p_handle;
}

static void
dxt_stop(struct dxt_interface *dxt_p)
{
	char *log_header = "dxt_stop";
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	if(priv_p->p_dxa) {
		nid_log_warning("%s: stop begin for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: stop begin for dxp...", log_header);
	}

	priv_p->p_to_stop = 1;

	// we must send out signal again and again, because send thread may miss signal
	while(!__dxt_no_thread(dxt_p)) {
		usleep(100*1000);
		pthread_cond_signal(&priv_p->p_out_cond);
	}

	if(priv_p->p_dxa) {
		nid_log_warning("%s: stop success for dxa...", log_header);
	} else if(priv_p->p_dxp) {
		nid_log_warning("%s: stop success for dxp...", log_header);
	}
}

static void
__dxt_clear_queues(struct dxt_private *priv_p)
{
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct tx_node *tnp, *tnp2;
	struct list_node *lnp, *lnp2;

	// free memory node in in/out queue
	if(!list_empty(&priv_p->p_out_head)) {
		list_for_each_entry_safe(tnp, tnp2, struct tx_node, &priv_p->p_out_head, tn_list) {
			list_del(&tnp->tn_list);
			pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
			txn_p->tn_op->tn_put_node(txn_p, tnp);
		}
	}

	if(!list_empty(&priv_p->p_out_data_head)) {
		list_for_each_entry_safe(tnp, tnp2, struct tx_node, &priv_p->p_out_data_head, tn_list) {
			list_del(&tnp->tn_list);
			pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
			txn_p->tn_op->tn_put_node(txn_p, tnp);
		}
	}

	if(!list_empty(&priv_p->p_in_head)) {
		list_for_each_entry_safe(lnp, lnp2, struct list_node, &priv_p->p_in_head, ln_list) {
			list_del(&lnp->ln_list);
			lstn_p->ln_op->ln_put_node(lstn_p, lnp);
		}
	}

	if(!list_empty(&priv_p->p_wait_ack_head)) {
		list_for_each_entry_safe(lnp, lnp2, struct list_node, &priv_p->p_wait_ack_head, ln_list) {
			list_del(&lnp->ln_list);
			lstn_p->ln_op->ln_put_node(lstn_p, lnp);
		}
	}

	if(!list_empty(&priv_p->p_wait_ack_data_head)) {
		list_for_each_entry_safe(lnp, lnp2, struct list_node, &priv_p->p_wait_ack_data_head, ln_list) {
			list_del(&lnp->ln_list);
			lstn_p->ln_op->ln_put_node(lstn_p, lnp);
		}
	}
}

static void
dxt_cleanup(struct dxt_interface *dxt_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct ds_interface	*ds_p = priv_p->p_ds;

	dxt_p->dx_op->dx_stop(dxt_p);

	ds_p->d_op->d_release_worker(ds_p, priv_p->p_in_ds, NULL);
	ds_p->d_op->d_release_worker(ds_p, priv_p->p_in_data_ds, NULL);

	// clear up queues include send/recv/wait queue
	__dxt_clear_queues(priv_p);

	pthread_mutex_destroy(&priv_p->p_lck);
	pthread_mutex_destroy(&priv_p->p_in_mlck);
	pthread_mutex_destroy(&priv_p->p_out_mlck);
	pthread_mutex_destroy(&priv_p->p_wait_ack_mlck);
	pthread_cond_destroy(&priv_p->p_out_cond);

	free(priv_p);
	return;
}

void
dxt_bind_dxa(struct dxt_interface *dxt_p, struct dxa_interface *dxa_p, nid_tx_callback dxa_callback)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	priv_p->p_dxa = dxa_p;
	priv_p->p_dxt_callback = dxa_callback;
}

static void
dxt_bind_dxp(struct dxt_interface *dxt_p, struct dxp_interface *dxp_p, nid_tx_callback dxp_callback)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	priv_p->p_dxp = dxp_p;
	priv_p->p_dxt_callback = dxp_callback;
}

// recover dxt to working state
static void
dxt_recover(struct dxt_interface *dxt_p, int dsfd, int csfd)
{
	char *log_header = "dxt_recover";
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct list_head wait_ack_head, wait_ack_data_head;

	nid_log_info("%s: start ...", log_header);

	INIT_LIST_HEAD(&wait_ack_head);
	INIT_LIST_HEAD(&wait_ack_data_head);

	// get message that need to re-send
	pthread_mutex_lock(&priv_p->p_wait_ack_mlck);
	list_splice_tail_init(&priv_p->p_wait_ack_head, &wait_ack_head);
	list_splice_tail_init(&priv_p->p_wait_ack_data_head, &wait_ack_data_head);
	pthread_mutex_unlock(&priv_p->p_wait_ack_mlck);
	nid_log_info("%s: get re-send message ...", log_header);

	// insert message queue into head of send queue
	pthread_mutex_lock(&priv_p->p_out_mlck);
	list_add(&wait_ack_head, &priv_p->p_out_head);
	list_add(&wait_ack_data_head, &priv_p->p_out_data_head);
	pthread_mutex_unlock(&priv_p->p_out_mlck);
	nid_log_info("%s: setup output message queue ...", log_header);

	// attach new socket with dxt module
	priv_p->p_dsfd = dsfd;
	priv_p->p_csfd = csfd;

	priv_p->p_freeze = 0;
	priv_p->p_is_working = 1;

	priv_p->p_send_frozen = 0;
	priv_p->p_send_data_frozen = 0;
	priv_p->p_recv_data_frozen = 0;
	priv_p->p_recv_ctrl_frozen = 0;
	priv_p->p_send_ack_frozen = 0;

	// wake up work threads in dxt module
	pthread_mutex_lock(&priv_p->p_out_mlck);
	pthread_cond_broadcast(&priv_p->p_out_cond);
	pthread_mutex_unlock(&priv_p->p_out_mlck);

	nid_log_info("%s: end ...", log_header);
}


// return data stream associated with a message
// return value:
//	1	data is not received yet
//	0	success get data stream
//	-1	data len in control message is 0
static int
dxt_get_dbuf_vec(struct dxt_interface *dxt_p, struct list_node *lnp, struct iovec *ibuf_p, uint32_t *vec_count)
{
	char *log_header = "dxt_get_dbuf";
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct umessage_tx_hdr *msg_hdr = (struct umessage_tx_hdr *)lnp->ln_data;
	uint64_t dseq = msg_hdr->um_dseq;
	uint32_t dret_len, dlen = msg_hdr->um_dlen;
	char *dbuf = NULL;
	uint8_t two_page = 0;

	nid_log_info("%s: start ...", log_header);
	if(dlen == 0)
		return -1;

	if(! dxt_p->dx_op->dx_is_data_ready(dxt_p, priv_p->p_in_data_ds, dseq + dlen)) {
		return 1;
	}

	*vec_count = 0;
	while(1) {
		dret_len = dlen;
		dbuf = dxt_p->dx_op->dx_data_position(dxt_p, priv_p->p_in_data_ds, dseq, &dret_len);
		if(dret_len < msg_hdr->um_dlen && !two_page) {
			nid_log_info("%s: dseq: %lu with dlen: %u in 2 pages", log_header,
					dseq, msg_hdr->um_dlen);
			two_page = 1;
		}
		ibuf_p[*vec_count].iov_base = dbuf;
		ibuf_p[*vec_count].iov_len = dret_len;
		(*vec_count)++;
		dlen -= dret_len;
		dseq += dret_len;
		if(dlen == 0) {
			break;
		}
		assert(dlen > 0);
	}

	return 0;
}

// put back data stream memory to ds module
static void
dxt_put_dbuf_vec(struct dxt_interface *dxt_p, struct iovec *ibuf_p, uint32_t vec_count)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	char *dbuf;
	uint32_t dlen, i;

	for(i = 0; i < vec_count; i++) {
		dbuf = ibuf_p[i].iov_base;
		dlen = ibuf_p[i].iov_len;
		ds_p->d_op->d_put_buffer2(ds_p, priv_p->p_in_data_ds, dbuf, dlen, NULL);
	}
}


// return data stream offset which associated with a message
static char*
dxt_get_dbuf(struct dxt_interface *dxt_p, struct list_node *lnp)
{
	char *log_header = "dxt_get_dbuf";
	struct umessage_tx_hdr *msg_hdr = (struct umessage_tx_hdr *)lnp->ln_data;
	char *dbuf = NULL, *dbuf_base;
	struct iovec ibuf[2];
	int rc;
	uint32_t vec_count, i;

	memset(&ibuf, 0, sizeof(ibuf));
	rc = dxt_p->dx_op->dx_get_dbuf_vec(dxt_p, lnp, ibuf, &vec_count);
	assert(vec_count <= 2);
	switch(rc) {
	case -1:
		nid_log_debug("%s: no data associated with control message", log_header);
		goto out;

	case 0:
		nid_log_debug("%s: data associated with control message ready", log_header);
		goto merge_mem;

	case 1:
		nid_log_debug("%s: data associated with control message not ready", log_header);
		goto out;
	}

merge_mem:
	// build a big buffer and merge data in ds to the buffer
	dbuf = malloc(msg_hdr->um_dlen);
	dbuf_base = dbuf;
	if(dbuf) {
		for(i = 0; i < vec_count; i++) {
			memcpy(dbuf, ibuf[i].iov_base, ibuf[i].iov_len);
			dbuf += ibuf[i].iov_len;
		}
	}

	// put buf back into ds
	dxt_p->dx_op->dx_put_dbuf_vec(dxt_p, ibuf, vec_count);

	dbuf = dbuf_base;

out:

	return dbuf;
}

// release data stream segment which associated with a message
static void
dxt_put_dbuf(char *dbuf)
{
	free(dbuf);
}

static void
dxt_get_status(struct dxt_interface *dxt_p, struct dxt_status *stat_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	assert(stat_p);
	stat_p->ack_seq = priv_p->p_ack_seq;
	stat_p->cmsg_left = priv_p->p_cmsg_left;
	stat_p->dsend_seq = priv_p->p_dsend_seq;
	stat_p->recv_dseq = priv_p->p_recv_dseq;
	stat_p->recv_seq = priv_p->p_recv_seq;
	stat_p->send_seq = priv_p->p_send_seq;
}

static int
dxt_is_working(struct dxt_interface *dxt_p) {
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	return priv_p->p_is_working;
}

static int
dxt_is_new(struct dxt_interface *dxt_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	if(priv_p->p_dxa || priv_p->p_dxp) {
		return 0;
	} else {
		return 1;
	}
}

static void
dxt_freeze(struct dxt_interface *dxt_p)
{
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	priv_p->p_freeze = 1;
}

static int
dxt_reset_all(struct dxt_interface *dxt_p)
{
	char *log_header = "dxt_reset_all";
	struct dxt_private *priv_p = (struct dxt_private *)dxt_p->dx_private;

	if(!priv_p->p_freeze) {
		nid_log_debug("%s: reset op can only allowed in freeze state", log_header);
		return -1;
	}

	// clear up queues include send/recv/wait queue
	__dxt_clear_queues(priv_p);

	// reset status variable
	priv_p->p_dsend_seq = 0;
	priv_p->p_send_seq = 0;
	priv_p->p_ack_seq = 0;
	priv_p->p_recv_seq = 0;
	priv_p->p_recv_dseq = 0;
	priv_p->p_cmsg_left = 0;
	priv_p->p_cmsg_pos = NULL;

	return 0;
}

struct dxt_operations dxt_op = {
	.dx_do_channel = dxt_do_channel,
	.dx_get_request = dxt_get_request,
	.dx_put_request = dxt_put_request,
	.dx_get_request_list = dxt_get_request_list,
	.dx_put_request_list = dxt_put_request_list,
	.dx_get_buffer = dxt_get_buffer,
	.dx_send = dxt_send,
	.dx_is_data_ready = dxt_is_data_ready,
	.dx_data_position = dxt_data_position,
	.dx_get_name = dxt_get_name,
	.dx_get_handle = dxt_get_handle,
	.dx_stop = dxt_stop,
	.dx_cleanup = dxt_cleanup,
	.dx_bind_dxa = dxt_bind_dxa,
	.dx_bind_dxp = dxt_bind_dxp,
	.dx_recover = dxt_recover,
	.dx_get_dbuf = dxt_get_dbuf,
	.dx_put_dbuf = dxt_put_dbuf,
	.dx_get_dbuf_vec = dxt_get_dbuf_vec,
	.dx_put_dbuf_vec = dxt_put_dbuf_vec,
	.dx_get_status = dxt_get_status,
	.dx_is_working = dxt_is_working,
	.dx_is_new = dxt_is_new,
	.dx_all_frozen = dxt_all_frozen,
	.dx_freeze = dxt_freeze,
	.dx_reset_all = dxt_reset_all,
};

int
dxt_initialization(struct dxt_interface *dxt_p, struct dxt_setup *setup)
{
	char *log_header = "dxt_initialization";
	struct dxt_private *priv_p;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);
	dxt_p->dx_op = &dxt_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dxt_p->dx_private = priv_p;

	priv_p->p_lstn = setup->lstn;
	priv_p->p_txn = setup->txn;
	priv_p->p_csfd = setup->csfd;
	priv_p->p_dsfd = setup->dsfd;
	priv_p->p_ds = setup->ds;
	priv_p->p_pp2 = setup->pp2;
	priv_p->p_req_size = setup->req_size;
	strcpy(priv_p->p_dxt_name, setup->dxt_name);
	priv_p->p_handle = setup->handle;
	priv_p->p_freeze = 0;
	priv_p->p_dxa = NULL;
	priv_p->p_dxp = NULL;
	priv_p->p_dxt_callback = NULL;
	priv_p->p_dsend_seq = 0;
	priv_p->p_send_seq = 0;
	priv_p->p_ack_seq = 0;
	priv_p->p_recv_dseq = 0;
	priv_p->p_cmsg_left = 0;
	priv_p->p_is_working = 0;

	INIT_LIST_HEAD(&priv_p->p_in_head);
	INIT_LIST_HEAD(&priv_p->p_out_head);
	INIT_LIST_HEAD(&priv_p->p_out_data_head);
	INIT_LIST_HEAD(&priv_p->p_wait_ack_head);
	INIT_LIST_HEAD(&priv_p->p_wait_ack_data_head);

	pthread_mutex_init(&priv_p->p_lck, NULL);
	pthread_mutex_init(&priv_p->p_in_mlck, NULL);
	pthread_mutex_init(&priv_p->p_out_mlck, NULL);
	pthread_mutex_init(&priv_p->p_wait_ack_mlck, NULL);
	pthread_cond_init(&priv_p->p_out_cond, NULL);

	return 0;
}
