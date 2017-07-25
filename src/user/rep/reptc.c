/*
 * reptc.c
 *	Implementation of Replication Target Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "util_nw.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_rept_if.h"
#include "rept_if.h"
#include "reptg_if.h"
#include "reptc_if.h"
#include "lstn_if.h"

struct reptc_private {
	struct reptg_interface	*p_reptg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
reptc_accept_new_channel(struct reptc_interface *rtc_p, int sfd)
{
	char *log_header = "reptc_accept_new_channel";
	struct reptc_private *priv_p = rtc_p->r_private;

	nid_log_debug("%s: start", log_header);
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__reptc_display(struct reptc_private *priv_p, char *msg_buf, struct umessage_rept_display *dis_msg)
{
	char *log_header = "__reptc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct reptg_interface *reptg_p = priv_p->p_reptg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_rept_hdr *msghdr;
	int ctype = NID_CTYPE_REPT, rc;
	char nothing_back;
	struct umessage_rept_display_resp dis_resp;
	struct rept_setup *rept_setup_p, *cur_setup_p;;
	struct rept_interface *rept_p;
	char **working_rept = NULL;
	int num_rept = 0, num_working_rept = 0, i, j;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_rept_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_REPT_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPT_HEADER_LEN);

	if (cmd_len > UMSG_REPT_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPT_HEADER_LEN, cmd_len - UMSG_REPT_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, rt_name: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_rt_name);

	switch (msghdr->um_req_code){
	case UMSG_REPT_CODE_S_DISP:
		if (dis_msg->um_rt_name == NULL)
			goto out;
		rept_setup_p = reptg_p->rtg_op->rtg_get_all_rt_setup(reptg_p, &num_rept);
		if (rept_setup_p == NULL)
			goto out;
		for (i = 0; i < num_rept; i++, rept_setup_p++) {
			if (!strcmp(dis_msg->um_rt_name, rept_setup_p->rt_name)) {
				dis_resp.um_rt_name_len = strlen(rept_setup_p->rt_name);
				memcpy(dis_resp.um_rt_name, rept_setup_p->rt_name, dis_resp.um_rt_name_len);
				dis_resp.um_dxp_name_len = strlen(rept_setup_p->dxp_name);
				memcpy(dis_resp.um_dxp_name, rept_setup_p->dxp_name , dis_resp.um_dxp_name_len);
				dis_resp.um_voluuid_len = strlen(rept_setup_p->voluuid);
				memcpy(dis_resp.um_voluuid, rept_setup_p->voluuid, dis_resp.um_voluuid_len);
				dis_resp.um_bitmap_len = rept_setup_p->bitmap_len;
				dis_resp.um_vol_len = rept_setup_p->vol_len;

				msghdr = (struct umessage_rept_hdr *)&dis_resp;
				msghdr->um_req = UMSG_REPT_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_REPT_CODE_S_RESP_DISP;

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

	case UMSG_REPT_CODE_S_DISP_ALL:
		rept_setup_p = reptg_p->rtg_op->rtg_get_all_rt_setup(reptg_p, &num_rept);
		for (i = 0; i < num_rept; i++, rept_setup_p++) {
			dis_resp.um_rt_name_len = strlen(rept_setup_p->rt_name);
			memcpy(dis_resp.um_rt_name, rept_setup_p->rt_name, dis_resp.um_rt_name_len);
			dis_resp.um_dxp_name_len = strlen(rept_setup_p->dxp_name);
			memcpy(dis_resp.um_dxp_name, rept_setup_p->dxp_name , dis_resp.um_dxp_name_len);
			dis_resp.um_voluuid_len = strlen(rept_setup_p->voluuid);
			memcpy(dis_resp.um_voluuid, rept_setup_p->voluuid, dis_resp.um_voluuid_len);
			dis_resp.um_bitmap_len = rept_setup_p->bitmap_len;
			dis_resp.um_vol_len = rept_setup_p->vol_len;

			msghdr = (struct umessage_rept_hdr *)&dis_resp;
			msghdr->um_req = UMSG_REPT_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_REPT_CODE_S_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_rept_hdr *)&dis_resp;
		msghdr->um_req = UMSG_REPT_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_REPT_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_REPT_CODE_W_DISP:
		if (dis_msg->um_rt_name == NULL)
			goto out;
		rept_p = reptg_p->rtg_op->rtg_search_rt(reptg_p, dis_msg->um_rt_name);
		if (rept_p == NULL)
			goto out;
		rept_setup_p = reptg_p->rtg_op->rtg_get_all_rt_setup(reptg_p, &num_rept);
		for (i = 0; i < num_rept; i++, rept_setup_p++) {
			if (!strcmp(dis_msg->um_rt_name, rept_setup_p->rt_name)) {
				dis_resp.um_rt_name_len = strlen(rept_setup_p->rt_name);
				memcpy(dis_resp.um_rt_name, rept_setup_p->rt_name, dis_resp.um_rt_name_len);
				dis_resp.um_dxp_name_len = strlen(rept_setup_p->dxp_name);
				memcpy(dis_resp.um_dxp_name, rept_setup_p->dxp_name , dis_resp.um_dxp_name_len);
				dis_resp.um_voluuid_len = strlen(rept_setup_p->voluuid);
				memcpy(dis_resp.um_voluuid, rept_setup_p->voluuid, dis_resp.um_voluuid_len);
				dis_resp.um_bitmap_len = rept_setup_p->bitmap_len;
				dis_resp.um_vol_len = rept_setup_p->vol_len;

				msghdr = (struct umessage_rept_hdr *)&dis_resp;
				msghdr->um_req = UMSG_REPT_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_REPT_CODE_W_RESP_DISP;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				write(sfd, msg_buf, cmd_len);
				read(sfd, &nothing_back, 1);
			}
		}
		break;

	case UMSG_REPT_CODE_W_DISP_ALL:
		working_rept = reptg_p->rtg_op->rtg_get_working_rt_name(reptg_p, &num_working_rept);
		rept_setup_p = reptg_p->rtg_op->rtg_get_all_rt_setup(reptg_p, &num_rept);
		for (i = 0; i < num_working_rept; i++) {
			cur_setup_p = rept_setup_p;
			for (j = 0; j < num_rept; j++, cur_setup_p++) {
				if (!strcmp(working_rept[i], cur_setup_p->rt_name)) {
					dis_resp.um_rt_name_len = strlen(cur_setup_p->rt_name);
					memcpy(dis_resp.um_rt_name, cur_setup_p->rt_name, dis_resp.um_rt_name_len);
					dis_resp.um_dxp_name_len = strlen(cur_setup_p->dxp_name);
					memcpy(dis_resp.um_dxp_name, cur_setup_p->dxp_name , dis_resp.um_dxp_name_len);
					dis_resp.um_voluuid_len = strlen(cur_setup_p->voluuid);
					memcpy(dis_resp.um_voluuid, cur_setup_p->voluuid, dis_resp.um_voluuid_len);
					dis_resp.um_bitmap_len = cur_setup_p->bitmap_len;
					dis_resp.um_vol_len = cur_setup_p->vol_len;

					msghdr = (struct umessage_rept_hdr *)&dis_resp;
					msghdr->um_req = UMSG_REPT_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_REPT_CODE_W_RESP_DISP_ALL;

					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					write(sfd, msg_buf, cmd_len);
				}
			}
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_rept_hdr *)&dis_resp;
		msghdr->um_req = UMSG_REPT_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_REPT_CODE_DISP_END;

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
	free(working_rept);
	close(sfd);
}

static void
__reptc_hello(struct reptc_private *priv_p, char *msg_buf, struct umessage_rept_hello *hello_msg)
{
	char *log_header = "__reptc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_rept_hdr *msghdr;
	int ctype = NID_CTYPE_REPT, rc;
	char nothing_back;
	struct umessage_rept_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_rept_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_REPT_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPT_HEADER_LEN);

	if (cmd_len > UMSG_REPT_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPT_HEADER_LEN, cmd_len - UMSG_REPT_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_REPT_CODE_HELLO:
		msghdr = (struct umessage_rept_hdr *)&hello_resp;
		msghdr->um_req = UMSG_REPT_CMD_HELLO;
		msghdr->um_req_code = UMSG_REPT_CODE_RESP_HELLO;

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
__reptc_start(struct reptc_private *priv_p, char *msg_buf, struct umessage_rept_start *start_msg)
{
	char *log_header = "__reptc_start";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct reptg_interface	*reptg = priv_p->p_reptg;
	struct rept_interface	*rept;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_rept_hdr *msghdr;
	int ctype = NID_CTYPE_REPT, rc;
	char nothing_back;
	struct umessage_rept_start start_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_rept_hdr *)start_msg;
	assert(msghdr->um_req == UMSG_REPT_CMD_START);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPT_HEADER_LEN);

	if (cmd_len > UMSG_REPT_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPT_HEADER_LEN, cmd_len - UMSG_REPT_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}
	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	rept = reptg->rtg_op->rtg_search_and_create_rt(reptg, start_msg->um_rt_name);
	rept->rt_op->rt_start(rept);

	switch (msghdr->um_req_code) {
	case UMSG_REPT_CODE_START:
		msghdr = (struct umessage_rept_hdr *)&start_resp;
		msghdr->um_req = UMSG_REPT_CMD_START;
		msghdr->um_req_code = UMSG_REPT_CODE_RESP_START;

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
__reptc_info(struct reptc_private *priv_p, char *msg_buf, struct umessage_rept_info *info_msg)
{
	char *log_header = "__reptc_info";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct reptg_interface *reptg_p = priv_p->p_reptg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_rept_hdr *msghdr;
	int ctype = NID_CTYPE_REPT, rc;
	char nothing_back;
	struct rept_interface *rept_p;
	struct umessage_rept_info_pro_resp info_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_rept_hdr *)info_msg;
	assert(msghdr->um_req == UMSG_REPT_CMD_INFO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPT_HEADER_LEN);

	if (cmd_len > UMSG_REPT_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPT_HEADER_LEN, cmd_len - UMSG_REPT_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, rept_name:%s", log_header, cmd_len, msghdr->um_req_code, info_msg->um_rt_name);

	rept_p = reptg_p->rtg_op->rtg_search_and_create_rt(reptg_p, info_msg->um_rt_name);
	if (rept_p == NULL)
		goto out;

	switch (msghdr->um_req_code) {
	case UMSG_REPT_CODE_INFO_PRO:
		msghdr = (struct umessage_rept_hdr *)&info_resp;
		msghdr->um_req = UMSG_REPT_CMD_INFO;
		msghdr->um_req_code = UMSG_REPT_CODE_RESP_INFO_PRO;

		info_resp.um_progress = rept_p->rt_op->rt_get_progress(rept_p);

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
reptc_do_channel(struct reptc_interface *reptc_p)
{
	char *log_header = "reptc_do_channel";
	struct reptc_private *priv_p = (struct reptc_private *)reptc_p->r_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_rept rept_msg;
	struct umessage_rept_hdr *msghdr_p;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_REPT_HEADER_LEN);

	msghdr_p = (struct umessage_rept_hdr *) &rept_msg;
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = *(uint32_t *)p;
	nid_log_warning("%s: req:%d, req_code:%d, um_len:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code, msghdr_p->um_len);

	if (!priv_p->p_reptg) {
		nid_log_warning("%s support_rept should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr_p->um_req) {
	case UMSG_REPT_CMD_HELLO:
		__reptc_hello(priv_p, msg_buf, (struct umessage_rept_hello *)msghdr_p);
		break;

	case UMSG_REPT_CMD_START:
		__reptc_start(priv_p, msg_buf, (struct umessage_rept_start *)msghdr_p);
		break;

	case UMSG_REPT_CMD_DISPLAY:
		__reptc_display(priv_p, msg_buf, (struct umessage_rept_display *)msghdr_p);
		break;

	case UMSG_REPT_CMD_INFO:
		__reptc_info(priv_p, msg_buf, (struct umessage_rept_info *)msghdr_p);
		break;

	default:
		nid_log_warning("%s: got wrong req:%d", log_header, msghdr_p->um_req);
		break;
	}
}

static void
reptc_cleanup(struct reptc_interface *reptc_p)
{
	char *log_header = "reptc_cleanup";

	nid_log_debug("%s start, reptc_p: %p", log_header, reptc_p);
	if (reptc_p->r_private != NULL) {
		free(reptc_p->r_private);
		reptc_p->r_private = NULL;
	}
}

struct reptc_operations reptc_op = {
	.rtc_accept_new_channel = reptc_accept_new_channel,
	.rtc_do_channel = reptc_do_channel,
	.rtc_cleanup = reptc_cleanup,
};

int
reptc_initialization(struct reptc_interface *reptc_p, struct reptc_setup *setup)
{
	char *log_header = "reptc_initialization";
	struct reptc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = calloc(1, sizeof(*priv_p));
	reptc_p->r_private = priv_p;
	reptc_p->r_op = &reptc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_reptg = setup->reptg;

	return 0;
}
