/*
 * dxa.c
 * 	Implementation of Data Exchange Active Module
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "util_nw.h"
#include "nid.h"
#include "nid_log.h"
#include "dxa_if.h"
#include "pp2_if.h"
#include "dxa_if.h"
#include "dxag_if.h"
#include "dxt_if.h"
#include "dxtg_if.h"
#include "umpk_if.h"
#include "umpk_dxa_if.h"
#include "umpk_dxp_if.h"
#include "tp_if.h"
#include "tx_shared.h"
#include "lstn_if.h"

#define DXA_RW_TIMEOUT	10

struct dxa_private {
	char			p_ip[NID_MAX_IP];
	int			p_csfd;	// control socket fd
	int			p_dsfd;	// data socket fd
	u_short			p_dport;
	struct pp2_interface    *p_dyn_pp2;
	struct dxag_interface	*p_dxag;
	pthread_mutex_t		p_lck;
	int                     p_ka_xtimer;    // ka: send keepalive when decreased to 0
	int                     p_ka_rtimer;    // timeout when decreased to 0
	char			p_to_recover;
	char			p_begin_recover;
	char			p_uuid[NID_MAX_UUID];
	char			p_peer_uuid[NID_MAX_UUID];
	struct dxt_interface    *p_dxt;
	struct dxtg_interface 	*p_dxtg;
	char                    p_dxt_name[NID_MAX_UUID];
	void			*p_handle;
	struct umpk_interface	*p_umpk;
};


static int
dxa_make_control_connection(struct dxa_interface *dxa_p)
{
	char *log_header = "dxa_make_control_connection";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	uint8_t chan_type = NID_CTYPE_DXP;	// I'm the dxa, talking to dxp
	int nwrite;

	nid_log_warning("%s: start ...", log_header);
	priv_p->p_csfd = util_nw_make_connection(priv_p->p_ip, NID_SERVER_PORT);
	if (priv_p->p_csfd < 0) {
		nid_log_error("%s: failed to connect (%s)",
			log_header, priv_p->p_ip);
		return -1;
	}

	nwrite = util_nw_write_timeout(priv_p->p_csfd, (char *)&chan_type, 1, DXA_RW_TIMEOUT);
	if (nwrite != 1) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
		nid_log_error("%s: failed to send out channel type, errno:%d", log_header, errno);
		return -1;
	}

	return priv_p->p_csfd;
}

static int
dxa_make_data_connection(struct dxa_interface *dxa_p, u_short dport)
{
	char *log_header = "dxa_make_data_connection";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;

	nid_log_warning("%s: start ...", log_header);
	priv_p->p_dsfd = util_nw_make_connection(priv_p->p_ip, dport);
	if (priv_p->p_dsfd < 0) {
		nid_log_error("%s: failed to connect (%s)",
			log_header, priv_p->p_ip);
		return -1;
	}

	priv_p->p_dport = dport;
	return priv_p->p_dsfd;
}

static void
__dxa_dxt_callback(void *p1, void *p2)
{
	struct dxa_interface *dxa_p = (struct dxa_interface *)p1;
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct umessage_tx_hdr *msg_hdr =  (struct umessage_tx_hdr *)p2;

	if(msg_hdr->um_req == UMSG_DXP_CMD_KEEPALIVE) {
		// reset keepalive recieve timeout counter
		priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	}

	return;
}

static void
dxa_do_channel(struct dxa_interface *dxa_p)
{
	char *log_header = "dxa_do_channel";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct dxt_interface *dxt_p;
	struct dxtg_interface *dxtg_p = priv_p->p_dxtg;

	nid_log_warning("%s: start ...", log_header);

	dxt_p = dxtg_p->dxg_op->dxg_search_and_create_channel(dxtg_p, priv_p->p_dxt_name, priv_p->p_csfd, priv_p->p_dsfd);

	if(dxt_p) {
		priv_p->p_dxt = dxt_p;
		dxt_p->dx_op->dx_bind_dxa(dxt_p, dxa_p, __dxa_dxt_callback);
		dxt_p->dx_op->dx_do_channel(dxt_p);
	} else {
		nid_log_error("%s: can not create dxt interface", log_header);
	}
}

static int
dxa_cleanup(struct dxa_interface *dxa_p)
{
	char *log_header = "dxa_cleanup";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct dxtg_interface *dxtg_p = priv_p->p_dxtg;
	struct dxt_interface *dxt_p = priv_p->p_dxt;
	int rc;

	nid_log_warning("%s: start ...", log_header);

	// dxa may failed to start dxt, here can only drop dxt when dxt created
	if(dxt_p) {
		rc = dxtg_p->dxg_op->dxg_drop_channel(dxtg_p, dxt_p);
		if(rc) {
			nid_log_warning("%s: failed to drop dxt %s", log_header, priv_p->p_dxt_name);
			goto out;
		}
		dxt_p->dx_op->dx_cleanup(dxt_p);
		free(dxt_p);
	}

	assert(priv_p);
	if(priv_p->p_csfd != 0 && priv_p->p_csfd != -1) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
	}
	if(priv_p->p_dsfd != 0 && priv_p->p_dsfd != -1) {
		close(priv_p->p_dsfd);
		priv_p->p_dsfd = -1;
	}

	pthread_mutex_destroy(&priv_p->p_lck);
	free(priv_p);
	priv_p = NULL;

out:
	return rc;
}

static char*
dxa_get_server_ip(struct dxa_interface *dxa_p)
{
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;

	return priv_p->p_ip;
}

static char*
dxa_get_chan_uuid(struct dxa_interface *dxa_p)
{
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;

	return priv_p->p_uuid;
}

static int
dxa_keepalive_channel(struct dxa_interface *dxa_p) {
	char *log_header = "dxa_keepalive_channel";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct dxt_interface *dxt_p = priv_p->p_dxt;
	struct umessage_tx_hdr req;
	char msg_buf[4096], *send_buf;
	int rc;
	uint32_t cmd_len;

	nid_log_debug("%s: start ...", log_header);

	/*
	 * send create_channel message to the passive site
	 */
	memset(&req, 0, sizeof(req));
	req.um_req = UMSG_DXP_CMD_KEEPALIVE;
	req.um_len = sizeof(req);
	req.um_level = UMSG_LEVEL_TX;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_DXP, &req);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto out;
	}

	send_buf = dxt_p->dx_op->dx_get_buffer(dxt_p, cmd_len);
	memcpy(send_buf, msg_buf, cmd_len);
	dxt_p->dx_op->dx_send(dxt_p, send_buf, cmd_len, NULL, 0);

