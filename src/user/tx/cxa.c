/*
 * cxa.c
 * 	Implementation of Control Exchange Active Module
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "util_nw.h"
#include "nid.h"
#include "nid_log.h"
#include "cxt_if.h"
#include "cxtg_if.h"
#include "cxa_if.h"
#include "cxag_if.h"
#include "umpk_if.h"
#include "umpk_cxa_if.h"
#include "umpk_cxp_if.h"
#include "lstn_if.h"

#define CXA_RW_TIMEOUT	10

struct cxa_private {
	struct ds_interface	*p_ds;
	struct cxt_interface	*p_cxt;
	struct cxtg_interface	*p_cxtg;
	char			p_cxt_name[NID_MAX_UUID];
	char			p_ip[NID_MAX_IP];
	int			p_csfd;	// control socket fd
	void			*p_handle;
	int			p_ka_xtimer;    // ka: send keepalive when decreased to 0
	int			p_ka_rtimer;    // timeout when decreased to 0
	char			p_to_recover;
	char			p_begin_recover;
	char			p_need_start;
	struct umpk_interface	*p_umpk;
	struct cxag_interface	*p_cxag;
	pthread_mutex_t		p_lck;
	char			p_uuid[NID_MAX_UUID];
	char			p_peer_uuid[NID_MAX_UUID];
};

static int
cxa_make_control_connection(struct cxa_interface *cxa_p)
{
	char *log_header = "control_connection";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	uint8_t chan_type = NID_CTYPE_CXP;	// I'm the cxa, talking to cxp
	int nwrite;

	nid_log_warning("%s: ...start", log_header);
	priv_p->p_csfd = util_nw_make_connection(priv_p->p_ip, NID_SERVER_PORT);
	nid_log_debug("%s: ip is %s", log_header, priv_p->p_ip);
	if (priv_p->p_csfd < 0) {
		nid_log_error("%s: failed to connect (%s)",
			log_header, priv_p->p_ip);
		return -1;
	}

	nwrite = util_nw_write_timeout(priv_p->p_csfd, (char *)&chan_type, 1, CXA_RW_TIMEOUT);
	if (nwrite != 1) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
		nid_log_error("%s: failed to send out channel type, errno:%d", log_header, errno);
		return -1;
	}

	return priv_p->p_csfd;
}

static void
__cxa_cxt_callback(void *p1, void *p2)
{
	struct cxa_interface *cxa_p = (struct cxa_interface *)p1;
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct umessage_tx_hdr *msg_hdr =  (struct umessage_tx_hdr *)p2;

	if(msg_hdr->um_req == UMSG_CXP_CMD_KEEPALIVE) {
		// reset keepalive recieve timeout counter
		priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	}

	return;
}

static void
cxa_do_channel(struct cxa_interface *cxa_p)
{
	char *log_header = "cxa_do_channel";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct cxt_interface *cxt_p;
	struct cxtg_interface *cxtg_p = priv_p->p_cxtg;

	nid_log_warning("%s: start ...", log_header);
	cxt_p = calloc(1, sizeof(*cxt_p));
	cxt_p = cxtg_p->cxg_op->cxg_search_and_create_channel(cxtg_p, priv_p->p_cxt_name, priv_p->p_csfd );

	if(!cxt_p){
		nid_log_error("%s: can not create cxt interface", log_header);
		return;
	}
	priv_p->p_cxt = cxt_p;
	cxt_p->cx_op->cx_bind_cxa(cxt_p, cxa_p, __cxa_cxt_callback);
	cxt_p->cx_op->cx_do_channel(cxt_p);
}

static void *
cxa_get_handle(struct cxa_interface *cxa_p)
{
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	return priv_p->p_handle;
}

static struct cxt_interface *
cxa_get_cxt(struct cxa_interface *cxa_p)
{
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	return priv_p->p_cxt;
}

static char*
cxa_get_server_ip(struct cxa_interface *cxa_p)
{
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;

	return priv_p->p_ip;
}

static char*
cxa_get_chan_uuid(struct cxa_interface *cxa_p)
{
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;

	return priv_p->p_uuid;
}


static void
cxa_cleanup(struct cxa_interface *cxa_p) {
	char *log_header = "cxa_cleanup";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct cxtg_interface *cxtg_p = priv_p->p_cxtg;
	nid_log_warning("%s: start ...", log_header);

	cxtg_p->cxg_op->cxg_drop_channel(cxtg_p, priv_p->p_cxt);
	if(priv_p->p_csfd != 0 && priv_p->p_csfd != -1) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
	}
	pthread_mutex_destroy(&priv_p->p_lck);
	free(priv_p);
}

static int
cxa_keepalive_channel(struct cxa_interface *cxa_p) {
	char *log_header = "cxa_keepalive_channel";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct cxt_interface *cxt_p = priv_p->p_cxt;
	struct umessage_tx_hdr req;
	char msg_buf[4096], *send_buf;
	int rc;
	uint32_t cmd_len;

	nid_log_debug("%s: start ...", log_header);

	/*
	 * send create_channel message to the passive site
	 */
	memset(&req, 0, sizeof(req));
	req.um_req = UMSG_CXP_CMD_KEEPALIVE;
	req.um_len = sizeof(req);
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_CXP, &req);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto out;
	}

	send_buf = cxt_p->cx_op->cx_get_buffer(cxt_p, cmd_len);
	memcpy(send_buf, msg_buf, cmd_len);
	cxt_p->cx_op->cx_send(cxt_p, send_buf, cmd_len);

