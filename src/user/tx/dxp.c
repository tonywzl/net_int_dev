/*
 * dxp.c
 * 	Implementation of Data Exchange Passive Module
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "util_nw.h"
#include "nid.h"
#include "nid_log.h"
#include "dxt_if.h"
#include "dxtg_if.h"
#include "dxp_if.h"
#include "dxpg_if.h"
#include "umpk_if.h"
#include "umpk_dxa_if.h"
#include "umpk_dxp_if.h"
#include "tx_shared.h"
#include "lstn_if.h"

struct dxp_private {
	int			p_csfd;	// control socket fd
	int			p_dsfd;	// data socket fd
	u_short			p_dport;
	struct dxt_interface	*p_dxt;
	struct dxtg_interface   *p_dxtg;
	char	p_dxt_name[NID_MAX_UUID];
	char	p_uuid[NID_MAX_UUID];
	char	p_peer_uuid[NID_MAX_UUID];
	uint64_t                p_last_sequence;
	uint32_t                p_last_len;
	struct umpk_interface	*p_umpk;
	struct dxpg_interface	*p_dxpg;
	uint8_t		p_stop;
	uint8_t		p_to_stop;
	uint8_t		p_keep_dxt;
	pthread_mutex_t		p_lck;
	pthread_cond_t		p_stop_cond;
};

static int __dxp_start(struct dxp_interface *);

static int
__dxp_bind_any(int *sfd)
{
	char *log_header = "__dxp_bind_any";
	u_short port;

	/*
	 * find and bind an avalible port for data connection
	 */
	port = NID_SERVER_PORT + 1;
	while (util_nw_bind(*sfd, port) < 0) {
		*sfd = util_nw_open();
		if (*sfd < 0) {
			nid_log_error("%s: cannot open socket, errno: %d", log_header, errno);
		}
		port++;
	}
	return port;
}

static int32_t
dxp_ack_dport(struct dxp_interface *dxp_p, int csfd) {
	char *log_header = "dxp_ack_dport";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_dxa_create_channel_dport dxa_cc_dport;
	struct umessage_tx_hdr *dxa_msghdr = (struct umessage_tx_hdr *)&dxa_cc_dport;
	char dbuf[1024], *msg_buf = dbuf;
	int len, rc, cmd_len, nwrite;

	/*
	 * Tell the active site the data port I'm binding
	 */
	memset(&dxa_cc_dport, 0, sizeof(struct umessage_dxa_create_channel_dport));
	len = strlen(priv_p->p_uuid);
	dxa_cc_dport.um_chan_uuid_len = len;
	memcpy(dxa_cc_dport.um_chan_uuid, priv_p->p_uuid, len);
	dxa_cc_dport.um_dport = priv_p->p_dport;
	dxa_msghdr->um_req = UMSG_DXA_CMD_CREATE_CHANNEL;
	dxa_msghdr->um_req_code = UMSG_DXA_CODE_DPORT;
	dxa_msghdr->um_level = UMSG_LEVEL_TX;
	dxa_msghdr->um_type = UMSG_TYPE_ACK;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, (uint32_t *)&cmd_len, NID_CTYPE_DXA, dxa_msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		return -1;
	}
	nwrite = util_nw_write_n(csfd, msg_buf, cmd_len);
	if(nwrite != cmd_len) {
		nid_log_error("%s: failed to send create channel command to dxa side, errno:%d",
				log_header, errno);
		return -1;
	}

	return 0;
}


static struct dxt_interface *
dxp_get_dxt(struct dxp_interface *dxp_p)
{
        struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
        return priv_p->p_dxt;
}


static int
dxp_accept(struct dxp_interface *dxp_p)
{
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	int data_sfd = priv_p->p_dsfd;
	struct sockaddr_in caddr;
	socklen_t clen;

	/*
	 * waiting for the active part to connect the data port
	 */
	priv_p->p_dsfd = accept(data_sfd, (struct sockaddr *)&caddr, &clen);
	close(data_sfd);
	return priv_p->p_dsfd;
}

