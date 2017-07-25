/*
 * cxt.c
 * 	Implementation of Data Exchange Transfer Module
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "util_nw.h"
#include "nid_shared.h"
#include "nid_log.h"
#include "lstn_if.h"
#include "txn_if.h"
#include "ds_if.h"
#include "pp2_if.h"
#include "cxt_if.h"
#include "cxa_if.h"
#include "cxp_if.h"
#include "tx_shared.h"


struct cxt_private {
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
	struct list_head	p_wait_ack_head;
	int			p_sfd;
	int			p_in_ds;
	uint32_t		p_req_size;	// must be power of 2
	uint8_t			p_is_working;
	uint8_t			p_to_stop;
	uint8_t			p_send_stop;	// 3 threads stop flags
	uint8_t			p_recv_stop;
	uint8_t			p_send_ack_stop;
	char			p_cxt_name[NID_MAX_UUID];
	void			*p_handle;
	int			p_freeze;
	struct cxa_interface	*p_cxa;
	struct cxp_interface	*p_cxp;
	nid_tx_callback	p_cxt_callback;
	uint64_t                p_send_seq;
	uint64_t                p_wait_seq;
	uint64_t                p_ack_seq;
	uint64_t		p_recv_seq;	// max seq of received control message
	uint32_t		p_cmsg_left;	// un-processed bytes in control message receive buffer
	char			*p_cmsg_pos;	// cur pos in receive control message stream
	uint8_t			p_send_frozen;	// 3 threads freezed flags
	uint8_t			p_recv_ctrl_frozen;
	uint8_t			p_send_ack_frozen;
};

static int
__cxt_no_thread(struct cxt_interface *cxt_p)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	return (priv_p->p_send_ack_stop && priv_p->p_recv_stop && priv_p->p_send_stop);
}

#ifdef SEQ_ACK_ENABLE
static void
__cxt_update_seq(struct cxt_interface *cxt_p, uint64_t seq) {
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct tx_node *txnp, *txnp1;
	priv_p->p_ack_seq = seq;

	// remove node from send list that not bigger than seq
	pthread_mutex_lock(&priv_p->p_wait_ack_mlck);
	list_for_each_entry_safe(txnp, txnp1, struct tx_node, &priv_p->p_wait_ack_head, tn_list) {
		struct umessage_tx_hdr *umsg_hdr_p = txnp->tn_data;
		if(umsg_hdr_p->um_seq <= seq) {
			list_del(&txnp->tn_list);
			// free control message memory
			pp2_p->pp2_op->pp2_put(pp2_p, txnp->tn_data);
			txn_p->tn_op->tn_put_node(txn_p, txnp);
		}
	}
	pthread_mutex_unlock(&priv_p->p_wait_ack_mlck);
}
#endif

static void
__cxt_trigger_recover(struct cxt_interface *cxt_p)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	if(priv_p->p_freeze) {
		return;
	}
	priv_p->p_freeze = 1;
	priv_p->p_is_working = 0;

	if(priv_p->p_cxa) {
		// when cxt used by cxa module, it cannot destroy, unless cxa destroy it
		struct cxa_interface *cxa_p = priv_p->p_cxa;

		// notify cxa recovery connections and pause cxt send/receive action
		cxa_p->cx_op->cx_mark_recover_cxt(cxa_p);
	} else if(priv_p->p_cxp) {
		struct cxp_interface *cxp_p = priv_p->p_cxp;

		// pause cxt receive action
		cxp_p->cx_op->cx_mark_recover_cxt(cxp_p);
	}
}


static void *
_cxt_send_control_thread(void *p)
{
	char *log_header = "_cxt_send_control_thread";
	struct cxt_interface *cxt_p = (struct cxt_interface *)p;
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct tx_node *tnp, *tnp2;
	struct list_head req_head, wait_ack_head;
	ssize_t nwrite;
	int rc;

	nid_log_warning("%s: start ...", log_header);
	INIT_LIST_HEAD(&req_head);
	INIT_LIST_HEAD(&wait_ack_head);

next_try:
	if (priv_p->p_to_stop)
		goto out;

	// make sure, cxt not stay in freeze state, it's no need to send in freeze state
	if(priv_p->p_freeze) {
		if(!priv_p->p_send_frozen) {
			priv_p->p_send_frozen = 1;
		}
		sleep(CXT_RETRY_INTERVAL);
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
		// jump out send ASAP when cxt in freeze state or need to stop
		if (priv_p->p_to_stop || priv_p->p_freeze) {
			break;
		}

		struct umessage_tx_hdr *msg_hdr_p = tnp->tn_data;
		rc = 0;
		nwrite = util_nw_write_timeout_rc(priv_p->p_sfd, tnp->tn_data_pos, tnp->tn_len, TX_RW_TIMEOUT, &rc);
		if(nwrite > 0 && (size_t)nwrite == tnp->tn_len) {
			// remove node when send out whole message
			list_del(&tnp->tn_list);

			// ack message will drop after send out, other message type have keep in list
			// until get ack
			if(msg_hdr_p->um_type == UMSG_TYPE_ACK) {
				pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
				txn_p->tn_op->tn_put_node(txn_p, tnp);
				nid_log_debug("%s: send %luth ack message success", log_header, msg_hdr_p->um_seq);
			} else {
#ifdef SEQ_ACK_ENABLE
				list_add_tail(&tnp->tn_list, &wait_ack_head);
#else
				// when disable auto ack feature, release memory
				pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
				txn_p->tn_op->tn_put_node(txn_p, tnp);
#endif
				nid_log_debug("%s: send %luth control message success", log_header, msg_hdr_p->um_seq);
			}
		} else {
			if(rc < 0) {
				nid_log_warning("%s: failed to send control message from socket %d", log_header, priv_p->p_sfd);
				__cxt_trigger_recover(cxt_p);
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

#ifdef SEQ_ACK_ENABLE
	// add success send to wait ack list
	pthread_mutex_lock(&priv_p->p_wait_ack_mlck);
	if (!list_empty(&wait_ack_head)) {
		list_splice_tail_init(&priv_p->p_wait_ack_head, &wait_ack_head);
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

	if(priv_p->p_cxa) {
		nid_log_warning("%s: terminate for cxa...", log_header);
	} else if(priv_p->p_cxp) {
		nid_log_warning("%s: terminate for cxp...", log_header);
	}

	return NULL;
}

static int
cxt_all_frozen(struct cxt_interface *cxt_p)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	int rc;

	rc = (priv_p->p_send_ack_frozen && priv_p->p_send_frozen  &&
			priv_p->p_recv_ctrl_frozen);

	/*
	 * if not all threads frozen, it may means send message thread fall in wait_cond
	 * wake it up
	 */
	if(!rc) {
		pthread_mutex_lock(&priv_p->p_out_mlck);
		pthread_cond_broadcast(&priv_p->p_out_cond);
		pthread_mutex_unlock(&priv_p->p_out_mlck);
	}

	return rc;
}