out:
	return rc;
}

static int
cxa_drop_channel(struct cxa_interface * cxa_p) {
	char *log_header = "cxa_drop_channel";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct cxt_interface *cxt_p = priv_p->p_cxt;
	struct umessage_tx_hdr req;
	char msg_buf[4096], *send_buf;
	int rc;
	uint32_t cmd_len;

	nid_log_debug("%s: start ...", log_header);

	/*
	 * send drop_channel message to the passive site
	 */
	memset(&req, 0, sizeof(req));
	req.um_req = UMSG_CXP_CMD_DROP_CHANNEL;
	req.um_len = sizeof(req);
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_CXP, &req);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto out;
	}

	send_buf = cxt_p->cx_op->cx_get_buffer(cxt_p, cmd_len);
	memcpy(send_buf, msg_buf, cmd_len);
	cxt_p->cx_op->cx_send(cxt_p, send_buf, cmd_len);

out:
	return rc;
}

static void
cxa_mark_recover_cxt(struct cxa_interface *cxa_p) {
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct cxt_interface *cxt_p = priv_p->p_cxt;

	pthread_mutex_lock(&priv_p->p_lck);
	if(! priv_p->p_to_recover) {
		priv_p->p_to_recover = 1;
		pthread_mutex_unlock(&priv_p->p_lck);
		cxt_p->cx_op->cx_freeze(cxt_p);
		return;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static int __cxa_setup_connection(struct cxa_interface *);

static int
__cxa_can_continue(struct cxa_interface *cxa_p, struct cxt_status *peer_cxt_stat) {
	char *log_header = "__cxa_cxt_continue";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct cxt_status cxt_stat;
	struct cxt_interface *cxt_p = priv_p->p_cxt;

	// make sure cxt initialized
	if(!cxt_p) {
		nid_log_warning("%s: cxt not initialize yet ...", log_header);
		return -1;
	}

	cxt_p->cx_op->cx_get_status(cxt_p, &cxt_stat);

	/*
	 * think it many times, it's better to drop continue design, because it will add
	 * complex in cxt level to get exactly stopped point, it need point in both active
	 * side and passive side (for data from passive side to active side)
	 *
	 */
	// TODO: consider reliable method to check if can transmit from stopped point
	if(cxt_stat.ack_seq && cxt_stat.ack_seq <= peer_cxt_stat->recv_seq &&
		peer_cxt_stat->ack_seq && cxt_stat.recv_seq >= peer_cxt_stat->ack_seq) {
		return 0;
	}

	return 1;
}

static int
__cxa_get_cxp_info(struct cxa_interface *cxa_p, struct cxt_status *cxt_stat_p)
{
	char *log_header = "__cxa_get_cxp_info";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_cxp_stat_channel cxp_sc_msg;
	struct umessage_tx_hdr *cxp_msghdr = (struct umessage_tx_hdr *)&cxp_sc_msg;
	uint32_t cmd_len;
	int rc, nwrite, nread;
	char msg_buf[1024];

	nid_log_warning("%s: start ...", log_header);

	/*
	 * send get channel state message to the passive site
	 */
	memset(&cxp_sc_msg, 0, sizeof(cxp_sc_msg));
	cxp_msghdr->um_req = UMSG_CXP_CMD_STAT_CHANNEL;
	cxp_msghdr->um_req_code = UMSG_CXP_CODE_NO;
	cxp_sc_msg.um_chan_uuid_len = strlen(priv_p->p_peer_uuid);
	strcpy(cxp_sc_msg.um_chan_uuid, priv_p->p_peer_uuid);
	cxp_msghdr->um_len = UMSG_TX_HEADER_LEN +  sizeof(cxp_sc_msg.um_chan_uuid_len) + cxp_sc_msg.um_chan_uuid_len;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_CXP, cxp_msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto error_lbl;
	}
	nwrite = util_nw_write_n(priv_p->p_csfd, msg_buf, cmd_len);
	if((uint32_t)nwrite != cmd_len) {
		nid_log_error("%s: failed to send state channel command to cxp side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	/*
	 * read response from the passive site
	 */
	nread = util_nw_read_n(priv_p->p_csfd, msg_buf, UMSG_TX_HEADER_LEN);
	if(nread != UMSG_TX_HEADER_LEN) {
		nid_log_error("%s: failed to receive data from cxp side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	struct umessage_cxp_stat_channel_resp cxp_sc_resp_msg, *cxp_sc_resp_msg_p = &cxp_sc_resp_msg;
	cxp_msghdr = (struct umessage_tx_hdr *)cxp_sc_resp_msg_p;
	memset(cxp_sc_resp_msg_p, 0, sizeof(*cxp_sc_resp_msg_p));

	// decode message header
	uint32_t dlen = (uint32_t)nread;
	decode_tx_hdr(msg_buf, &dlen, (struct umessage_tx_hdr *)cxp_msghdr);

	assert(cxp_msghdr->um_len <= 4096);
	assert(cxp_msghdr->um_len >= UMSG_TX_HEADER_LEN);
	if (cmd_len > UMSG_TX_HEADER_LEN)
		util_nw_read_n(priv_p->p_csfd, msg_buf + UMSG_TX_HEADER_LEN, cxp_msghdr->um_len - UMSG_TX_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cxp_msghdr->um_len, NID_CTYPE_CXP, (void *)cxp_sc_resp_msg_p);
	if (rc) {
		nid_log_error("%s: cannot decode", log_header);
		goto error_lbl;
	}

	if(cxp_sc_resp_msg_p->um_header.um_req == UMSG_CXP_CMD_STAT_CHANNEL &&
			cxp_sc_resp_msg_p->um_header.um_req_code == UMSG_CXP_CODE_STAT_RESP) {
		cxt_stat_p->send_seq = cxp_sc_resp_msg_p->send_seq;
		cxt_stat_p->ack_seq = cxp_sc_resp_msg_p->ack_seq;
		cxt_stat_p->recv_seq = cxp_sc_resp_msg_p->recv_seq;
		cxt_stat_p->cmsg_left = cxp_sc_resp_msg_p->cmsg_left;
	} else {
		nid_log_error("%s: got wrong cmd from cxp side", log_header);
		goto error_lbl;
	}

	nid_log_warning("%s: done ...", log_header);

	return 0;

error_lbl:

	return -1;
}

static int
__cxa_start_cxp(struct cxa_interface *cxa_p, int drop_it)
{
	char *log_header = "__cxa_start_cxp";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_cxp_start_work cxp_sw_msg, *cxp_sw_msg_p = &cxp_sw_msg;
	struct umessage_cxp_start_work_resp cxp_sw_msg_resp, *cxp_sw_msg_resp_p = &cxp_sw_msg_resp;
	struct umessage_tx_hdr *msghdr = (struct umessage_tx_hdr *)cxp_sw_msg_p;
	uint32_t cmd_len;
	int rc, nwrite, nread;
	char msg_buf[4096];

	nid_log_warning("%s: start ...", log_header);

	/*
	 * send get channel state message to the passive site
	 */
	memset(cxp_sw_msg_p, 0, sizeof(*cxp_sw_msg_p));
	cxp_sw_msg_p->um_chan_uuid_len = strlen(priv_p->p_peer_uuid);
	strcpy(cxp_sw_msg_p->um_chan_uuid, priv_p->p_peer_uuid);
	msghdr->um_req = UMSG_CXP_CMD_START_WORK;
	msghdr->um_len = UMSG_TX_HEADER_LEN +  sizeof(cxp_sw_msg_p->um_chan_uuid_len) + cxp_sw_msg_p->um_chan_uuid_len;
	// if cxp side have data in cxt side, notify cxp destroy cxt and create it again
	if(drop_it) {
		// passive side have data in cxt module, notify passive side drop data then to work
		// because here is start, not recovery, we have no data in hand, the simplest method
		// to work it let passive side drop all data in its hand
		msghdr->um_req_code = UMSG_CXP_CODE_DROP_ALL;
	}

	memset(msg_buf, 0, sizeof(msg_buf));
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_CXP, msghdr);
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

	memset(&cxp_sw_msg_resp, 0, sizeof(cxp_sw_msg_resp));
	msghdr = (struct umessage_tx_hdr *)cxp_sw_msg_resp_p;

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
	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, msghdr->um_len, NID_CTYPE_CXP, cxp_sw_msg_resp_p);
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
__cxa_run_cxt_thread(void *p)
{
	char *log_header = "__cxa_run_cxt_thread";
	struct cxt_interface *cxt_p = (struct cxt_interface *)p;

	nid_log_warning("%s: start ...", log_header);
	cxt_p->cx_op->cx_do_channel(cxt_p);

	nid_log_warning("%s: cxt for cxa terminate ...", log_header);

	return NULL;
}

static void
__cxa_recover_cxt(struct cxa_interface *cxa_p) {
	char *log_header = "__cxa_recover_cxt";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct cxt_interface *cxt_p = priv_p->p_cxt;
	struct cxt_status peer_cxt_stat;

	assert(cxt_p);

do_it:
	if (priv_p->p_csfd >= 0) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
	}

	while(1) {
		// setup connection with cxp side
		if(__cxa_setup_connection(cxa_p) < 0) {
			nid_log_debug("%s: failed to recover channel: [%s]...", log_header, priv_p->p_uuid);
			sleep(CXT_RETRY_INTERVAL);
			goto do_it;
		}

		// get cxp side cxt runtime info
		if(__cxa_get_cxp_info(cxa_p, &peer_cxt_stat)) {
			nid_log_debug("%s: failed to get passive side %s cxt runtime info", log_header, priv_p->p_uuid);
			sleep(CXT_RETRY_INTERVAL);
			goto do_it;
		}

		// here can sure, cxt exist, function will only return 0/1
		if(__cxa_can_continue(cxa_p, &peer_cxt_stat) == 1) {
			// cannot transmit with stopped point, reset cxt in passive side
			if(__cxa_start_cxp(cxa_p, 1)) {
				nid_log_debug("%s: failed to drop passive side %s cxt instance", log_header, priv_p->p_uuid);
				sleep(CXT_RETRY_INTERVAL);
				goto do_it;
			}

			// reset cxt
			cxt_p->cx_op->cx_reset_all(cxt_p);
		} else {
			if(__cxa_start_cxp(cxa_p, 0)) {
				nid_log_debug("%s: failed to restart passive side %s cxt instance", log_header, priv_p->p_uuid);
				sleep(CXT_RETRY_INTERVAL);
				goto do_it;
			}

			// TODO: it almost impossible to adjust re-transmit start point by re-construct
			// cxt send/recv queue

			// success get control channel, recover cxt
			cxt_p->cx_op->cx_recover(cxt_p, priv_p->p_csfd);
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
__cxa_setup_connection(struct cxa_interface *cxa_p) {
	char *log_header = "__cxa_setup_connection";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_cxp_create_channel cxp_cc_msg;
	struct umessage_tx_hdr *cxp_msghdr = (struct umessage_tx_hdr *)&cxp_cc_msg;
	uint32_t cmd_len;
	int rc, nwrite, csfd = -1;
	char msg_buf[1024];

	nid_log_warning("%s: start ...", log_header);

	csfd = cxa_p->cx_op->cx_make_control_connection(cxa_p);
	if(csfd < 0) {
		nid_log_error("%s: failed to connect control channel from cxp", log_header);
		goto error_lbl;
	}
	/*
	 * send create_channel message to the passive site
	 */
	cxp_msghdr->um_req = UMSG_CXP_CMD_CREATE_CHANNEL;
	cxp_msghdr->um_req_code = UMSG_CXP_CODE_UUID;
	cxp_cc_msg.um_chan_uuid_len = strlen(priv_p->p_peer_uuid);
	strcpy(cxp_cc_msg.um_chan_uuid, priv_p->p_peer_uuid);
	cxp_msghdr->um_len = UMSG_TX_HEADER_LEN +  sizeof(cxp_cc_msg.um_chan_uuid_len) + cxp_cc_msg.um_chan_uuid_len;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, NID_CTYPE_CXP, cxp_msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto error_lbl;
	}
	nid_log_debug("%s: start to send create channel command to cxp side", log_header);
	nwrite = write(csfd, msg_buf, cmd_len);
	if((uint32_t)nwrite != cmd_len) {
		nid_log_error("%s: failed to send create channel command to cxp side, errno:%d",
				log_header, errno);
		goto error_lbl;
	}

	nid_log_debug("%s: success connect to cxp side", log_header);
	priv_p->p_csfd = csfd;

	return 0;

error_lbl:
	if(csfd >= 0) {
		close(csfd);
	}

	return -1;
}


static int
__cxa_do_housekeeping(void *data)
{
	char *log_header = "cxa_do_housekeeping";
	struct cxa_interface *cxa_p = (struct cxa_interface *)data;
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	int rc = 0, need_recover;

	nid_log_debug("%s: start %s...", log_header, priv_p->p_ip);

	pthread_mutex_lock(&priv_p->p_lck);
	if (priv_p->p_to_recover) {
		// make sure __cxa_recover_cxt will not trigger multiple times by housekeeping
		need_recover = !priv_p->p_begin_recover;
		if(need_recover)
			priv_p->p_begin_recover = 1;
		pthread_mutex_unlock(&priv_p->p_lck);
		if(need_recover) {
			nid_log_debug("%s: start recover cxt %s", log_header, priv_p->p_cxt_name);
			__cxa_recover_cxt(cxa_p);
		} else {
			nid_log_debug("%s: cxt %s is in recovering progress", log_header, priv_p->p_cxt_name);
		}
		return 0;
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (priv_p->p_ka_rtimer)
		priv_p->p_ka_rtimer--;

	if (!(priv_p->p_ka_rtimer)) {
		nid_log_notice("%s: timeout %s", log_header, priv_p->p_ip);

		// mark p_to_recover flag
		cxa_mark_recover_cxt(cxa_p);

		return rc;
	}

	priv_p->p_ka_xtimer--;
	if (!priv_p->p_ka_xtimer) {
		/* do keepalive message */
		// TODO: two zero need to change to wanted value
		cxa_keepalive_channel(cxa_p);
		priv_p->p_ka_xtimer = NID_KEEPALIVE_XTIME;
	}
	return rc;
}


static int
cxa_housekeeping(struct cxa_interface *cxa_p)
{
	char *log_header = "cxa_housekeeping";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	static uint32_t run_flag = 0;

	nid_log_debug("%s: start for channel: %s ...", log_header, priv_p->p_uuid);
	if(run_flag) {
		nid_log_debug("%s: housekeeping is in progress for channel: %s ...", log_header, priv_p->p_uuid);
		return 1;
	} else {
		__sync_add_and_fetch(&run_flag, 1);
	}

	if(priv_p->p_cxt) {
		__cxa_do_housekeeping(cxa_p);
	}

	__sync_sub_and_fetch(&run_flag, 1);

	return 0;
}

static int
cxa_start(struct cxa_interface *cxa_p)
{
	char *log_header = "cxa_start";
	struct cxa_private *priv_p = (struct cxa_private *)cxa_p->cx_private;
	struct cxt_interface *cxt_p;
	struct cxtg_interface *cxtg_p = priv_p->p_cxtg;
	struct cxt_status cxt_stat;

	nid_log_warning("%s: start ...", log_header);

	// setup connection with cxp side
	if(__cxa_setup_connection(cxa_p) < 0) {
		goto error_lbl;
	}

	// get cxp side cxt runtime info
	if(__cxa_get_cxp_info(cxa_p, &cxt_stat)) {
		goto error_lbl;
	}

	// make cxp side start to work, drop data in cxp side
	if(__cxa_start_cxp(cxa_p, 1)) {
		goto error_lbl;
	}

	// create cxt and start it
	cxt_p = cxtg_p->cxg_op->cxg_search_and_create_channel(cxtg_p, priv_p->p_cxt_name, priv_p->p_csfd);
	if(cxt_p) {
		nid_log_debug("%s: success create cxt %s for cxa", log_header, priv_p->p_cxt_name);
		priv_p->p_cxt = cxt_p;
		cxt_p->cx_op->cx_bind_cxa(cxt_p, cxa_p, __cxa_cxt_callback);

		// start cxt in a seperate
		pthread_attr_t attr;
		pthread_t thread_id;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, __cxa_run_cxt_thread, (void *)cxt_p);

		return 0;
	}

	nid_log_error("%s: can not create cxt interface", log_header);

error_lbl:
	if(priv_p->p_csfd > 0) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
	}

	return -1;
}


struct cxa_operations cxa_op = {
	.cx_make_control_connection = cxa_make_control_connection,
	.cx_do_channel = cxa_do_channel,
	.cx_get_handle = cxa_get_handle,
	.cx_get_cxt = cxa_get_cxt,
	.cx_cleanup =cxa_cleanup,
	.cx_housekeeping = cxa_housekeeping,
	.cx_keepalive_channel = cxa_keepalive_channel,
	.cx_get_server_ip = cxa_get_server_ip,
	.cx_get_chan_uuid = cxa_get_chan_uuid,
	.cx_mark_recover_cxt = cxa_mark_recover_cxt,
	.cx_drop_channel = cxa_drop_channel,
	.cx_start = cxa_start,
};

int
cxa_initialization(struct cxa_interface *cxa_p, struct cxa_setup *setup)
{
	char *log_header = "cxa_initialization";
	struct cxa_private *priv_p;

	nid_log_warning("%s: start ...", log_header);

	cxa_p->cx_op = &cxa_op;
	priv_p = calloc(1, sizeof(*priv_p));
	cxa_p->cx_private = priv_p;

	priv_p->p_ds = setup->ds;
	priv_p->p_cxtg = setup->cxtg;
	priv_p->p_handle = setup->handle;
	priv_p->p_csfd = -1;
	priv_p->p_cxt = NULL;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_begin_recover = 0;
	priv_p->p_to_recover = 0;
	strcpy(priv_p->p_cxt_name, setup->cxt_name);
	strcpy(priv_p->p_ip, setup->ip);
	strcpy(priv_p->p_uuid, setup->uuid);
	strcpy(priv_p->p_peer_uuid, setup->peer_uuid);

	pthread_mutex_init(&priv_p->p_lck, NULL);

	return 0;
}