static int __dxp_destroy_dxt(struct dxp_interface *dxp_p);

static void
__dxp_dxt_callback(void *p1, void *p2)
{
	char *log_header = "__dxp_dxt_callback";
	struct dxp_interface *dxp_p = (struct dxp_interface *)p1;
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct dxt_interface *dxt_p = priv_p->p_dxt;
	struct umessage_tx_hdr *msg_hdr =  (struct umessage_tx_hdr *)p2;
	uint32_t cmd_len = sizeof(struct umessage_tx_hdr);
	struct umessage_tx_hdr *ack_msg_hdr;
	char *msg_buf;

	switch(msg_hdr->um_req) {
	case UMSG_DXP_CMD_KEEPALIVE:
		// send back ack to dxa
		msg_buf = dxt_p->dx_op->dx_get_buffer(dxt_p, cmd_len);
		memcpy(msg_buf, msg_hdr, sizeof(struct umessage_tx_hdr));
		ack_msg_hdr = (struct umessage_tx_hdr *)msg_buf;
		ack_msg_hdr->um_type = UMSG_TYPE_ACK;
		dxt_p->dx_op->dx_send(dxt_p, msg_buf, cmd_len, NULL, 0);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msg_hdr->um_req);
		break;
	}

	// Notice: there have no seq update call, because dxp is passive mode
	// TODO: if we need it later, ref code in _dxa_dxt_callback part

	return;
}

static int
__dxp_get_dxt_stat(struct dxp_interface *dxp_p, struct dxt_status *stat_p)
{
	char *log_header = "__dxp_get_dxt_stat";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct dxt_interface *dxt_p = priv_p->p_dxt;

	assert(stat_p);

	// make sure dxt initialized
	if(!dxt_p) {
		nid_log_warning("%s: dxt not initialize yet ...", log_header);
		return -1;
	}

	dxt_p->dx_op->dx_get_status(dxt_p, stat_p);
	return 0;
}


static int
__dxp_stat_channel(struct dxp_interface *dxp_p) {
	char *log_header = "__dxp_stat_channel";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct dxt_status dxt_stat, *stat_p = &dxt_stat;
	struct umessage_dxp_stat_channel stat_msg;
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
	if(msghdr->um_req != UMSG_DXP_CMD_STAT_CHANNEL) {
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
	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, msghdr->um_len, NID_CTYPE_DXP, (void *)msghdr);
	if (rc) {
		nid_log_error("%s: cannot decode", log_header);
		return -1;
	}

	struct umessage_dxp_stat_channel_resp dxp_sc_resp_msg, *dxp_sc_resp_msg_p = &dxp_sc_resp_msg;

	// send back dxt runtime info to dxa side
	__dxp_get_dxt_stat(dxp_p, stat_p);
	dxp_sc_resp_msg_p->ack_seq = stat_p->ack_seq;
	dxp_sc_resp_msg_p->cmsg_left = stat_p->cmsg_left;
	dxp_sc_resp_msg_p->dsend_seq = stat_p->dsend_seq;
	dxp_sc_resp_msg_p->recv_dseq = stat_p->recv_dseq;
	dxp_sc_resp_msg_p->recv_seq = stat_p->recv_seq;
	dxp_sc_resp_msg_p->send_seq = stat_p->send_seq;

	memcpy(dxp_sc_resp_msg_p, &stat_msg, sizeof(stat_msg));
	dxp_sc_resp_msg_p->um_header.um_req_code = UMSG_DXP_CODE_STAT_RESP;
	msghdr = (struct umessage_tx_hdr *)dxp_sc_resp_msg_p;

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &dlen, NID_CTYPE_DXP, msghdr);
	if (rc) {
		nid_log_error("%s: failed to encode message", log_header);
		goto out;
	}
	nid_log_debug("%s: start to send stat channel response message to dxa side", log_header);
	nwrite = util_nw_write_n(csfd, msg_buf, dlen);
	if((uint32_t)nwrite != dlen) {
		rc = -1;
		nid_log_error("%s: failed to send stat channel response message to dxa side, errno:%d",
				log_header, errno);
		goto out;
	}

	nid_log_warning("%s: done ...", log_header);

