/*
 * cxac.c
 * 	Implementation of Control Exchange Active Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "util_nw.h"
#include "list.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_cxa_if.h"
#include "umpk_cxp_if.h"
#include "sac_if.h"
#include "tp_if.h"
#include "bio_if.h"
#include "cxa_if.h"
#include "cxag_if.h"
#include "cxacg_if.h"
#include "cxac_if.h"

struct cxac_private {
	struct cxacg_interface	*p_cxacg;
	struct cxag_interface	*p_cxag;
	struct umpk_interface	*p_umpk;
	int			p_sfd;
};

static void
__cxac_display(struct cxac_private *priv_p, char *msg_buf, struct umessage_cxa_display *dis_msg)
{
	char *log_header = "__cxac_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct cxag_interface *cxag_p =  priv_p->p_cxag;
	struct umessage_cxa_display_resp resp_msg;
	struct cxa_setup *setup_p, *cur_setup_p;
	int sfd = priv_p->p_sfd;
	uint32_t cmd_len;
	struct umessage_tx_hdr *msghdr;
	int ctype = NID_CTYPE_CXA, rc;
	char nothing_back;
	int num_cxa, num_working_cxa, i, j;
	char **working_cxa = NULL;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_tx_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_CXA_CMD_DISPLAY);
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

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, cxa_uuid:%s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_chan_cxa_uuid);
memset(&resp_msg, 0, sizeof(resp_msg));
	switch (msghdr->um_req_code) {
	case UMSG_CXA_CODE_S_DISP:
		setup_p = cxag_p->cxg_op->cxg_get_all_cxa_setup(cxag_p, &num_cxa);
		for (i = 0; i < num_cxa; i++, setup_p++) {
			if (!strcmp(setup_p->uuid, dis_msg->um_chan_cxa_uuid)) {
				resp_msg.um_chan_cxa_uuid_len = strlen(setup_p->uuid);
				memcpy(resp_msg.um_chan_cxa_uuid, setup_p->uuid, resp_msg.um_chan_cxa_uuid_len);
				resp_msg.um_peer_uuid_len = strlen(setup_p->peer_uuid);
				memcpy(resp_msg.um_peer_uuid, setup_p->peer_uuid, resp_msg.um_peer_uuid_len);
				resp_msg.um_ip_len = strlen(setup_p->ip);
				memcpy(resp_msg.um_ip, setup_p->ip, resp_msg.um_ip_len);
				resp_msg.um_cxt_name_len = strlen(setup_p->cxt_name);
				memcpy(resp_msg.um_cxt_name, setup_p->cxt_name, resp_msg.um_cxt_name_len);

				msghdr = (struct umessage_tx_hdr *)&resp_msg;
				msghdr->um_req = UMSG_CXA_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_CXA_CODE_S_RESP_DISP;
				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				write(sfd, msg_buf, cmd_len);
				read(sfd, &nothing_back, 1);
				break;
			}
		}

		break;

	case UMSG_CXA_CODE_S_DISP_ALL:
		setup_p = cxag_p->cxg_op->cxg_get_all_cxa_setup(cxag_p, &num_cxa);
		for (i = 0; i < num_cxa; i++, setup_p++) {
			resp_msg.um_chan_cxa_uuid_len = strlen(setup_p->uuid);
			memcpy(resp_msg.um_chan_cxa_uuid, setup_p->uuid, resp_msg.um_chan_cxa_uuid_len);
			resp_msg.um_peer_uuid_len = strlen(setup_p->peer_uuid);
			memcpy(resp_msg.um_peer_uuid, setup_p->peer_uuid, resp_msg.um_peer_uuid_len);
			resp_msg.um_ip_len = strlen(setup_p->ip);
			memcpy(resp_msg.um_ip, setup_p->ip, resp_msg.um_ip_len);
			resp_msg.um_cxt_name_len = strlen(setup_p->cxt_name);
			memcpy(resp_msg.um_cxt_name, setup_p->cxt_name, resp_msg.um_cxt_name_len);

			msghdr = (struct umessage_tx_hdr *)&resp_msg;
			msghdr->um_req = UMSG_CXA_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_CXA_CODE_S_RESP_DISP_ALL;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&resp_msg, 0, sizeof(resp_msg));
		msghdr = (struct umessage_tx_hdr *)&resp_msg;
		msghdr->um_req = UMSG_CXA_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_CXA_CODE_DISP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);	
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_CXA_CODE_W_DISP:
		working_cxa = cxag_p->cxg_op->cxg_get_working_cxa_name(cxag_p, &num_working_cxa);
		for (i = 0; i < num_working_cxa; i++) {
			if (!strcmp(working_cxa[i], dis_msg->um_chan_cxa_uuid)) {
				break;
			}
		}

		if (i < num_working_cxa) {
			setup_p = cxag_p->cxg_op->cxg_get_all_cxa_setup(cxag_p, &num_cxa);
			for (i = 0; i < num_cxa; i++, setup_p++) {
				if (!strcmp(setup_p->uuid, dis_msg->um_chan_cxa_uuid)) {
					resp_msg.um_chan_cxa_uuid_len = strlen(setup_p->uuid);
					memcpy(resp_msg.um_chan_cxa_uuid, setup_p->uuid, resp_msg.um_chan_cxa_uuid_len);
					resp_msg.um_peer_uuid_len = strlen(setup_p->peer_uuid);
					memcpy(resp_msg.um_peer_uuid, setup_p->peer_uuid, resp_msg.um_peer_uuid_len);
					resp_msg.um_ip_len = strlen(setup_p->ip);
					memcpy(resp_msg.um_ip, setup_p->ip, resp_msg.um_ip_len);
					resp_msg.um_cxt_name_len = strlen(setup_p->cxt_name);
					memcpy(resp_msg.um_cxt_name, setup_p->cxt_name, resp_msg.um_cxt_name_len);

					msghdr = (struct umessage_tx_hdr *)&resp_msg;
					msghdr->um_req = UMSG_CXA_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_CXA_CODE_W_RESP_DISP;
					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					write(sfd, msg_buf, cmd_len);
					read(sfd, &nothing_back, 1);
				}
			}
		}

		break;

	case UMSG_CXA_CODE_W_DISP_ALL:
		setup_p = cxag_p->cxg_op->cxg_get_all_cxa_setup(cxag_p, &num_cxa);
		working_cxa = cxag_p->cxg_op->cxg_get_working_cxa_name(cxag_p, &num_working_cxa);
		for (i = 0; i < num_working_cxa; i++) {
			cur_setup_p = setup_p;
			for (j = 0; j < num_cxa; j++, cur_setup_p++) {
				if (!strcmp(working_cxa[i], cur_setup_p->uuid)) {
					resp_msg.um_chan_cxa_uuid_len = strlen(cur_setup_p->uuid);
					memcpy(resp_msg.um_chan_cxa_uuid, cur_setup_p->uuid, resp_msg.um_chan_cxa_uuid_len);
					resp_msg.um_peer_uuid_len = strlen(cur_setup_p->peer_uuid);
					memcpy(resp_msg.um_peer_uuid, cur_setup_p->peer_uuid, resp_msg.um_peer_uuid_len);
					resp_msg.um_ip_len = strlen(cur_setup_p->ip);
					memcpy(resp_msg.um_ip, cur_setup_p->ip, resp_msg.um_ip_len);
					resp_msg.um_cxt_name_len = strlen(cur_setup_p->cxt_name);
					memcpy(resp_msg.um_cxt_name, cur_setup_p->cxt_name, resp_msg.um_cxt_name_len);

					msghdr = (struct umessage_tx_hdr *)&resp_msg;
					msghdr->um_req = UMSG_CXA_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_CXA_CODE_W_RESP_DISP_ALL;
					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					write(sfd, msg_buf, cmd_len);
				}
			}
		}

		memset(&resp_msg, 0, sizeof(resp_msg));
		msghdr = (struct umessage_tx_hdr *)&resp_msg;
		msghdr->um_req = UMSG_CXA_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_CXA_CODE_DISP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);	
		read(sfd, &nothing_back, 1);

		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

out:
	free(working_cxa);
}

static void
__cxac_hello(struct cxac_private *priv_p, char *msg_buf, struct umessage_cxa_hello *hello_msg)
{
	char *log_header = "__cxac_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_sfd;
	uint32_t cmd_len;
	struct umessage_tx_hdr *msghdr = (struct umessage_tx_hdr *)hello_msg;
	int ctype = NID_CTYPE_CXA, rc;
	char nothing_back;
	struct umessage_cxa_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	assert(msghdr->um_req == UMSG_CXA_CMD_HELLO);
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
	case UMSG_CXA_CODE_HELLO:
		msghdr = (struct umessage_tx_hdr *)&hello_resp;
		msghdr->um_req = UMSG_CXA_CMD_HELLO;
		msghdr->um_req_code = UMSG_CXA_CODE_RESP_HELLO;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}

		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

out:
	return;
}

static int
cxac_accept_new_channel(struct cxac_interface *cxac_p, int sfd)
{
	nid_log_debug("cxac_accept_new_channel start ...");
	struct cxac_private *priv_p = cxac_p->cxc_private;
	priv_p->p_sfd = sfd;
	return 0;
}

static void
cxac_do_channel(struct cxac_interface *cxac_p)
{
	char *log_header = "cxac_do_channel";
	struct cxac_private *priv_p = (struct cxac_private *)cxac_p->cxc_private;
	int sfd = priv_p->p_sfd;
	char msg_buf[4096];
	union umessage_cxa cxa_msg;
	struct umessage_tx_hdr *msghdr = (struct umessage_tx_hdr *)&cxa_msg;
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

	if (!priv_p->p_cxag) {
		nid_log_warning("%s support_cxa should be set to 1 to support this operation", log_header);
		goto out;
	}

	switch (msghdr->um_req) {
	case UMSG_CXA_CMD_DISPLAY:
		__cxac_display(priv_p, msg_buf, (struct umessage_cxa_display *)msghdr);
		break;

	case UMSG_CXA_CMD_HELLO:
		__cxac_hello(priv_p, msg_buf, (struct umessage_cxa_hello *)msghdr);
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
cxac_cleanup(struct cxac_interface *cxac_p)
{
	nid_log_debug("cxac_cleanup start, cxac_p:%p", cxac_p);
	if (cxac_p->cxc_private != NULL) {
		free(cxac_p->cxc_private);
		cxac_p->cxc_private = NULL;
	}
}

struct cxac_operations cxac_op = {
	.cxc_accept_new_channel = cxac_accept_new_channel,
	.cxc_do_channel = cxac_do_channel,
	.cxc_cleanup = cxac_cleanup,
};

int
cxac_initialization(struct cxac_interface *cxac_p, struct cxac_setup *setup)
{
	char *log_header = "cxac_initialization";
	struct cxac_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = calloc(1, sizeof(*priv_p));
	cxac_p->cxc_private = priv_p;
	cxac_p->cxc_op = &cxac_op;

	priv_p->p_sfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_cxacg = setup->cxacg;
	priv_p->p_cxag =setup->cxag;
	return 0;
}
