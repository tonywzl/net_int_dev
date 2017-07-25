/*
 * cxp.c
 * 	Implementation of Control Exchange Passive Module
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <pthread.h>

#include "util_nw.h"
#include "nid.h"
#include "nid_log.h"
#include "cxt_if.h"
#include "cxtg_if.h"
#include "cxp_if.h"
#include "cxpg_if.h"
#include "umpk_if.h"
#include "umpk_cxp_if.h"
#include "tx_shared.h"
#include "lstn_if.h"

struct ds_interface;
struct cxp_private {
	struct cxt_interface	*p_cxt;
	struct cxtg_interface	*p_cxtg;
	int			p_csfd;	// control socket fd
	void			*p_handle;
	char			p_cxt_name[NID_MAX_UUID];
	char			p_uuid[NID_MAX_UUID];
	char			p_peer_uuid[NID_MAX_UUID];
	struct umpk_interface	*p_umpk;
	struct cxpg_interface	*p_cxpg;
	uint8_t		p_stop;
	uint8_t		p_to_stop;
	uint8_t		p_keep_cxt;
	pthread_mutex_t		p_lck;
	pthread_cond_t		p_stop_cond;
};

static void *
cxp_get_handle(struct cxp_interface *cxp_p)
{
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	return priv_p->p_handle;
}

static struct cxt_interface *
cxp_get_cxt(struct cxp_interface *cxp_p)
{
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	return priv_p->p_cxt;
}

static char*
cxp_get_chan_uuid(struct cxp_interface *cxp_p)
{
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;

	return priv_p->p_uuid;
}

static void
__cxp_stop(struct cxp_interface *cxp_p)
{
	char *log_header = "cxp_stop";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;

	nid_log_warning("%s: start ...", log_header);

	// make cxp_do_channel thread terminate
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_to_stop = 1;
	pthread_cond_signal(&priv_p->p_stop_cond);
	while (!priv_p->p_stop)
		pthread_cond_wait(&priv_p->p_stop_cond, &priv_p->p_lck);
	pthread_mutex_unlock(&priv_p->p_lck);

	if(priv_p->p_csfd != 0 && priv_p->p_csfd != -1) {
		close(priv_p->p_csfd);
		priv_p->p_csfd = -1;
	}

	nid_log_warning("%s: done ...", log_header);
}


static void
__cxp_cxt_callback(void *p1, void *p2)
{
	char *log_header = "__cxp_cxt_callback";
	struct cxp_interface *cxp_p = (struct cxp_interface *)p1;
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct cxt_interface *cxt_p = priv_p->p_cxt;
	struct umessage_tx_hdr *msg_hdr =  (struct umessage_tx_hdr *)p2;
	uint32_t cmd_len = sizeof(struct umessage_tx_hdr);
	struct umessage_tx_hdr *ack_msg_hdr;
	char *send_buf;

	switch(msg_hdr->um_req) {
	case UMSG_CXP_CMD_KEEPALIVE:
		// send back ack to cxa
		send_buf = cxt_p->cx_op->cx_get_buffer(cxt_p, cmd_len);
		memcpy(send_buf, msg_hdr, sizeof(struct umessage_tx_hdr));
		ack_msg_hdr = (struct umessage_tx_hdr *)send_buf;
		ack_msg_hdr->um_type = UMSG_TYPE_ACK;
		cxt_p->cx_op->cx_send(cxt_p, send_buf, cmd_len);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msg_hdr->um_req);
		break;
	}

	// Notice: there have no seq update call, because cxp is passive mode
	// TODO: if we need it later, ref code in _cxa_cxt_callback part

	return;
}


static void*
__cxp_run_cxt_thread(void *p)
{
	char *log_header = "__cxp_run_cxt_thread";
	struct cxt_interface *cxt_p = (struct cxt_interface *)p;

	nid_log_warning("%s: start ...", log_header);
	cxt_p->cx_op->cx_do_channel(cxt_p);

	return NULL;
}

static int
cxp_start(struct cxp_interface *cxp_p)
{
	char *log_header = "cxp_start";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct cxt_interface *cxt_p;
	struct cxtg_interface *cxtg_p = priv_p->p_cxtg;

	nid_log_warning("%s: start ...", log_header);
	cxt_p = cxtg_p->cxg_op->cxg_search_and_create_channel(cxtg_p, priv_p->p_cxt_name, priv_p->p_csfd );
	if(cxt_p) {
		priv_p->p_cxt = cxt_p;
		cxt_p->cx_op->cx_bind_cxp(cxt_p, cxp_p, __cxp_cxt_callback);

		// start cxt in a seperate
		pthread_attr_t attr;
		pthread_t thread_id;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, __cxp_run_cxt_thread, (void *)cxt_p);

		return 0;
	} else {
		nid_log_error("%s: can not create cxt interface", log_header);
	}

	return -1;
}

static int
__cxp_start(struct cxp_interface *cxp_p)
{
	char *log_header = "__cxp_start";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct cxt_interface *cxt_p;
	struct cxtg_interface *cxtg_p = priv_p->p_cxtg;

	nid_log_warning("%s: start ...", log_header);

	nid_log_warning("%s: create cxt ...", log_header);
	cxt_p = cxtg_p->cxg_op->cxg_search_and_create_channel(cxtg_p, priv_p->p_cxt_name, priv_p->p_csfd);
	if(!cxt_p) {
		nid_log_error("%s: can not create cxt interface", log_header);
		return -1;
	}
	priv_p->p_cxt = cxt_p;

	if(cxt_p->cx_op->cx_is_new(cxt_p)) {
		nid_log_warning("%s: run cxt ...", log_header);
		cxt_p->cx_op->cx_bind_cxp(cxt_p, cxp_p, __cxp_cxt_callback);

		// start cxt in a seperate thread
		pthread_attr_t attr;
		pthread_t thread_id;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, __cxp_run_cxt_thread, (void *)cxt_p);
	} else {
		nid_log_warning("%s: cxt recover ...", log_header);
		// cxt already exist in system, only need to wake it up
		cxt_p->cx_op->cx_recover(cxt_p, priv_p->p_csfd);
	}

	nid_log_warning("%s: done ...", log_header);

	return 0;
}


static int
__cxp_get_cxt_stat(struct cxp_interface *cxp_p, struct cxt_status *stat_p)
{
	char *log_header = "__cxp_get_cxt_stat";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct cxt_interface *cxt_p = priv_p->p_cxt;

	assert(stat_p);

	// make sure cxt initialized
	if(!cxt_p) {
		nid_log_warning("%s: cxt not initialize yet ...", log_header);
		return -1;
	}

	cxt_p->cx_op->cx_get_status(cxt_p, stat_p);
	return 0;
}


static int
__cxp_stat_channel(struct cxp_interface *cxp_p) {
	char *log_header = "__cxp_stat_channel";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct cxt_status cxt_stat, *stat_p = &cxt_stat;
	struct umessage_cxp_stat_channel stat_msg;
	struct umessage_tx_hdr *msghdr = (struct umessage_tx_hdr *)&stat_msg;
	int csfd = priv_p->p_csfd;
	char msg_buf[4096];
	int rc, nread, nwrite;
	uint32_t dlen;

	nid_log_warning("%s: start ...", log_header);

	nread = util_nw_read_n(csfd, msg_buf, UMSG_TX_HEADER_LEN);
	if(nread != UMSG_TX_HEADER_LEN) {
		nid_log_error("%s: failed to read request message, return %d", log_header, nread);
		return -1;
	}

	// decode message header
	dlen = (uint32_t)nread;
	decode_tx_hdr(msg_buf, &dlen, msghdr);
	if(msghdr->um_req != UMSG_CXP_CMD_STAT_CHANNEL) {
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
		return -1;
	}

	assert(msghdr->um_len <= 4096);
	assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
	dlen = msghdr->um_len - UMSG_TX_HEADER_LEN;
	if (dlen > 0) {
		nread = util_nw_read_n(priv_p->p_csfd, msg_buf + UMSG_TX_HEADER_LEN, dlen);
		if((uint32_t)nread != dlen) {
			nid_log_error("%s: failed to receive data from passive side, errno:%d",
					log_header, errno);
			return -1;
		}
	}

	// don't need to decode umpk, ignore it
	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, msghdr->um_len, NID_CTYPE_CXP, (void *)msghdr);
	if (rc) {
		nid_log_error("%s: cannot decode", log_header);
		return -1;
	}

	struct umessage_cxp_stat_channel_resp cxp_sc_resp_msg, *cxp_sc_resp_msg_p = &cxp_sc_resp_msg;

	// send back cxt runtime info to cxa side
	__cxp_get_cxt_stat(cxp_p, stat_p);
	cxp_sc_resp_msg_p->ack_seq = stat_p->ack_seq;
	cxp_sc_resp_msg_p->cmsg_left = stat_p->cmsg_left;
	cxp_sc_resp_msg_p->recv_seq = stat_p->recv_seq;
	cxp_sc_resp_msg_p->send_seq = stat_p->send_seq;

	memcpy(cxp_sc_resp_msg_p, &stat_msg, sizeof(stat_msg));
	cxp_sc_resp_msg_p->um_header.um_req_code = UMSG_CXP_CODE_STAT_RESP;
	msghdr = (struct umessage_tx_hdr *)cxp_sc_resp_msg_p;

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &dlen, NID_CTYPE_CXP, msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto out;
	}
	nid_log_debug("%s: start to send stat channel response message to cxa side", log_header);
	nwrite = util_nw_write_n(csfd, msg_buf, dlen);
	if((uint32_t)nwrite != dlen) {
		rc = -1;
		nid_log_error("%s: failed to send stat channel response message to cxa side, errno:%d",
				log_header, errno);
		goto out;
	}

	nid_log_warning("%s: done ...", log_header);

out:
	return rc;
}

static int
__cxp_check_start_work_msg(struct cxp_interface *cxp_p) {
	char *log_header = "__cxp_check_start_work_msg";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct cxt_interface *cxt_p;
	struct umessage_tx_hdr *msghdr;
	struct umessage_cxp_start_work cxp_sw_msg, *cxp_sw_msg_p = &cxp_sw_msg;
	struct umessage_cxp_start_work_resp cxp_sw_resp_msg, *cxp_sw_resp_msg_p = &cxp_sw_resp_msg;
	int csfd = priv_p->p_csfd;
	char msg_buf[1024];
	int rc, nread, nwrite;
	uint32_t dlen;

	nid_log_warning("%s: start ...", log_header);

	memset(msg_buf, 0, sizeof(msg_buf));
	nread = util_nw_read_n(csfd, msg_buf, UMSG_TX_HEADER_LEN);
	if(nread != UMSG_TX_HEADER_LEN) {
		nid_log_error("%s: failed to read request message, return %d", log_header, nread);
		return -1;
	}

	// decode message header
	dlen = (uint32_t)nread;
	msghdr = (struct umessage_tx_hdr *)cxp_sw_msg_p;
	memset(cxp_sw_msg_p, 0, sizeof(*cxp_sw_msg_p));
	decode_tx_hdr(msg_buf, &dlen, msghdr);

	if(msghdr->um_req != UMSG_CXP_CMD_START_WORK) {
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
		return -1;
	}
	assert(msghdr->um_len <= 4096);
	assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
	dlen = msghdr->um_len - UMSG_TX_HEADER_LEN;
	if (dlen > 0) {
		nread = util_nw_read_n(priv_p->p_csfd, msg_buf + UMSG_TX_HEADER_LEN, dlen);
		if((uint32_t)nread != dlen) {
			nid_log_error("%s: failed to receive data from passive side, errno:%d",
					log_header, errno);
			return -1;
		}
	}

	// decode message
	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, msghdr->um_len, NID_CTYPE_CXP, cxp_sw_msg_p);
	if(rc) {
		nid_log_error("%s: failed to decode umpk message", log_header);
		return -1;
	}

	/*
	 * if drop code, reset dxt module to initial status, don't drop dxt, because user
	 * of dxp may have dxt reference, if here drop dxt, the upper-level user may
	 * trigger core dump at some time
	 *
	 */
	if(msghdr->um_req_code == UMSG_CXP_CODE_DROP_ALL) {
		nid_log_debug("%s: destroy cxt allocated in the past", log_header);
		cxt_p = priv_p->p_cxt;
		if(cxt_p) {
			cxt_p->cx_op->cx_reset_all(cxt_p);
		}
	}

	memset(cxp_sw_resp_msg_p, 0, sizeof(*cxp_sw_resp_msg_p));
	memcpy(cxp_sw_resp_msg_p, cxp_sw_msg_p, sizeof(*cxp_sw_resp_msg_p));
	msghdr = (struct umessage_tx_hdr *)cxp_sw_resp_msg_p;
	// send back response message
	msghdr->um_req_code = UMSG_CXP_START_WORK_RESP;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &dlen, NID_CTYPE_CXP, msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
	} else {
		nwrite = util_nw_write_n(csfd, msg_buf, dlen);
		if((uint32_t)nwrite != dlen) {
			rc = -1;
			nid_log_error("%s: failed to send response message, errno:%d", log_header, errno);
		}
	}

	return rc;
}