out:
	return rc;
}

static void
dxa_mark_recover_dxt(struct dxa_interface *dxa_p) {
	char *log_header = "dxa_mark_recover_dxt";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct dxt_interface *dxt_p = priv_p->p_dxt;

	nid_log_error("%s: start recover", log_header);
	pthread_mutex_lock(&priv_p->p_lck);
	if(! priv_p->p_to_recover) {
		nid_log_error("%s: trigger recover", log_header);
		priv_p->p_to_recover = 1;
		pthread_mutex_unlock(&priv_p->p_lck);
		dxt_p->dx_op->dx_freeze(dxt_p);
		return;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	nid_log_error("%s: quit recover", log_header);
}

static int __dxa_setup_connection(struct dxa_interface *);

static int
__dxa_can_continue(struct dxa_interface *dxa_p, struct dxt_status *peer_dxt_stat) {
	char *log_header = "__dxa_dxt_continue";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct dxt_status dxt_stat;
	struct dxt_interface *dxt_p = priv_p->p_dxt;

	// make sure dxt initialized
	if(!dxt_p) {
		nid_log_warning("%s: dxt not initialize yet ...", log_header);
		return -1;
	}

	dxt_p->dx_op->dx_get_status(dxt_p, &dxt_stat);

	/*
	 * think it many times, it's better to drop continue design, because it will add
	 * complex in dxt level to get exactly stopped point, it need point in both active
	 * side and passive side (for data from passive side to active side)
	 *
	 */
	// TODO: consider reliable method to check if can transmit from stopped point
	if(dxt_stat.ack_seq && dxt_stat.ack_seq <= peer_dxt_stat->recv_seq &&
		peer_dxt_stat->ack_seq && dxt_stat.recv_seq >= peer_dxt_stat->ack_seq) {
		return 0;
	}

	return 1;
}

static int
__dxa_get_dxp_info(struct dxa_interface *dxa_p, struct dxt_status *dxt_stat_p)
{
	char *log_header = "__dxa_get_dxp_info";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_dxp_stat_channel dxp_sc_msg;
	struct umessage_tx_hdr *dxp_msghdr = (struct umessage_tx_hdr *)&dxp_sc_msg;
	uint32_t cmd_len;
	int rc, nwrite, nread;
	char msg_buf[1024];

	nid_log_warning("%s: start ...", log_header);

	/*
	 * send get channel state message to the passive site
	 */
	memset(&dxp_sc_msg, 0, sizeof(dxp_sc_msg));
	dxp_msghdr->um_req = UMSG_DXP_CMD_STAT_CHANNEL;
	dxp_msghdr->um_req_code = UMSG_DXP_CODE_NO;
	dxp_sc_msg.um_chan_uuid_len = strlen(priv_p->p_peer_uuid);
	strcpy(dxp_sc_msg.um_chan_uuid, priv_p->p_peer_uuid);
	dxp_msghdr->um_len = UMSG_TX_HEADER_LEN +  sizeof(dxp_sc_msg.um_chan_uuid_len) + dxp_sc_msg.um_chan_uuid_len;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_DXP, dxp_msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto error_lbl;
	}
	nwrite = util_nw_write_n(priv_p->p_csfd, msg_buf, cmd_len);
	if((uint32_t)nwrite != cmd_len) {
		nid_log_error("%s: failed to send state channel command to dxp side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	/*
	 * read response from the passive site
	 */
	nread = util_nw_read_n(priv_p->p_csfd, msg_buf, UMSG_TX_HEADER_LEN);
	if(nread != UMSG_TX_HEADER_LEN) {
		nid_log_error("%s: failed to receive data from dxp side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	struct umessage_dxp_stat_channel_resp dxp_sc_resp_msg, *dxp_sc_resp_msg_p = &dxp_sc_resp_msg;
	dxp_msghdr = (struct umessage_tx_hdr *)dxp_sc_resp_msg_p;
	memset(dxp_sc_resp_msg_p, 0, sizeof(*dxp_sc_resp_msg_p));

	// decode message header
	uint32_t dlen = (uint32_t)nread;
	decode_tx_hdr(msg_buf, &dlen, (struct umessage_tx_hdr *)dxp_msghdr);

	assert(dxp_msghdr->um_len <= 4096);
	assert(dxp_msghdr->um_len >= UMSG_TX_HEADER_LEN);
	if (cmd_len > UMSG_TX_HEADER_LEN)
		util_nw_read_n(priv_p->p_csfd, msg_buf + UMSG_TX_HEADER_LEN, dxp_msghdr->um_len - UMSG_TX_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, dxp_msghdr->um_len, NID_CTYPE_DXP, (void *)dxp_sc_resp_msg_p);
	if (rc) {
		nid_log_error("%s: cannot decode", log_header);
		goto error_lbl;
	}

	if(dxp_sc_resp_msg_p->um_header.um_req == UMSG_DXP_CMD_STAT_CHANNEL &&
			dxp_sc_resp_msg_p->um_header.um_req_code == UMSG_DXP_CODE_STAT_RESP) {
		dxt_stat_p->dsend_seq = dxp_sc_resp_msg_p->dsend_seq;
		dxt_stat_p->send_seq = dxp_sc_resp_msg_p->send_seq;
		dxt_stat_p->ack_seq = dxp_sc_resp_msg_p->ack_seq;
		dxt_stat_p->recv_seq = dxp_sc_resp_msg_p->recv_seq;
		dxt_stat_p->recv_dseq = dxp_sc_resp_msg_p->recv_dseq;
		dxt_stat_p->cmsg_left = dxp_sc_resp_msg_p->cmsg_left;
	} else {
		nid_log_error("%s: got wrong cmd from dxp side", log_header);
		goto error_lbl;
	}

	nid_log_warning("%s: done ...", log_header);

	return 0;

error_lbl:

	return -1;
}

static int
__dxa_start_dxp(struct dxa_interface *dxa_p, int drop_it)
{
	char *log_header = "__dxa_start_dxp";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_dxp_start_work dxp_sw_msg, *dxp_sw_msg_p = &dxp_sw_msg;
	struct umessage_dxp_start_work_resp dxp_sw_msg_resp, *dxp_sw_msg_resp_p = &dxp_sw_msg_resp;
	struct umessage_tx_hdr *msghdr = (struct umessage_tx_hdr *)dxp_sw_msg_p;
	uint32_t cmd_len;
	int rc, nwrite, nread;
	char msg_buf[4096];

	nid_log_warning("%s: start ...", log_header);

	/*
	 * send get channel state message to the passive site
	 */
	memset(dxp_sw_msg_p, 0, sizeof(*dxp_sw_msg_p));
	dxp_sw_msg_p->um_chan_uuid_len = strlen(priv_p->p_peer_uuid);
	strcpy(dxp_sw_msg_p->um_chan_uuid, priv_p->p_peer_uuid);
	msghdr->um_req = UMSG_DXP_CMD_START_WORK;
	msghdr->um_len = UMSG_TX_HEADER_LEN +  sizeof(dxp_sw_msg_p->um_chan_uuid_len) + dxp_sw_msg_p->um_chan_uuid_len;
	// if dxp side have data in dxt side, notify dxp destroy dxt and create it again
	if(drop_it) {
		// passive side have data in dxt module, notify passive side drop data then to work
		// because here is start, not recovery, we have no data in hand, the simplest method
		// to work it let passive side drop all data in its hand
		msghdr->um_req_code = UMSG_DXP_CODE_DROP_ALL;
	}

	memset(msg_buf, 0, sizeof(msg_buf));
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_DXP, msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto error_lbl;
	}
	nid_log_debug("%s: send start work message to passive side", log_header);
	nwrite = util_nw_write_n(priv_p->p_csfd, msg_buf, cmd_len);
	if((uint32_t)nwrite != cmd_len) {
		rc = -1;
		nid_log_error("%s: failed to send start work message to passive side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	/*
	 * read response from the passive site
	 */
	nid_log_debug("%s: read start work response message from passive side", log_header);
	nread = util_nw_read_n(priv_p->p_csfd, msg_buf, UMSG_TX_HEADER_LEN);
	if(nread != UMSG_TX_HEADER_LEN) {
		nid_log_error("%s: failed to receive data from passive side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	memset(&dxp_sw_msg_resp, 0, sizeof(dxp_sw_msg_resp));
	msghdr = (struct umessage_tx_hdr *)dxp_sw_msg_resp_p;

	// decode message header
	uint32_t dlen = (uint32_t)nread;
	decode_tx_hdr(msg_buf, &dlen, msghdr);

	assert(msghdr->um_len <= 4096);
	assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
	dlen = msghdr->um_len - UMSG_TX_HEADER_LEN;
	if (dlen > 0) {
		nread = util_nw_read_n(priv_p->p_csfd, msg_buf + UMSG_TX_HEADER_LEN, dlen);
		if((uint32_t)nread != dlen) {
			assert(0);
			nid_log_error("%s: failed to receive data from passive side, errno:%d",
					log_header, errno);
			goto error_lbl;
		}
	}

	// decode message
	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, msghdr->um_len, NID_CTYPE_DXP, dxp_sw_msg_resp_p);
	if(rc) {
		nid_log_error("%s: failed to decode umpk message", log_header);
		return -1;
	}

	nid_log_warning("%s: done ...", log_header);
	return 0;

error_lbl:

	return -1;
}

static void*
__dxa_run_dxt_thread(void *p)
{
	char *log_header = "__dxa_run_dxt_thread";
	struct dxt_interface *dxt_p = (struct dxt_interface *)p;

	nid_log_warning("%s: start ...", log_header);
	dxt_p->dx_op->dx_do_channel(dxt_p);

	nid_log_warning("%s: dxt for dxa terminate ...", log_header);

	return NULL;
}

static void
__dxa_recover_dxt(struct dxa_interface *dxa_p) {
	char *log_header = "__dxa_recover_dxt";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct dxt_interface *dxt_p = priv_p->p_dxt;
	struct dxt_status peer_dxt_stat;

	assert(dxt_p);

do_it:
	if (priv_p->p_csfd >= 0) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
	}
	if (priv_p->p_dsfd >= 0) {
		close(priv_p->p_dsfd);
		priv_p->p_dsfd = -1;
	}

	while(1) {
		// setup connection with dxp side
		if(__dxa_setup_connection(dxa_p) < 0) {
			nid_log_debug("%s: failed to recover channel: [%s]...", log_header, priv_p->p_uuid);
			sleep(DXT_RETRY_INTERVAL);
			goto do_it;
		}

		// get dxp side dxt runtime info
		if(__dxa_get_dxp_info(dxa_p, &peer_dxt_stat)) {
			nid_log_debug("%s: failed to get passive side %s dxt runtime info", log_header, priv_p->p_uuid);
			sleep(DXT_RETRY_INTERVAL);
			goto do_it;
		}

		// here can sure, dxt exist, function will only return 0/1
		if(__dxa_can_continue(dxa_p, &peer_dxt_stat) == 1) {
			// cannot transmit with stopped point, reset dxt in passive side
			if(__dxa_start_dxp(dxa_p, 1)) {
				nid_log_debug("%s: failed to drop passive side %s dxt instance", log_header, priv_p->p_uuid);
				sleep(DXT_RETRY_INTERVAL);
				goto do_it;
			}

			// reset dxt
			dxt_p->dx_op->dx_reset_all(dxt_p);
		} else {
			if(__dxa_start_dxp(dxa_p, 0)) {
				nid_log_debug("%s: failed to restart passive side %s dxt instance", log_header, priv_p->p_uuid);
				sleep(DXT_RETRY_INTERVAL);
				goto do_it;
			}

			// TODO: it almost impossible to adjust re-transmit start point by re-construct
			// dxt send/recv queue

			// success get control and data channel, recover dxt
			dxt_p->dx_op->dx_recover(dxt_p, priv_p->p_dsfd, priv_p->p_csfd);
			break;
		}
	}

	// re-initialize timer for keepalive action
	priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	priv_p->p_ka_xtimer = NID_KEEPALIVE_XTIME;

	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_to_recover = 0;
	priv_p->p_begin_recover = 0;
	pthread_mutex_unlock(&priv_p->p_lck);
}

static int
__dxa_do_housekeeping(void *data)
{
	char *log_header = "dxa_do_housekeeping";
	struct dxa_interface *dxa_p = (struct dxa_interface *)data;
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	int rc = 0, need_recover;

	nid_log_warning("%s: start for dxa: [%s]...", log_header, priv_p->p_uuid);

	pthread_mutex_lock(&priv_p->p_lck);
	if (priv_p->p_to_recover) {
		// make sure __dxa_recover_dxt will not trigger multiple times by housekeeping
		need_recover = !priv_p->p_begin_recover;
		if(need_recover)
			priv_p->p_begin_recover = 1;
		pthread_mutex_unlock(&priv_p->p_lck);
		if(need_recover) {
			nid_log_debug("%s: start recover dxt %s", log_header, priv_p->p_dxt_name);
			__dxa_recover_dxt(dxa_p);
		} else {
			nid_log_debug("%s: dxt %s is in recovering progress", log_header, priv_p->p_dxt_name);
		}
		return 0;
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (priv_p->p_ka_rtimer)
		priv_p->p_ka_rtimer--;

	if (!(priv_p->p_ka_rtimer)) {
		nid_log_notice("%s: timeout %s", log_header, priv_p->p_ip);

		// mark p_to_recover flag
		dxa_mark_recover_dxt(dxa_p);

		return rc;
	}

	priv_p->p_ka_xtimer--;
	if (!priv_p->p_ka_xtimer) {
		/* do keepalive message */
		// TODO: two zero need to change to wanted value
		dxa_keepalive_channel(dxa_p);
		priv_p->p_ka_xtimer = NID_KEEPALIVE_XTIME;
	}

	return rc;
}

static int
dxa_housekeeping(struct dxa_interface *dxa_p)
{
	char *log_header = "dxa_housekeeping";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	static uint32_t run_flag = 0;

	nid_log_debug("%s: start for channel: %s ...", log_header, priv_p->p_uuid);
	if(run_flag) {
		nid_log_debug("%s: housekeeping is in progress for channel: %s ...", log_header, priv_p->p_uuid);
		return 1;
	} else {
		__sync_add_and_fetch(&run_flag, 1);
	}
	if(priv_p->p_dxt) {
		__dxa_do_housekeeping(dxa_p);
	}

	__sync_sub_and_fetch(&run_flag, 1);

	return 1;
}


static struct dxt_interface *
dxa_get_dxt(struct dxa_interface *dxa_p)
{
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	return priv_p->p_dxt;
}

static int
__dxa_setup_connection(struct dxa_interface *dxa_p) {
	char *log_header = "__dxa_setup_connection";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_dxp_create_channel dxp_cc_msg;
	struct umessage_tx_hdr *dxp_msghdr = (struct umessage_tx_hdr *)&dxp_cc_msg;
	uint32_t cmd_len;
	int rc, nwrite, nread, csfd = -1, dsfd = -1;
	char msg_buf[1024];

	nid_log_warning("%s: start ...", log_header);

	csfd = dxa_p->dx_op->dx_make_control_connection(dxa_p);
	if(csfd < 0) {
		nid_log_error("%s: failed to connect control channel from dxp", log_header);
		goto error_lbl;
	}
	/*
	 * send create_channel message to the passive site
	 */
	dxp_msghdr->um_req = UMSG_DXP_CMD_CREATE_CHANNEL;
	dxp_msghdr->um_req_code = UMSG_DXP_CODE_UUID;
	dxp_cc_msg.um_chan_uuid_len = strlen(priv_p->p_peer_uuid);
	strcpy(dxp_cc_msg.um_chan_uuid, priv_p->p_peer_uuid);
	dxp_msghdr->um_len = UMSG_TX_HEADER_LEN +  sizeof(dxp_cc_msg.um_chan_uuid_len) + dxp_cc_msg.um_chan_uuid_len;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_DXP, dxp_msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto error_lbl;
	}
	nid_log_debug("%s: start to send create channel command to dxp side", log_header);
	nwrite = write(csfd, msg_buf, cmd_len);
	if((uint32_t)nwrite != cmd_len) {
		nid_log_error("%s: failed to send create channel command to dxp side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	/*
	 * read response from the passive site
	 */
	nid_log_debug("%s: start to read port from dxp side", log_header);
	nread = util_nw_read_n(csfd, msg_buf, UMSG_TX_HEADER_LEN);
	if(nread != UMSG_TX_HEADER_LEN) {
		nid_log_error("%s: failed to receive data from dxp side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	struct umessage_dxa_create_channel_dport dxa_cc_msg, *dxa_cc_msg_p = &dxa_cc_msg;
	dxp_msghdr = (struct umessage_tx_hdr *)dxa_cc_msg_p;

	// decode message header
	uint32_t dlen = (uint32_t)nread;
	decode_tx_hdr(msg_buf, &dlen, (struct umessage_tx_hdr *)dxp_msghdr);

	assert(dxp_msghdr->um_len <= 4096);
	assert(dxp_msghdr->um_len >= UMSG_TX_HEADER_LEN);
	if (cmd_len > UMSG_TX_HEADER_LEN)
		util_nw_read_n(csfd, msg_buf + UMSG_TX_HEADER_LEN, dxp_msghdr->um_len - UMSG_TX_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, nread, NID_CTYPE_DXA, (void *)dxa_cc_msg_p);
	if (rc) {
		nid_log_error("%s: cannot decode", log_header);
		goto error_lbl;
	}

	if(dxa_cc_msg_p->um_header.um_req == UMSG_DXA_CMD_CREATE_CHANNEL &&
	   dxa_cc_msg_p->um_header.um_req_code == UMSG_DXA_CODE_DPORT &&
	   dxa_cc_msg_p->um_header.um_type == UMSG_TYPE_ACK) {
		u_short dport = dxa_cc_msg_p->um_dport;
		nid_log_debug("%s: port is %d", log_header, dport);
		dsfd = dxa_p->dx_op->dx_make_data_connection(dxa_p, dport);
		if(dsfd < 0) {
			nid_log_error("%s: failed to connect data channel port %d from dxp", log_header, dport);
			goto error_lbl;
		}
	} else {
		nid_log_error("%s: got wrong cmd from dxp side", log_header);
		goto error_lbl;
	}

	nid_log_debug("%s: success connect to dxp side", log_header);
	priv_p->p_csfd = csfd;;
	priv_p->p_dsfd = dsfd;

	return 0;

error_lbl:
	if(csfd >= 0) {
		close(csfd);
	}
	if(dsfd >= 0) {
		close(dsfd);
	}
	return -1;
}

static int
dxa_start(struct dxa_interface *dxa_p)
{
	char *log_header = "dxa_start";
	struct dxa_private *priv_p = (struct dxa_private *)dxa_p->dx_private;
	struct dxt_interface *dxt_p;
	struct dxtg_interface *dxtg_p = priv_p->p_dxtg;
	struct dxt_status dxt_stat;

	nid_log_warning("%s: start ...", log_header);

	// setup connection with dxp side
	if(__dxa_setup_connection(dxa_p) < 0) {
		goto error_lbl;
	}

	// get dxp side dxt runtime info
	if(__dxa_get_dxp_info(dxa_p, &dxt_stat)) {
		goto error_lbl;
	}

	// make dxp side start to work, drop data in dxp side
	if(__dxa_start_dxp(dxa_p, 1)) {
		goto error_lbl;
	}

	// create dxt and start it
	dxt_p = dxtg_p->dxg_op->dxg_search_and_create_channel(dxtg_p, priv_p->p_dxt_name, priv_p->p_csfd, priv_p->p_dsfd);
	if(dxt_p) {
		nid_log_debug("%s: success create dxt %s for dxa", log_header, priv_p->p_dxt_name);
		priv_p->p_dxt = dxt_p;
		dxt_p->dx_op->dx_bind_dxa(dxt_p, dxa_p, __dxa_dxt_callback);

		// start dxt in a seperate
		pthread_attr_t attr;
		pthread_t thread_id;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, __dxa_run_dxt_thread, (void *)dxt_p);

		return 0;
	}

	nid_log_error("%s: can not create dxt interface", log_header);

error_lbl:
	if(priv_p->p_csfd > 0) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
	}
	if(priv_p->p_dsfd > 0) {
		close(priv_p->p_dsfd);
		priv_p->p_dsfd = -1;
	}

	return -1;
}


struct dxa_operations dxa_op = {
	.dx_make_control_connection = dxa_make_control_connection,
	.dx_make_data_connection = dxa_make_data_connection,
	.dx_do_channel = dxa_do_channel,
	.dx_cleanup = dxa_cleanup,
	.dx_housekeeping = dxa_housekeeping,
	.dx_get_server_ip = dxa_get_server_ip,
	.dx_get_chan_uuid = dxa_get_chan_uuid,
	.dx_get_dxt = dxa_get_dxt,
	.dx_mark_recover_dxt = dxa_mark_recover_dxt,
	.dx_start = dxa_start,
};

int
dxa_initialization(struct dxa_interface *dxa_p, struct dxa_setup *setup)
{
	char *log_header = "dxa_initialization";
	struct dxa_private *priv_p;

	nid_log_warning("%s: start ...", log_header);

	dxa_p->dx_op = &dxa_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dxa_p->dx_private = priv_p;

	priv_p->p_dxag = setup->dxag;
	priv_p->p_dxtg = setup->dxtg;
	priv_p->p_handle = setup->handle;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_dxt = NULL;
	priv_p->p_csfd = -1;
	priv_p->p_dsfd = -1;
	priv_p->p_to_recover = 0;
	priv_p->p_begin_recover = 0;
	priv_p->p_ka_xtimer = NID_KEEPALIVE_XTIME;
	priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	strcpy(priv_p->p_ip, setup->ip);
	strcpy(priv_p->p_uuid, setup->uuid);
	strcpy(priv_p->p_peer_uuid, setup->peer_uuid);
	strcpy(priv_p->p_dxt_name, setup->dxt_name);

	pthread_mutex_init(&priv_p->p_lck, NULL);

	return 0;
}
