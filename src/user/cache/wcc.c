/*
 * wcc.c
 * 	Implemantation	of Write Cache Channel Module
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
#include "umpk_wc_if.h"
#include "sac_if.h"
#include "tp_if.h"
#include "bio_if.h"
#include "wc_if.h"
#include "wccg_if.h"
#include "wcc_if.h"

struct wcc_private {
	struct wccg_interface	*p_wccg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
wcc_accept_new_channel(struct wcc_interface *wcc_p, int sfd)
{
	struct wcc_private *priv_p = wcc_p->w_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__wcc_information(struct wcc_private *priv_p, char *msg_buf, struct umessage_wc_information *info_msg)
{
	char *log_header = "__wcc_information";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct wccg_interface *wccg_p = priv_p->p_wccg;
	struct wc_interface *wc_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_wc_hdr *msghdr;
	int ctype = NID_CTYPE_WC, rc;
	char nothing_back;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_wc_hdr *)info_msg;
	assert(msghdr->um_req == UMSG_WC_CMD_INFORMATION);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_WC_HEADER_LEN);
	if (cmd_len > UMSG_WC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_WC_HEADER_LEN, cmd_len - UMSG_WC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, info_msg->um_uuid);
	wc_p = wccg_p->wcg_op->wcg_get_wc(wccg_p, info_msg->um_uuid);
	switch (msghdr->um_req_code) {
	case UMSG_WC_CODE_FLUSHING:
		nid_log_warning("%s: got UMSG_WC_CODE_FLUSHING, uuid:%s",  log_header, info_msg->um_uuid);
		break;

	case UMSG_WC_CODE_STAT: {
		struct umessage_wc_information_resp_stat info_stat;
		nid_log_warning("%s: got UMSG_WC_CODE_STAT, uuid:%s",  log_header, info_msg->um_uuid);
		if (wc_p) {
	
			wc_p->wc_op->wc_get_info_stat(wc_p, &info_stat);
			info_stat.wc_uuid_len  = info_msg->um_uuid_len;
			memcpy(info_stat.wc_uuid, info_msg->um_uuid, info_msg->um_uuid_len);
			msghdr = (struct umessage_wc_hdr *)&info_stat;
			msghdr->um_req = UMSG_WC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_WC_CODE_RESP_STAT;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			nid_log_warning("%s: seq_assigned:%lu, seq_flushed:%lu, char2:%d char3:%d char4:%d char5:%d",
				log_header, info_stat.wc_seq_assigned, info_stat.wc_seq_flushed,
				msg_buf[2], msg_buf[3], msg_buf[4], msg_buf[5]);
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;
	}

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

out:
	close(sfd);
}

static void
__wcc_hello(struct wcc_private *priv_p, char *msg_buf, struct umessage_wc_hello *hello_msg)
{
	char *log_header = "__wcc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_wc_hdr *msghdr;
	int ctype = NID_CTYPE_WC, rc;
	char nothing_back;
	struct umessage_wc_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_wc_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_WC_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_WC_HEADER_LEN);

	if (cmd_len > UMSG_WC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_WC_HEADER_LEN, cmd_len - UMSG_WC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_WC_CODE_HELLO:
		msghdr = (struct umessage_wc_hdr *)&hello_resp;
		msghdr->um_req = UMSG_WC_CMD_HELLO;
		msghdr->um_req_code = UMSG_WC_CODE_RESP_HELLO;

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
	close(sfd);

}

static void
wcc_do_channel(struct wcc_interface *wcc_p)
{
	char *log_header = "wcc_do_channel";
	struct wcc_private *priv_p = (struct wcc_private *)wcc_p->w_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_wc wc_msg; 
	struct umessage_wc_hdr *msghdr = (struct umessage_wc_hdr *)&wc_msg;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_WC_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	switch (msghdr->um_req) {
	case UMSG_WC_CMD_INFORMATION:
		__wcc_information(priv_p, msg_buf, (struct umessage_wc_information *)msghdr);
		break;

	case UMSG_WC_CMD_HELLO:
		__wcc_hello(priv_p, msg_buf, (struct umessage_wc_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
	}
}

static void
wcc_cleanup(struct wcc_interface *wcc_p)
{
	nid_log_debug("wcc_cleanup start, wcc_p:%p", wcc_p);
	if (wcc_p->w_private != NULL) {
		free(wcc_p->w_private);
		wcc_p->w_private = NULL;
	}
}

struct wcc_operations wcc_op = {
	.w_accept_new_channel = wcc_accept_new_channel,
	.w_do_channel = wcc_do_channel,
	.w_cleanup = wcc_cleanup,
};

int
wcc_initialization(struct wcc_interface *wcc_p, struct wcc_setup *setup)
{
	char *log_header = "wcc_initialization";
	struct wcc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	wcc_p->w_private = priv_p;
	wcc_p->w_op = &wcc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_wccg = setup->wccg;
	return 0;
}