#ifdef SEQ_ACK_ENABLE
static void
_cxt_send_ack_msg(struct cxt_interface *cxt_p, uint64_t ack_seq)
{
	uint32_t cmd_len = sizeof(struct umessage_tx_hdr);
	struct umessage_tx_hdr *ack_msg_hdr;
	char *send_buf;

	// send back ack message
	send_buf = cxt_p->cx_op->cx_get_buffer(cxt_p, cmd_len);
	memset(send_buf, 0, cmd_len);
	ack_msg_hdr = (struct umessage_tx_hdr *)send_buf;
	ack_msg_hdr->um_type = UMSG_TYPE_ACK;
	ack_msg_hdr->um_seq = ack_seq;
	ack_msg_hdr->um_level = UMSG_LEVEL_TX;
	cxt_p->cx_op->cx_send(cxt_p, send_buf, cmd_len);
}

// the thread used to scan receive control message, and check if can send back ack message
static void *
_cxt_confirm_ack_thread(void *p)
{
	char *log_header = "_cxt_confirm_ack_thread";
	struct cxt_interface *cxt_p = (struct cxt_interface *)p;
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	if(priv_p->p_cxa) {
		nid_log_warning("%s: start for cxa...", log_header);
	} else if(priv_p->p_cxp) {
		nid_log_warning("%s: start for cxp...", log_header);
	}

next_try:
	if (priv_p->p_to_stop)
		goto out;

	// make sure, cxt not stay in freeze state, it's no need to send in freeze state
	if(priv_p->p_freeze) {
		if(!priv_p->p_send_ack_frozen) {
			priv_p->p_send_ack_frozen = 1;
		}
		sleep(CXT_RETRY_INTERVAL);
		goto next_try;
	}

	if(priv_p->p_ack_seq < priv_p->p_recv_seq) {
		if(priv_p->p_cxa) {
			nid_log_debug("%s: active side send back ack msg for seq %lu...",
					log_header, priv_p->p_recv_seq);
		} else if(priv_p->p_cxp) {
			nid_log_debug("%s: passive side send back ack msg for seq %lu...",
					log_header, priv_p->p_recv_seq);
		}
		// send back ack mssage
		_cxt_send_ack_msg(cxt_p, priv_p->p_recv_seq);
	}

	// we check data ready in 0.2s interval
	usleep(200*1000);
	goto next_try;

out:
	priv_p->p_send_ack_stop = 1;
	if(priv_p->p_cxa) {
		nid_log_warning("%s: terminate for cxa...", log_header);
	} else if(priv_p->p_cxp) {
		nid_log_warning("%s: terminate for cxp...", log_header);
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
cxt_do_channel(struct cxt_interface *cxt_p)
{
	char *log_header = "cxt_do_channel";
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
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

	nid_log_warning("%s: start ...", log_header);
	assert(priv_p);

	priv_p->p_cmsg_left = 0;
	priv_p->p_cmsg_pos = NULL;

	priv_p->p_in_ds = ds_p->d_op->d_create_worker(ds_p, NULL, 0, NULL);
	in_ds_index = priv_p->p_in_ds;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _cxt_send_control_thread, cxt_p);

#ifdef SEQ_ACK_ENABLE
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _cxt_confirm_ack_thread, cxt_p);
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
		sleep(CXT_RETRY_INTERVAL);
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
	nread = util_nw_read_stop(priv_p->p_sfd, p, to_read, 0, &priv_p->p_to_stop);

	// if nread == 0 here, it means socket has been closed by peer
	if (nread <= 0) {
		nid_log_warning("%s: failed to read cotrol message socket, try recovery ...", log_header);
		__cxt_trigger_recover(cxt_p);
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

		// refresh seq if needed
		if(msg_hdr->um_type == UMSG_TYPE_ACK) {
#ifdef SEQ_ACK_ENABLE
			__cxt_update_seq(cxt_p, msg_hdr->um_seq);
#endif
		} else {
			priv_p->p_recv_seq = msg_hdr->um_seq;
		}

		// if package for cxa/cxp, callback the process function
		if(__tx_get_pkg_level(msg_hdr) == UMSG_LEVEL_TX) {
			if(priv_p->p_cxa) {
				priv_p->p_cxt_callback(priv_p->p_cxa, msg_hdr);
			} else if(priv_p->p_cxp) {
				priv_p->p_cxt_callback(priv_p->p_cxp, msg_hdr);
			} else {
				nid_log_error("%s: no cxa or cxp registered", log_header);
				assert(0);
			}
			ds_p->d_op->d_put_buffer2(ds_p, in_ds_index, msg_hdr, msg_hdr->um_len , NULL);
		} else {
			lnp = lstn_p->ln_op->ln_get_node(lstn_p);
			lnp->ln_data = priv_p->p_cmsg_pos;

			got_msg++;
			list_add_tail(&lnp->ln_list, &req_head);
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
	priv_p->p_recv_stop = 1;
	if(priv_p->p_cxa) {
		nid_log_warning("%s: terminate for cxa...", log_header);
	} else if(priv_p->p_cxp) {
		nid_log_warning("%s: terminate for cxp...", log_header);
	}

	return;
}

/*
 * Algorithm:
 * 	get next request
 */
static struct list_node *
cxt_get_next_one(struct cxt_interface *cxt_p)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct list_node *lnp = NULL;

	pthread_mutex_lock(&priv_p->p_in_mlck);
	if (!list_empty(&priv_p->p_in_head)) {
		lnp = list_first_entry(&priv_p->p_in_head, struct list_node, ln_list);
		list_del(&lnp->ln_list);
	}
	pthread_mutex_unlock(&priv_p->p_in_mlck);

	return lnp;
}

/*
 * Algorithm:
 * 	return one request
 *
 * NOte:
 * 	Not multithreading safe for ds
 */
static void
cxt_put_one(struct cxt_interface *cxt_p, struct list_node *lnp)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	int in_ds_index = priv_p->p_in_ds;

	ds_p->d_op->d_put_buffer2(ds_p, in_ds_index, lnp->ln_data, priv_p->p_req_size , NULL);
	lstn_p->ln_op->ln_put_node(lstn_p, lnp);
}

