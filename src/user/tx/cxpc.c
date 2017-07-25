/*
 * cxpc.c
 * 	Implementation of Control Exchange Passive Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>

#include "util_nw.h"
#include "list.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_cxa_if.h"
#include "umpk_cxp_if.h"
#include "cxp_if.h"
#include "cxpg_if.h"
#include "cxpcg_if.h"
#include "cxpc_if.h"

struct cxpc_private {
	struct cxp_interface	*p_cxp;
	struct cxpg_interface	*p_cxpg;
	int			p_csfd;
	int			p_dsfd;
	u_short			p_dport;
	struct umpk_interface	*p_umpk;
};

static struct cxp_interface *
__cxpc_create_channel_uuid(struct cxpc_private *priv_p, char *msg_buf,
		struct umessage_tx_hdr *cxp_msghdr)
{
	char *log_header = "__cxpc_create_channel_uuid";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct cxpg_interface *cxpg_p = priv_p->p_cxpg;
	struct cxp_interface *cxp_p = NULL;
	int csfd = priv_p->p_csfd;
	uint32_t cmd_len;
	struct umessage_cxp_create_channel *cxp_cc_msg_p;
	int rc;

	nid_log_warning("%s: start ...", log_header);
	assert(cxp_msghdr->um_req_code == UMSG_CXP_CODE_UUID);
	cmd_len = cxp_msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TX_HEADER_LEN);
	if (cmd_len > UMSG_TX_HEADER_LEN)
		util_nw_read_n(csfd, msg_buf + UMSG_TX_HEADER_LEN, cmd_len - UMSG_TX_HEADER_LEN);

	cxp_cc_msg_p = (struct umessage_cxp_create_channel *)cxp_msghdr;
	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, NID_CTYPE_CXP, (void *)cxp_cc_msg_p);
	if (rc)
		goto out;

	cxp_p = cxpg_p->cxg_op->cxg_create_channel(cxpg_p, csfd, cxp_cc_msg_p->um_chan_uuid);
	if(!cxp_p){
		nid_log_error("%s: cannot create cxpg channel", log_header);
		goto out;
	}

out:
	return cxp_p;
}

static struct cxp_interface *
__cxpc_create_channel(struct cxpc_private *priv_p, char *msg_buf,
	struct umessage_tx_hdr *msghdr)
{
	char *log_header = "__cxpc_create_channel";
	struct cxp_interface *cxp_p = NULL;

	nid_log_warning("%s: start ...", log_header);
	assert(msghdr->um_req == UMSG_CXP_CMD_CREATE_CHANNEL);
	switch (msghdr->um_req_code) {
	case UMSG_CXP_CODE_UUID:
		cxp_p = __cxpc_create_channel_uuid(priv_p, msg_buf, msghdr);
		break;

	default:
		nid_log_error("%s: got wrong code (%d)", log_header, msghdr->um_req_code);
		break;
	}

	return cxp_p;
}

static int
__cxpc_drop_channel(struct cxpc_private *priv_p, struct umessage_tx_hdr *msghdr)
{
	char *log_header = "__cxpc_drop_channel";
	struct cxp_interface *cxp_p;
	struct cxpg_interface *cxpg_p = priv_p->p_cxpg;
	struct umessage_cxp_drop_channel *msg_drop_chan = (struct umessage_cxp_drop_channel *)msghdr;
	char *cxp_uuid = msg_drop_chan->um_chan_cxp_uuid;

	nid_log_warning("%s: start ...", log_header);

	// get cxp with uuid by cxpg
	cxp_p = cxpg_p->cxg_op->cxg_get_cxp_by_uuid(cxpg_p, cxp_uuid);
	if(!cxp_p) {
		nid_log_warning("%s: cxp with uuid(%s) not found", log_header, cxp_uuid);
		return -1;
	}

	return cxp_p->cx_op->cx_drop_channel(cxp_p, msghdr);
}

static int
cxpc_accept_new_channel(struct cxpc_interface *cxpc_p, int csfd)
{
	char *log_header = "cxpc_accept_new_channel";
	struct cxpc_private *priv_p = cxpc_p->cxc_private;

	nid_log_warning("%s: start ...", log_header);
	priv_p->p_csfd = csfd;
	return 0;
}

static void
cxpc_do_channel(struct cxpc_interface *cxpc_p)
{
	char *log_header = "cxpc_do_channel";
	struct cxpc_private *priv_p = (struct cxpc_private *)cxpc_p->cxc_private;
	int csfd = priv_p->p_csfd;
	char msg_buf[4096];
	union umessage_cxp cxp_msg;
	struct umessage_tx_hdr *msghdr = (struct umessage_tx_hdr *)&cxp_msg;
	struct cxp_interface *cxp_p = NULL;
	struct cxpg_interface *cxpg_p = priv_p->p_cxpg;
	int nread;

	nid_log_warning("%s: start ...", log_header);

	nread = util_nw_read_n(csfd, msg_buf, UMSG_TX_HEADER_LEN);
	if(nread != UMSG_TX_HEADER_LEN) {
		nid_log_error("%s: failed to read request message, return %d", log_header, nread);
		assert(0);
	}

	// decode message header
	uint32_t dlen = (uint32_t)nread;
	decode_tx_hdr(msg_buf, &dlen, msghdr);

	switch (msghdr->um_req) {
	case UMSG_CXP_CMD_CREATE_CHANNEL: {
		cxp_p = __cxpc_create_channel(priv_p, msg_buf, msghdr);

		if(cxp_p) {
			// bind socket with cxp, because when network error, cxp will exist, but perviouse socket
			// will become invalid, it must rebind csfd with cxp
			cxp_p->cx_op->cx_bind_socket(cxp_p, csfd);

			/* Now, the cx channel established, do the job */
			cxp_p->cx_op->cx_do_channel(cxp_p);

			cxpg_p->cxg_op->cxg_drop_channel(cxpg_p, cxp_p);
			cxp_p->cx_op->cx_cleanup(cxp_p);
		}
		break;
	}

	// response for drop channel message from dxa side
	case UMSG_CXP_CMD_DROP_CHANNEL:
		__cxpc_drop_channel(priv_p, (struct umessage_tx_hdr *)msg_buf);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
		break;
	}

	close(csfd);

	return;
}

static void
cxpc_cleanup(struct cxpc_interface *cxpc_p)
{
	nid_log_debug("cxpc_cleanup start, cxpc_p:%p", cxpc_p);
	if (cxpc_p->cxc_private != NULL) {
		free(cxpc_p->cxc_private);
		cxpc_p->cxc_private = NULL;
	}
}

struct cxpc_operations cxpc_op = {
	.cxc_accept_new_channel = cxpc_accept_new_channel,
	.cxc_do_channel = cxpc_do_channel,
	.cxc_cleanup = cxpc_cleanup,
};

int
cxpc_initialization(struct cxpc_interface *cxpc_p, struct cxpc_setup *setup)
{
	char *log_header = "cxpc_initialization";
	struct cxpc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = calloc(1, sizeof(*priv_p));
	cxpc_p->cxc_private = priv_p;
	cxpc_p->cxc_op = &cxpc_op;

	priv_p->p_csfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_cxpg = setup->cxpg;

	return 0;
}