out:
	return rc;
}

static int
__dxp_check_start_work_msg(struct dxp_interface *dxp_p) {
	char *log_header = "__dxp_check_start_work_msg";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct dxt_interface *dxt_p;
	struct umessage_tx_hdr *msghdr;
	struct umessage_dxp_start_work dxp_sw_msg, *dxp_sw_msg_p = &dxp_sw_msg;
	struct umessage_dxp_start_work_resp dxp_sw_resp_msg, *dxp_sw_resp_msg_p = &dxp_sw_resp_msg;
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
	msghdr = (struct umessage_tx_hdr *)dxp_sw_msg_p;
	memset(dxp_sw_msg_p, 0, sizeof(*dxp_sw_msg_p));
	decode_tx_hdr(msg_buf, &dlen, msghdr);

	if(msghdr->um_req != UMSG_DXP_CMD_START_WORK) {
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
	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, msghdr->um_len, NID_CTYPE_DXP, dxp_sw_msg_p);
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
	if(msghdr->um_req_code == UMSG_DXP_CODE_DROP_ALL) {
		nid_log_debug("%s: destroy dxt allocated in the past", log_header);
		dxt_p = priv_p->p_dxt;
		if(dxt_p) {
			dxt_p->dx_op->dx_reset_all(dxt_p);
		}
	}

	memset(dxp_sw_resp_msg_p, 0, sizeof(*dxp_sw_resp_msg_p));
	memcpy(dxp_sw_resp_msg_p, dxp_sw_msg_p, sizeof(*dxp_sw_resp_msg_p));
	msghdr = (struct umessage_tx_hdr *)dxp_sw_resp_msg_p;
	// send back response message
	msghdr->um_req_code = UMSG_DXP_START_WORK_RESP;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &dlen, NID_CTYPE_DXP, msghdr);
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
dxp_do_channel(struct dxp_interface *dxp_p)
{
	char *log_header = "dxp_do_channel";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	int rc = 0;

	nid_log_warning("%s: start ...", log_header);

	// if not success response channel state info to dxa side
	if(__dxp_stat_channel(dxp_p) != 0) {
		nid_log_warning("%s: failed in process channel stat command", log_header);
		goto out;
	}

	// if not success response channel state info to dxa side
	if(__dxp_check_start_work_msg(dxp_p) != 0) {
		nid_log_warning("%s: failed in process channel stat command", log_header);
		goto out;
	}

	// start dxt or make dxt from recover state to work state
	rc = __dxp_start(dxp_p);

	if(rc != -1) {
		nid_log_warning("%s: wait stop signal ...", log_header);
		pthread_mutex_lock(&priv_p->p_lck);
		while (!priv_p->p_to_stop)
			pthread_cond_wait(&priv_p->p_stop_cond, &priv_p->p_lck);
		priv_p->p_stop = 1;
		pthread_cond_signal(&priv_p->p_stop_cond);
		pthread_mutex_unlock(&priv_p->p_lck);
	}
	nid_log_warning("%s: done ...", log_header);

out:
	priv_p->p_stop = 1;
}


static int
dxp_drop_channel(struct dxp_interface *dxp_p)
{
	char *log_header = "dxp_drop_channel";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct dxpg_interface *dxpg_p = priv_p->p_dxpg;

	nid_log_warning("%s: start ...", log_header);

	// remove dxp from dxpg channel list
	if(dxpg_p->dxg_op->dxg_drop_channel(dxpg_p, dxp_p)) {
		nid_log_warning("%s: failed to drop dxp: (%s)", log_header, priv_p->p_uuid);
		return -1;
	}

	// destroy dxp instance
	dxp_p->dx_op->dx_cleanup(dxp_p);
	free(dxp_p);
	dxp_p = NULL;

	return 0;
}