static void
cxt_get_list(struct cxt_interface *cxt_p, struct list_head *req_head)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	pthread_mutex_lock(&priv_p->p_in_mlck);
	if (!list_empty(&priv_p->p_in_head)) {
		list_splice_tail_init(&priv_p->p_in_head, req_head);
	}
	pthread_mutex_unlock(&priv_p->p_in_mlck);
}

static void
cxt_put_list(struct cxt_interface *cxt_p, struct list_head *req_head)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct ds_interface *ds_p = priv_p->p_ds;
	struct list_node *lnp, *lnp1;

	list_for_each_entry_safe(lnp, lnp1, struct list_node, req_head, ln_list){
		list_del(&lnp->ln_list);
		ds_p->d_op->d_put_buffer2(ds_p, priv_p->p_in_ds, lnp->ln_data, priv_p->p_req_size , NULL);
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}
}

/*
 * Note:
 * 	size to allocate must be multiple of p_req_size
 */
static void *
cxt_get_buffer(struct cxt_interface *cxt_p, uint32_t size)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	uint32_t num_req = size/priv_p->p_req_size;

	assert(num_req > 0);
	assert(size == (num_req*priv_p->p_req_size));

	return pp2_p->pp2_op->pp2_get(pp2_p, size);
}

/*
 * Note:
 * 	send_buf must be allocated by cxt_get_buffer.
 */
