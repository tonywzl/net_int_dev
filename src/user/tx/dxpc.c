/*
 * dxpc.c
 * 	Implementation of Data Exchange Passive Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include <errno.h>

#include "util_nw.h"
#include "list.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_dxa_if.h"
#include "umpk_dxp_if.h"
#include "dxp_if.h"
#include "dxpg_if.h"
#include "dxpcg_if.h"
#include "dxpc_if.h"

struct dxpc_private {
	struct dxpg_interface	*p_dxpg;
	int			p_csfd;
	int			p_dsfd;
	struct umpk_interface	*p_umpk;
};

static struct dxp_interface *
__dxpc_create_channel_uuid(struct dxpc_private *priv_p, char *msg_buf,
		struct umessage_tx_hdr *dxp_msghdr)
{
	char *log_header = "__dxpc_create_channel_uuid";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct dxpg_interface *dxpg_p = priv_p->p_dxpg;
	struct dxp_interface *dxp_p = NULL;
	int csfd = priv_p->p_csfd;
	uint32_t cmd_len;
	struct umessage_dxp_create_channel *dxp_cc;
	int rc;

	nid_log_warning("%s: start ...", log_header);
	assert(dxp_msghdr->um_req_code == UMSG_DXP_CODE_UUID);
	cmd_len = dxp_msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TX_HEADER_LEN);
	if (cmd_len > UMSG_TX_HEADER_LEN) {
		int nread = util_nw_read_n(csfd, msg_buf + UMSG_TX_HEADER_LEN, cmd_len - UMSG_TX_HEADER_LEN);
		if(nread != (int)(cmd_len - UMSG_TX_HEADER_LEN)) {
			nid_log_error("%s: failed to read full message", log_header);
			goto out;
		}
	}

	dxp_cc = (struct umessage_dxp_create_channel *)dxp_msghdr;
	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, NID_CTYPE_DXP, (void *)dxp_cc);
	if (rc)
		goto out;

	dxp_p = dxpg_p->dxg_op->dxg_create_channel(dxpg_p, csfd, dxp_cc->um_chan_uuid);
	if(!dxp_p){
		nid_log_error("%s: cannot create dxpg channel", log_header);
		goto out;
	}

out:
	// should not close csfd, it wil lead csfd in dxp become invalid
	return dxp_p;
}

static struct dxp_interface *
__dxpc_create_channel(struct dxpc_private *priv_p, char *msg_buf,
	struct umessage_tx_hdr *msghdr)
{
	char *log_header = "__dxpc_create_channel";
	struct dxp_interface *dxp_p = NULL;

	nid_log_warning("%s: start ...", log_header);
	assert(msghdr->um_req == UMSG_DXP_CMD_CREATE_CHANNEL);
	switch (msghdr->um_req_code) {
	case UMSG_DXP_CODE_UUID:
		dxp_p = __dxpc_create_channel_uuid(priv_p, msg_buf, msghdr);
		break;

	default:
		nid_log_error("%s: got wrong code (%d)", log_header, msghdr->um_req_code);
		break;
	}

	return dxp_p;
}

static int
__do_drop_channel(struct dxpc_private *priv_p, struct umessage_tx_hdr *msghdr)
{
	char *log_header = "__do_drop_channel";
	struct dxp_interface *dxp_p;
	struct dxpg_interface *dxpg_p = priv_p->p_dxpg;
	struct umessage_dxp_drop_channel *msg_drop_chan = (struct umessage_dxp_drop_channel *)msghdr;
	char *dxp_uuid = msg_drop_chan->um_chan_dxp_uuid;

	nid_log_warning("%s: start ...", log_header);

	// get dxp with uuid by dxpg
	dxp_p = dxpg_p->dxg_op->dxg_get_dxp_by_uuid(dxpg_p, dxp_uuid);
	if(!dxp_p) {
		nid_log_warning("%s: dxp with uuid(%s) not found", log_header, dxp_uuid);
		return -1;
	}

	// drop dxp by dxpg, and destroy dxt by dxp, then free dxp
	return dxp_p->dx_op->dx_drop_channel(dxp_p);
}

static int
dxpc_accept_new_channel(struct dxpc_interface *dxpc_p, int csfd)
{
	char *log_header = "dxpc_accept_new_channel";
	struct dxpc_private *priv_p = dxpc_p->dxc_private;

	nid_log_warning("%s: start ...", log_header);
	priv_p->p_csfd = csfd;
	return 0;
}

static void
dxpc_do_channel(struct dxpc_interface *dxpc_p)
{
	char *log_header = "dxpc_do_channel";
	struct dxpc_private *priv_p = (struct dxpc_private *)dxpc_p->dxc_private;
	int csfd = priv_p->p_csfd;
	char msg_buf[4096];
	union umessage_dxp dxp_msg;
	struct umessage_tx_hdr *msghdr = (struct umessage_tx_hdr *)&dxp_msg;
	struct dxp_interface *dxp_p = NULL;
	struct dxpg_interface *dxpg_p = priv_p->p_dxpg;
	int nread;

	nid_log_warning("%s: start ...", log_header);

	nread = util_nw_read_n(csfd, msg_buf, UMSG_TX_HEADER_LEN);
	if(nread != UMSG_TX_HEADER_LEN) {
		nid_log_error("%s: failed to read request message, return %d", log_header, nread);
		goto out;
	}

	// decode message header
	uint32_t dlen = (uint32_t)nread;
	decode_tx_hdr(msg_buf, &dlen, msghdr);

	switch (msghdr->um_req) {
	case UMSG_DXP_CMD_CREATE_CHANNEL: {
		dxp_p = __dxpc_create_channel(priv_p, msg_buf, msghdr);

		if(dxp_p) {
			// bind socket with dxp, because when network error, dxp will exist, but perviouse socket
			// will become invalid, it must rebind csfd with dxp
			dxp_p->dx_op->dx_bind_socket(dxp_p, csfd);

			/* Now, the dx channel established, do the job */
			dxp_p->dx_op->dx_do_channel(dxp_p);

			dxpg_p->dxg_op->dxg_drop_channel(dxpg_p, dxp_p);
			dxp_p->dx_op->dx_cleanup(dxp_p);
		}
		break;
	}

	// response for drop channel message from dxa side
	case UMSG_DXP_CMD_DROP_CHANNEL: {
		__do_drop_channel(priv_p, (struct umessage_tx_hdr *)msg_buf);
		break;
	}

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
		break;
	}

out:
	nid_log_warning("%s: close control socket %d ...", log_header, csfd);
	close(csfd);
}

static void
dxpc_cleanup(struct dxpc_interface *dxpc_p)
{
	nid_log_debug("dxpc_cleanup start, dxpc_p:%p", dxpc_p);
	if (dxpc_p->dxc_private != NULL) {
		free(dxpc_p->dxc_private);
		dxpc_p->dxc_private = NULL;
	}
}

struct dxpc_operations dxpc_op = {
	.dxc_accept_new_channel = dxpc_accept_new_channel,
	.dxc_do_channel = dxpc_do_channel,
	.dxc_cleanup = dxpc_cleanup,
};

int
dxpc_initialization(struct dxpc_interface *dxpc_p, struct dxpc_setup *setup)
{
	char *log_header = "dxpc_initialization";
	struct dxpc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = calloc(1, sizeof(*priv_p));
	dxpc_p->dxc_private = priv_p;
	dxpc_p->dxc_op = &dxpc_op;

	priv_p->p_csfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_dxpg = setup->dxpg;

	return 0;
}