static int
__dxp_destroy_dxt(struct dxp_interface *dxp_p)
{
	char *log_header = "__dxp_destroy_dxt";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct dxtg_interface *dxtg_p = priv_p->p_dxtg;
	struct dxt_interface *dxt_p = priv_p->p_dxt;
	int rc;

	if(dxt_p) {
		rc = dxtg_p->dxg_op->dxg_drop_channel(dxtg_p, dxt_p);
		if(rc) {
			nid_log_warning("%s: failed to drop dxt %s", log_header, priv_p->p_dxt_name);
			goto out;
		}
		dxt_p->dx_op->dx_cleanup(dxt_p);
		free(dxt_p);
		priv_p->p_dxt = NULL;
	}

out:
	return rc;
}

static void
__dxp_stop(struct dxp_interface *dxp_p)
{
	char *log_header = "dxp_stop";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;

	nid_log_warning("%s: start ...", log_header);

	// make dxp_do_channel thread terminate
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_to_stop = 1;
	pthread_cond_signal(&priv_p->p_stop_cond);
	while (!priv_p->p_stop)
		pthread_cond_wait(&priv_p->p_stop_cond, &priv_p->p_lck);
	pthread_mutex_unlock(&priv_p->p_lck);

	if(priv_p->p_dsfd != 0 && priv_p->p_dsfd != -1) {
		nid_log_warning("%s: close data socket %d ...", log_header, priv_p->p_dsfd);
		close(priv_p->p_dsfd);
		priv_p->p_dsfd = -1;
	}

	nid_log_warning("%s: done ...", log_header);
}


static int
dxp_cleanup(struct dxp_interface *dxp_p)
{
	char *log_header = "dxp_cleanup";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct dxt_interface *dxt_p = priv_p->p_dxt;

	nid_log_warning("%s: start ...", log_header);

	// if dxp cleanup trigger by dxt recover, should not destroy dxt
	if(!priv_p->p_keep_dxt) {
		nid_log_warning("%s: destroy dxt:[%s] ...", log_header, priv_p->p_dxt_name);
		__dxp_destroy_dxt(dxp_p);
	} else {
		nid_log_warning("%s: wait for dxt:[%s] pause to work ...", log_header, priv_p->p_dxt_name);
		while(! dxt_p->dx_op->dx_all_frozen(dxt_p)) {
			usleep(100*1000);
		}
		nid_log_warning("%s: dxt:[%s] pause to work done", log_header, priv_p->p_dxt_name);
	}

	// close data socket
	if(!priv_p->p_stop && priv_p->p_dsfd != -1) {
		__dxp_stop(dxp_p);
	}

	pthread_mutex_destroy(&priv_p->p_lck);
	pthread_cond_destroy(&priv_p->p_stop_cond);

	free(priv_p);
	priv_p = NULL;

	nid_log_warning("%s: done ...", log_header);

	return 0;
}

static void
dxp_mark_recover_dxt(struct dxp_interface *dxp_p) {
	char *log_header = "dxp_mark_recover_dxt";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;

	nid_log_warning("%s: start ...", log_header);

	priv_p->p_keep_dxt = 1;
	__dxp_stop(dxp_p);
}

static char*
dxp_get_chan_uuid(struct dxp_interface *dxp_p)
{
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;

	return priv_p->p_uuid;
}

static void*
__dxp_run_dxt_thread(void *p)
{
	char *log_header = "__dxp_run_dxt_thread";
	struct dxt_interface *dxt_p = (struct dxt_interface *)p;

	nid_log_warning("%s: start ...", log_header);
	dxt_p->dx_op->dx_do_channel(dxt_p);
	nid_log_warning("%s: done ...", log_header);

	return NULL;
}