static ssize_t
cxt_send(struct cxt_interface *cxt_p, void *send_buf, size_t len)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct tx_node *tnp;
	struct umessage_tx_hdr *msg_hdr;

	assert(send_buf);
	assert((len > 0) && (len % priv_p->p_req_size == 0));

	// when cxt in waiting to stop status or freeze status, we should not put request to queue any more
	if(priv_p->p_to_stop || priv_p->p_freeze) {
		struct pp2_interface *pp2_p = priv_p->p_pp2;
		pp2_p->pp2_op->pp2_put(pp2_p, send_buf);

		return -1;
	}

	tnp = txn_p->tn_op->tn_get_node(txn_p);
	tnp->tn_len = len;
	tnp->tn_data = send_buf;
	msg_hdr = (struct umessage_tx_hdr *)send_buf;

	pthread_mutex_lock(&priv_p->p_out_mlck);
	if(msg_hdr->um_type != UMSG_TYPE_ACK) {
		// update seq
		msg_hdr->um_seq = priv_p->p_send_seq++;
		priv_p->p_wait_seq++;
	}
	list_add_tail(&tnp->tn_list, &priv_p->p_out_head);
	if(!priv_p->p_freeze)
		pthread_cond_broadcast(&priv_p->p_out_cond);
	pthread_mutex_unlock(&priv_p->p_out_mlck);

	return 0;
}

static void
cxt_stop(struct cxt_interface *cxt_p)
{
	char *log_header = "cxt_stop";
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	if(priv_p->p_cxa) {
		nid_log_warning("%s: stop begin for cxa...", log_header);
	} else if(priv_p->p_cxp) {
		nid_log_warning("%s: stop begin for cxp...", log_header);
	}

	priv_p->p_to_stop = 1;

	while(!__cxt_no_thread(cxt_p)) {
		usleep(100*1000);
		pthread_cond_signal(&priv_p->p_out_cond);
	}

	if(priv_p->p_cxa) {
		nid_log_warning("%s: stop success for cxa...", log_header);
	} else if(priv_p->p_cxp) {
		nid_log_warning("%s: stop success for cxp...", log_header);
	}
}