/*
 * Algorithm:
 * 	1> create recv/send and send control threads.
 * 	2> send control requests.
 */
static void
cxp_do_channel(struct cxp_interface *cxp_p)
{
	char *log_header = "cxp_do_channel";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;

	nid_log_warning("%s: start ...", log_header);

	// if not success response channel state info to cxa side
	if(__cxp_stat_channel(cxp_p) != 0) {
		nid_log_warning("%s: failed in process channel stat command", log_header);
		goto out;
	}

	// if not success response channel state info to cxa side
	if(__cxp_check_start_work_msg(cxp_p) != 0) {
		nid_log_warning("%s: failed in process channel stat command", log_header);
		goto out;
	}

	if(!priv_p->p_cxt) {
		nid_log_warning("%s: start new cxt...", log_header);
		__cxp_start(cxp_p);
	} else {
		nid_log_warning("%s: recover old cxt...", log_header);
		// if already have cxt, it means it's recovery, call cxt_recover
		struct cxt_interface *cxt_p = priv_p->p_cxt;
		cxt_p->cx_op->cx_recover(cxt_p, priv_p->p_csfd);
	}

	nid_log_warning("%s: pause to wait quit signal ...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);
	while (!priv_p->p_to_stop)
		pthread_cond_wait(&priv_p->p_stop_cond, &priv_p->p_lck);
	priv_p->p_stop = 1;
	pthread_cond_signal(&priv_p->p_stop_cond);
	pthread_mutex_unlock(&priv_p->p_lck);
	nid_log_warning("%s: done ...", log_header);

out:
	priv_p->p_stop = 1;
}

static int
__cxp_destroy_cxt(struct cxp_interface *cxp_p)
{
	char *log_header = "__cxp_destroy_cxt";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct cxtg_interface *cxtg_p = priv_p->p_cxtg;
	struct cxt_interface *cxt_p = priv_p->p_cxt;
	int rc;

	if(cxt_p) {
		rc = cxtg_p->cxg_op->cxg_drop_channel(cxtg_p, cxt_p);
		if(rc) {
			nid_log_warning("%s: failed to drop cxt %s", log_header, priv_p->p_cxt_name);
			goto out;
		}
		cxt_p->cx_op->cx_cleanup(cxt_p);
		free(cxt_p);
		priv_p->p_cxt = NULL;
	}

out:
	return rc;
}

static void
cxp_cleanup(struct cxp_interface *cxp_p) {
	char *log_header = "cxp_cleanup";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct cxt_interface *cxt_p = priv_p->p_cxt;

	nid_log_warning("%s: start ...", log_header);

	// if cxp cleanup trigger by cxt recover, should not destroy cxt
	if(!priv_p->p_keep_cxt) {
		nid_log_warning("%s: destroy cxt:[%s] ...", log_header, priv_p->p_cxt_name);
		__cxp_destroy_cxt(cxp_p);
	} else {
		nid_log_warning("%s: wait for cxt:[%s] pause to work ...", log_header, priv_p->p_cxt_name);
		while(! cxt_p->cx_op->cx_all_frozen(cxt_p)) {
			usleep(100*1000);
		}
	}

	if(!priv_p->p_stop) {
		__cxp_stop(cxp_p);
	}

	pthread_mutex_destroy(&priv_p->p_lck);
	pthread_cond_destroy(&priv_p->p_stop_cond);

	free(priv_p);
	priv_p = NULL;

	nid_log_warning("%s: done ...", log_header);
}

static int
cxp_drop_channel(struct cxp_interface *cxp_p, struct umessage_tx_hdr *msghdr)
{
	char *log_header = "cxp_drop_channel";
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	struct cxpg_interface *cxpg_p = priv_p->p_cxpg;
	uint32_t cmd_len;
	struct cxt_interface *cxt_p = priv_p->p_cxt;
	int rc = 0;
	struct umessage_cxp_drop_channel *msg_drop_chan = (struct umessage_cxp_drop_channel *)msghdr;
	char *cxp_uuid = msg_drop_chan->um_chan_cxp_uuid;
	char *send_buf;

	nid_log_warning("%s: start ...", log_header);

	// use cxpg to drop cxp with uuid
	if(cxpg_p->cxg_op->cxg_drop_channel_by_uuid(cxpg_p, cxp_uuid)) {
		nid_log_warning("%s: failed to drop cxp with uuid(%s)", log_header, cxp_uuid);
		return -1;
	}

	// send back drop channel ack message
	cmd_len = sizeof(struct umessage_cxp_drop_channel);
	send_buf = cxt_p->cx_op->cx_get_buffer(cxt_p, cmd_len);
	msghdr->um_type = UMSG_TYPE_ACK;
	memcpy(send_buf, msghdr, cmd_len);
	cxt_p->cx_op->cx_send(cxt_p, send_buf, cmd_len);

	// destroy cxp instance
	cxp_p->cx_op->cx_cleanup(cxp_p);
	free(cxp_p);
	cxp_p = NULL;

	return rc;
}

static void
cxp_mark_recover_cxt(struct cxp_interface *cxp_p) {
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;

	priv_p->p_keep_cxt = 1;
	__cxp_stop(cxp_p);
}

static void
cxp_bind_socket(struct cxp_interface *cxp_p, int csfd)
{
	struct cxp_private *priv_p = (struct cxp_private *)cxp_p->cx_private;
	priv_p->p_csfd = csfd;
}

struct cxp_operations cxp_op = {
	.cx_get_handle = cxp_get_handle,
	.cx_get_cxt = cxp_get_cxt,
	.cx_get_chan_uuid = cxp_get_chan_uuid,
	.cx_do_channel = cxp_do_channel,
	.cx_cleanup =cxp_cleanup,
	.cx_start = cxp_start,
	.cx_drop_channel = cxp_drop_channel,
	.cx_mark_recover_cxt = cxp_mark_recover_cxt,
	.cx_bind_socket = cxp_bind_socket,
};

int
cxp_initialization(struct cxp_interface *cxp_p, struct cxp_setup *setup)
{
	char *log_header = "cxp_initialization";
	struct cxp_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	cxp_p->cx_op = &cxp_op;
	priv_p = calloc(1, sizeof(*priv_p));
	cxp_p->cx_private = priv_p;

	priv_p->p_csfd = setup->csfd;
	priv_p->p_handle = setup->handle;
	priv_p->p_cxtg = setup->cxtg;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_cxpg = setup->cxpg;
	strcpy(priv_p->p_cxt_name, setup->cxt_name);
	strcpy(priv_p->p_uuid, setup->uuid);
	strcpy(priv_p->p_peer_uuid, setup->peer_uuid);

	pthread_cond_init(&priv_p->p_stop_cond, NULL);
	pthread_mutex_init(&priv_p->p_lck, NULL);

	return 0;
}