static int
__dxp_start(struct dxp_interface *dxp_p)
{
	char *log_header = "__dxp_start";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	struct dxt_interface *dxt_p;
	struct dxtg_interface *dxtg_p = priv_p->p_dxtg;

	nid_log_warning("%s: start ...", log_header);

	nid_log_warning("%s: create dxt ...", log_header);
	dxt_p = dxtg_p->dxg_op->dxg_search_and_create_channel(dxtg_p, priv_p->p_dxt_name, priv_p->p_csfd, priv_p->p_dsfd );
	if(!dxt_p) {
		nid_log_error("%s: can not create dxt interface", log_header);
		return -1;
	}
	priv_p->p_dxt = dxt_p;

	if(dxt_p->dx_op->dx_is_new(dxt_p)) {
		nid_log_warning("%s: run dxt ...", log_header);
		dxt_p->dx_op->dx_bind_dxp(dxt_p, dxp_p, __dxp_dxt_callback);

		// start dxt in a seperate thread
		pthread_attr_t attr;
		pthread_t thread_id;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, __dxp_run_dxt_thread, (void *)dxt_p);
	} else {
		nid_log_warning("%s: dxt recover with dsfd:%d and csfd:%d ...",
				log_header, priv_p->p_dsfd, priv_p->p_csfd);
		// dxt already exist in system, only need to wake it up
		dxt_p->dx_op->dx_recover(dxt_p, priv_p->p_dsfd, priv_p->p_csfd);
	}

	nid_log_warning("%s: done ...", log_header);

	return 0;
}

static void
dxp_bind_socket(struct dxp_interface *dxp_p, int csfd)
{
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;
	priv_p->p_csfd = csfd;
}

static int
dxp_init_dport(struct dxp_interface *dxp_p)
{
	char *log_header = "dxp_init_dport";
	struct dxp_private *priv_p = (struct dxp_private *)dxp_p->dx_private;

	if(priv_p->p_dsfd && priv_p->p_dsfd != -1) {
		nid_log_error("%s: socket is in open status", log_header);
		return -1;
	}

	priv_p->p_dsfd = util_nw_open();
	if (priv_p->p_dsfd < 0) {
		nid_log_error("%s: cannot open socket", log_header);
		return -1;
	}
	priv_p->p_dport = __dxp_bind_any(&priv_p->p_dsfd);

	return 0;
}

struct dxp_operations dxp_op = {
	.dx_get_dxt = dxp_get_dxt,
	.dx_accept = dxp_accept,
	.dx_do_channel = dxp_do_channel,
	.dx_cleanup = dxp_cleanup,
	.dx_get_chan_uuid = dxp_get_chan_uuid,
	.dx_drop_channel = dxp_drop_channel,
	.dx_mark_recover_dxt = dxp_mark_recover_dxt,
	.dx_ack_dport = dxp_ack_dport,
	.dx_bind_socket = dxp_bind_socket,
	.dx_init_dport = dxp_init_dport,
};

int
dxp_initialization(struct dxp_interface *dxp_p, struct dxp_setup *setup)
{
	char *log_header = "dxp_initialization";
	struct dxp_private *priv_p;

	nid_log_debug("%s: start ...", log_header);
	assert(setup);
	dxp_p->dx_op = &dxp_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dxp_p->dx_private = priv_p;

	priv_p->p_csfd = setup->csfd;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_dxpg = setup->dxpg;
	priv_p->p_dxtg = setup->dxtg;
	strcpy(priv_p->p_uuid, setup->uuid);
	strcpy(priv_p->p_peer_uuid, setup->peer_uuid);
	strcpy(priv_p->p_dxt_name, setup->dxt_name);

	if(dxp_p->dx_op->dx_init_dport(dxp_p)) {
		return -1;
	}

	pthread_cond_init(&priv_p->p_stop_cond, NULL);
	pthread_mutex_init(&priv_p->p_lck, NULL);

	return 0;
}