static void
__cxt_clear_queues(struct cxt_private *priv_p)
{
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct tx_node *tnp, *tnp2;
	struct list_node *lnp, *lnp2;

	// free memory node in in/out/wait_ack queue
	list_for_each_entry_safe(tnp, tnp2, struct tx_node, &priv_p->p_out_head, tn_list) {
		list_del(&tnp->tn_list);
		pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
		txn_p->tn_op->tn_put_node(txn_p, tnp);
	}

	list_for_each_entry_safe(tnp, tnp2, struct tx_node, &priv_p->p_wait_ack_head, tn_list) {
		list_del(&tnp->tn_list);
		pp2_p->pp2_op->pp2_put(pp2_p, tnp->tn_data);
		txn_p->tn_op->tn_put_node(txn_p, tnp);
	}

	list_for_each_entry_safe(lnp, lnp2, struct list_node, &priv_p->p_in_head, ln_list) {
		list_del(&lnp->ln_list);
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}

}


static void
cxt_cleanup(struct cxt_interface *cxt_p)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct ds_interface *ds_p = priv_p->p_ds;

	cxt_p->cx_op->cx_stop(cxt_p);

	// clear up queues include send/recv/wait queue
	__cxt_clear_queues(priv_p);

	ds_p->d_op->d_release_worker(ds_p, priv_p->p_in_ds, NULL);

	pthread_mutex_destroy(&priv_p->p_in_mlck);
	pthread_mutex_destroy(&priv_p->p_out_mlck);
	pthread_mutex_destroy(&priv_p->p_wait_ack_mlck);
	pthread_cond_destroy(&priv_p->p_out_cond);

	free(priv_p);
	return;
}

static char *
cxt_get_name(struct cxt_interface *cxt_p){
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	return priv_p->p_cxt_name;
}

static void *
cxt_get_handle(struct cxt_interface *cxt_p)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	return priv_p->p_handle;
}

void
cxt_freeze(struct cxt_interface *cxt_p) {
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	priv_p->p_freeze = 1;
}

void
cxt_bind_cxa(struct cxt_interface *cxt_p, struct cxa_interface *cxa_p, nid_tx_callback cxa_callback) {
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	priv_p->p_cxa = cxa_p;
	priv_p->p_cxt_callback = cxa_callback;
}

void
cxt_bind_cxp(struct cxt_interface *cxt_p, struct cxp_interface *cxp_p, nid_tx_callback cxp_callback) {
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	priv_p->p_cxp = cxp_p;
	priv_p->p_cxt_callback = cxp_callback;
}

void
cxt_recover(struct cxt_interface *cxt_p, int csfd) {
	char *log_header = "cxt_refresh_sockfd";
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;
	struct list_head wait_ack_head;

	nid_log_info("%s: start ...", log_header);

	INIT_LIST_HEAD(&wait_ack_head);

	// get message that need to re-send
	pthread_mutex_lock(&priv_p->p_wait_ack_mlck);
	list_splice_tail_init(&priv_p->p_wait_ack_head, &wait_ack_head);
	pthread_mutex_unlock(&priv_p->p_wait_ack_mlck);

	// insert message queue into head of send queue
	pthread_mutex_lock(&priv_p->p_out_mlck);
	list_add(&wait_ack_head, &priv_p->p_out_head);
	pthread_mutex_unlock(&priv_p->p_out_mlck);

	// attach new socket with cxt module
	priv_p->p_sfd = csfd;

	// unfreeze cxt module
	priv_p->p_freeze = 0;
	priv_p->p_is_working = 1;

	priv_p->p_send_frozen = 0;
	priv_p->p_recv_ctrl_frozen = 0;
	priv_p->p_send_ack_frozen = 0;

	// wakt up work threads in cxt module
	pthread_mutex_lock(&priv_p->p_out_mlck);
	pthread_cond_broadcast(&priv_p->p_out_cond);
	pthread_mutex_unlock(&priv_p->p_out_mlck);

	nid_log_info("%s: end ...", log_header);
}

