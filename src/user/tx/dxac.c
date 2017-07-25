/*
 * dxac.c
 * 	Implementation of Data Exchange Active Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "util_nw.h"
#include "list.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_dxa_if.h"
#include "umpk_dxp_if.h"
#include "sac_if.h"
#include "tp_if.h"
#include "bio_if.h"
#include "dxa_if.h"
#include "dxag_if.h"
#include "dxacg_if.h"
#include "dxac_if.h"

struct dxac_private {
	struct dxacg_interface	*p_dxacg;
	struct dxag_interface	*p_dxag;
	struct umpk_interface	*p_umpk;
	int			p_sfd;
};


static int
dxac_accept_new_channel(struct dxac_interface *dxac_p, int sfd)
{
	nid_log_debug("dxac_accept_new_channel start ...");
	struct dxac_private *priv_p = dxac_p->dxc_private;
	priv_p->p_sfd = sfd;
	return 0;
}

static int
__dxac_drop_channel(struct dxac_private *priv_p, char *msg_buf, struct umessage_tx_hdr *msghdr)
{
	char *log_header = "__dxac_drop_channel";
	struct dxag_interface *dxag_p = priv_p->p_dxag;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_dxa_drop_channel *drop_msg_p = (struct umessage_dxa_drop_channel *)msghdr;
	struct dxa_interface *dxa_p = NULL;
	uint32_t cmd_len;
	int rc, csfd = priv_p->p_sfd;

	nid_log_warning("%s: start ...", log_header);
	assert(msghdr->um_req == UMSG_DXA_CMD_DROP_CHANNEL);
	assert(msghdr->um_req_code == UMSG_DXA_DROP_RESP_SUCCESS || msghdr->um_req_code == UMSG_DXA_DROP_RESP_FAILE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TX_HEADER_LEN);

	if(msghdr->um_req_code == UMSG_DXA_DROP_RESP_FAILE) {
		nid_log_warning("%s: failed to drop channel in dxp side", log_header);
		rc = 1;
		goto out;

	}
	util_nw_read_n(csfd, msg_buf + UMSG_TX_HEADER_LEN, cmd_len - UMSG_TX_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, NID_CTYPE_DXA, drop_msg_p);
	if (rc) {
		nid_log_warning("%s: failed to decode request message", log_header);
		goto out;
	}

	// TODO: add code to verify it uuid match ?
	char *dxa_chan_uuid = drop_msg_p->um_chan_dxa_uuid;
	dxa_p = dxag_p->dxg_op->dxg_get_dxa_by_uuid(dxag_p, dxa_chan_uuid);
	rc = dxag_p->dxg_op->dxg_drop_channel_by_uuid(dxag_p, dxa_chan_uuid);
	if(rc) {
		nid_log_warning("%s: success drop channel with uuid:%s", log_header, dxa_chan_uuid);
		dxa_p->dx_op->dx_cleanup(dxa_p);
		free(dxa_p);
		dxa_p = NULL;
	} else {
		nid_log_warning("%s: failed to find channel with uuid:%s", log_header, dxa_chan_uuid);
		rc = 1;
	}

out:
	return rc;
}

static void
__dxac_display(struct dxac_private *priv_p, char *msg_buf, struct umessage_dxa_display *dis_msg)
{
	char *log_header = "__dxac_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct dxag_interface *dxag_p =  priv_p->p_dxag;
	struct umessage_dxa_display_resp resp_msg;
	struct dxa_setup *setup_p, *cur_setup_p;
	int sfd = priv_p->p_sfd;
	uint32_t cmd_len;
	struct umessage_tx_hdr *msghdr;
	int ctype = NID_CTYPE_DXA, rc;
	char nothing_back;
	int num_dxa, num_working_dxa, i, j;
	char **working_dxa = NULL;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_tx_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_DXA_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TX_HEADER_LEN);

	if (cmd_len > UMSG_TX_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TX_HEADER_LEN, cmd_len - UMSG_TX_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, dxa_uuid:%s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_chan_dxa_uuid);
	memset(&resp_msg, 0, sizeof(resp_msg));
	switch (msghdr->um_req_code) {
	case UMSG_DXA_CODE_S_DISP:
		setup_p = dxag_p->dxg_op->dxg_get_all_dxa_setup(dxag_p, &num_dxa);
		for (i = 0; i < num_dxa; i++, setup_p++) {
			if (!strcmp(setup_p->uuid, dis_msg->um_chan_dxa_uuid)) {
				resp_msg.um_chan_dxa_uuid_len = strlen(setup_p->uuid);
				memcpy(resp_msg.um_chan_dxa_uuid, setup_p->uuid, resp_msg.um_chan_dxa_uuid_len);
				resp_msg.um_peer_uuid_len = strlen(setup_p->peer_uuid);
				memcpy(resp_msg.um_peer_uuid, setup_p->peer_uuid, resp_msg.um_peer_uuid_len);
				resp_msg.um_ip_len = strlen(setup_p->ip);
				memcpy(resp_msg.um_ip, setup_p->ip, resp_msg.um_ip_len);
				resp_msg.um_dxt_name_len = strlen(setup_p->dxt_name);
				memcpy(resp_msg.um_dxt_name, setup_p->dxt_name, resp_msg.um_dxt_name_len);

				msghdr = (struct umessage_tx_hdr *)&resp_msg;
				msghdr->um_req = UMSG_DXA_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_DXA_CODE_S_RESP_DISP;
				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				util_nw_write_n(sfd, msg_buf, cmd_len);
				util_nw_read_n(sfd, &nothing_back, 1);
				break;
			}
		}

		break;

	case UMSG_DXA_CODE_S_DISP_ALL:
		setup_p = dxag_p->dxg_op->dxg_get_all_dxa_setup(dxag_p, &num_dxa);
		for (i = 0; i < num_dxa; i++, setup_p++) {
			resp_msg.um_chan_dxa_uuid_len = strlen(setup_p->uuid);
			memcpy(resp_msg.um_chan_dxa_uuid, setup_p->uuid, resp_msg.um_chan_dxa_uuid_len);
			resp_msg.um_peer_uuid_len = strlen(setup_p->peer_uuid);
			memcpy(resp_msg.um_peer_uuid, setup_p->peer_uuid, resp_msg.um_peer_uuid_len);
			resp_msg.um_ip_len = strlen(setup_p->ip);
			memcpy(resp_msg.um_ip, setup_p->ip, resp_msg.um_ip_len);
			resp_msg.um_dxt_name_len = strlen(setup_p->dxt_name);
			memcpy(resp_msg.um_dxt_name, setup_p->dxt_name, resp_msg.um_dxt_name_len);

			msghdr = (struct umessage_tx_hdr *)&resp_msg;
			msghdr->um_req = UMSG_DXA_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_DXA_CODE_S_RESP_DISP_ALL;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			util_nw_write_n(sfd, msg_buf, cmd_len);
		}

		memset(&resp_msg, 0, sizeof(resp_msg));
		msghdr = (struct umessage_tx_hdr *)&resp_msg;
		msghdr->um_req = UMSG_DXA_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_DXA_CODE_DISP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		util_nw_write_n(sfd, msg_buf, cmd_len);
		util_nw_read_n(sfd, &nothing_back, 1);

		break;

	case UMSG_DXA_CODE_W_DISP:
		working_dxa = dxag_p->dxg_op->dxg_get_working_dxa_name(dxag_p, &num_working_dxa);
		for (i = 0; i < num_working_dxa; i++) {
			if (!strcmp(working_dxa[i], dis_msg->um_chan_dxa_uuid)) {
				break;
			}
		}

		if (i < num_working_dxa) {
			setup_p = dxag_p->dxg_op->dxg_get_all_dxa_setup(dxag_p, &num_dxa);
			for (i = 0; i < num_dxa; i++, setup_p++) {
				if (!strcmp(setup_p->uuid, dis_msg->um_chan_dxa_uuid)) {
					resp_msg.um_chan_dxa_uuid_len = strlen(setup_p->uuid);
					memcpy(resp_msg.um_chan_dxa_uuid, setup_p->uuid, resp_msg.um_chan_dxa_uuid_len);
					resp_msg.um_peer_uuid_len = strlen(setup_p->peer_uuid);
					memcpy(resp_msg.um_peer_uuid, setup_p->peer_uuid, resp_msg.um_peer_uuid_len);
					resp_msg.um_ip_len = strlen(setup_p->ip);
					memcpy(resp_msg.um_ip, setup_p->ip, resp_msg.um_ip_len);
					resp_msg.um_dxt_name_len = strlen(setup_p->dxt_name);
					memcpy(resp_msg.um_dxt_name, setup_p->dxt_name, resp_msg.um_dxt_name_len);

					msghdr = (struct umessage_tx_hdr *)&resp_msg;
					msghdr->um_req = UMSG_DXA_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_DXA_CODE_W_RESP_DISP;
					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					util_nw_write_n(sfd, msg_buf, cmd_len);
					util_nw_read_n(sfd, &nothing_back, 1);
				}
			}
		}

		break;

	case UMSG_DXA_CODE_W_DISP_ALL:
		setup_p = dxag_p->dxg_op->dxg_get_all_dxa_setup(dxag_p, &num_dxa);
		working_dxa = dxag_p->dxg_op->dxg_get_working_dxa_name(dxag_p, &num_working_dxa);
		for (i = 0; i < num_working_dxa; i++) {
			cur_setup_p = setup_p;
			for (j = 0; j < num_dxa; j++, cur_setup_p++) {
				if (!strcmp(working_dxa[i], cur_setup_p->uuid)) {
					resp_msg.um_chan_dxa_uuid_len = strlen(cur_setup_p->uuid);
					memcpy(resp_msg.um_chan_dxa_uuid, cur_setup_p->uuid, resp_msg.um_chan_dxa_uuid_len);
					resp_msg.um_peer_uuid_len = strlen(cur_setup_p->peer_uuid);
					memcpy(resp_msg.um_peer_uuid, cur_setup_p->peer_uuid, resp_msg.um_peer_uuid_len);
					resp_msg.um_ip_len = strlen(cur_setup_p->ip);
					memcpy(resp_msg.um_ip, cur_setup_p->ip, resp_msg.um_ip_len);
					resp_msg.um_dxt_name_len = strlen(cur_setup_p->dxt_name);
					memcpy(resp_msg.um_dxt_name, cur_setup_p->dxt_name, resp_msg.um_dxt_name_len);

					msghdr = (struct umessage_tx_hdr *)&resp_msg;
					msghdr->um_req = UMSG_DXA_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_DXA_CODE_W_RESP_DISP_ALL;
					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					util_nw_write_n(sfd, msg_buf, cmd_len);
				}
			}
		}

		memset(&resp_msg, 0, sizeof(resp_msg));
		msghdr = (struct umessage_tx_hdr *)&resp_msg;
		msghdr->um_req = UMSG_DXA_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_DXA_CODE_DISP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		util_nw_write_n(sfd, msg_buf, cmd_len);
		util_nw_read_n(sfd, &nothing_back, 1);

		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

out:
	free(working_dxa);
}

static void
__dxac_hello(struct dxac_private *priv_p, char *msg_buf, struct umessage_tx_hdr *msghdr)
{
	char *log_header = "__dxac_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_sfd;
	uint32_t cmd_len;
	int ctype = NID_CTYPE_DXA, rc;
	char nothing_back;
	struct umessage_dxa_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	assert(msghdr->um_req == UMSG_DXA_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TX_HEADER_LEN);

	if (cmd_len > UMSG_TX_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TX_HEADER_LEN, cmd_len - UMSG_TX_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_DXA_CODE_HELLO:
		msghdr = (struct umessage_tx_hdr *)&hello_resp;
		msghdr->um_req = UMSG_DXA_CMD_HELLO;
		msghdr->um_req_code = UMSG_DXA_CODE_RESP_HELLO;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}

		util_nw_write_n(sfd, msg_buf, cmd_len);
		util_nw_read_n(sfd, &nothing_back, 1);

		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}
out:
	return;
}

static void
dxac_do_channel(struct dxac_interface *dxac_p)
{
	char *log_header = "dxac_do_channel";
	struct dxac_private *priv_p = (struct dxac_private *)dxac_p->dxc_private;
	int sfd = priv_p->p_sfd;
	char msg_buf[4096];
	union umessage_dxa dxa_msg;
	struct umessage_tx_hdr *msghdr = (struct umessage_tx_hdr *)&dxa_msg;
	int nread;

	nid_log_warning("%s: start ...", log_header);

	nread = util_nw_read_n(sfd, msg_buf, UMSG_TX_HEADER_LEN);
	if (nread != UMSG_TX_HEADER_LEN) {
		nid_log_warning("%s: failed to read socket", log_header);
		goto out;
	}

	// decode message header
	uint32_t dlen = (uint32_t)nread;
	decode_tx_hdr(msg_buf, &dlen, msghdr);

	if (!priv_p->p_dxag) {
		nid_log_warning("%s support_dxa should be set to 1 to support this operation", log_header);
		goto out;
	}

	switch (msghdr->um_req) {
	case UMSG_DXA_CMD_DROP_CHANNEL:
		__dxac_drop_channel(priv_p, msg_buf, msghdr);
		break;

	case UMSG_DXA_CMD_DISPLAY:
		__dxac_display(priv_p, msg_buf, (struct umessage_dxa_display *)msghdr);
		break;

	case UMSG_DXA_CMD_HELLO:
		__dxac_hello(priv_p, msg_buf, msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
		break;
	}

out:
	nid_log_warning("%s: close control socket %d ...", log_header, sfd);
	close(sfd);
}

static void
dxac_cleanup(struct dxac_interface *dxac_p)
{
	nid_log_debug("dxac_cleanup start, dxac_p:%p", dxac_p);
	if (dxac_p->dxc_private != NULL) {
		free(dxac_p->dxc_private);
		dxac_p->dxc_private = NULL;
	}
}

struct dxac_operations dxac_op = {
	.dxc_accept_new_channel = dxac_accept_new_channel,
	.dxc_do_channel = dxac_do_channel,
	.dxc_cleanup = dxac_cleanup,
};

int
dxac_initialization(struct dxac_interface *dxac_p, struct dxac_setup *setup)
{
	char *log_header = "dxac_initialization";
	struct dxac_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = calloc(1, sizeof(*priv_p));
	dxac_p->dxc_private = priv_p;
	dxac_p->dxc_op = &dxac_op;

	priv_p->p_sfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_dxacg = setup->dxacg;
	priv_p->p_dxag = setup->dxag;
	return 0;
}