static void
cxt_get_status(struct cxt_interface *cxt_p, struct cxt_status *stat_p)
{
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	assert(stat_p);
	stat_p->ack_seq = priv_p->p_ack_seq;
	stat_p->cmsg_left = priv_p->p_cmsg_left;
	stat_p->recv_seq = priv_p->p_recv_seq;
	stat_p->send_seq = priv_p->p_send_seq;
}

static int
cxt_is_working(struct cxt_interface *cxt_p) {
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	return priv_p->p_is_working;
}

static int
cxt_is_new(struct cxt_interface *cxt_p) {
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	if(priv_p->p_cxa || priv_p->p_cxp) {
		return 0;
	} else {
		return 1;
	}
}


static int
cxt_reset_all(struct cxt_interface *cxt_p)
{
	char *log_header = "cxt_reset_all";
	struct cxt_private *priv_p = (struct cxt_private *)cxt_p->cx_private;

	if(!priv_p->p_freeze) {
		nid_log_debug("%s: reset op can only allowed in freeze state", log_header);
		return -1;
	}

	// clear up queues include send/recv/wait queue
	__cxt_clear_queues(priv_p);

	// reset status variable
	priv_p->p_send_seq = 0;
	priv_p->p_ack_seq = 0;
	priv_p->p_recv_seq = 0;
	priv_p->p_cmsg_left = 0;
	priv_p->p_cmsg_pos = NULL;

	return 0;
}

struct cxt_operations cxt_op = {
	.cx_do_channel = cxt_do_channel,
	.cx_get_next_one = cxt_get_next_one,
	.cx_put_one = cxt_put_one,
	.cx_get_list = cxt_get_list,
	.cx_put_list = cxt_put_list,
	.cx_get_buffer = cxt_get_buffer,
	.cx_send = cxt_send,
	.cx_stop = cxt_stop,
	.cx_cleanup = cxt_cleanup,
	.cx_get_name = cxt_get_name,
	.cx_get_handle = cxt_get_handle,
	.cx_freeze = cxt_freeze,
	.cx_bind_cxa = cxt_bind_cxa,
	.cx_bind_cxp = cxt_bind_cxp,
	.cx_recover = cxt_recover,
	.cx_get_status = cxt_get_status,
	.cx_is_working = cxt_is_working,
	.cx_is_new = cxt_is_new,
	.cx_all_frozen = cxt_all_frozen,
	.cx_reset_all = cxt_reset_all,
};

int
cxt_initialization(struct cxt_interface *cxt_p, struct cxt_setup *setup)
{
	char *log_header = "cxt_initialization";
	struct cxt_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	cxt_p->cx_op = &cxt_op;
	priv_p = calloc(1, sizeof(*priv_p));
	cxt_p->cx_private = priv_p;

	priv_p->p_lstn = setup->lstn;
	priv_p->p_txn = setup->txn;
	priv_p->p_sfd = setup->sfd;
	priv_p->p_ds = setup->ds;
	priv_p->p_pp2 = setup->pp2;
	priv_p->p_req_size = setup->req_size;
	strcpy(priv_p->p_cxt_name, setup->cxt_name);
	priv_p->p_handle = setup->handle;
	priv_p->p_freeze = 0;
	priv_p->p_cxa = setup->cxa;
	priv_p->p_cxp = setup->cxp;
	priv_p->p_cxt_callback = NULL;
	priv_p->p_send_seq = 0;
	priv_p->p_wait_seq = 0;
	priv_p->p_ack_seq = 0;

	INIT_LIST_HEAD(&priv_p->p_in_head);
	INIT_LIST_HEAD(&priv_p->p_out_head);
	INIT_LIST_HEAD(&priv_p->p_wait_ack_head);
	pthread_mutex_init(&priv_p->p_in_mlck, NULL);
	pthread_mutex_init(&priv_p->p_out_mlck, NULL);
	pthread_mutex_init(&priv_p->p_wait_ack_mlck, NULL);
	pthread_cond_init(&priv_p->p_out_cond, NULL);

	return 0;
}
