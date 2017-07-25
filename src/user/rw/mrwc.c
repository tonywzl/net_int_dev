/*
 * mrwc.c
 * 	Implementation of Meta Server Read Write Guardian Module
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
#include "umpk_mrw_if.h"
#include "sac_if.h"
#include "tp_if.h"
#include "bio_if.h"
#include "rw_if.h"
#include "mrw_if.h"
#include "mrwcg_if.h"
#include "mrwc_if.h"
#include "mrwg_if.h"


struct mrwc_private {
	struct mrwcg_interface	*p_mrwcg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
mrwc_accept_new_channel(struct mrwc_interface *mrwc_p, int sfd)
{
	nid_log_debug("mrwc_accept_new_channel start ...");
	struct mrwc_private *priv_p = mrwc_p->w_private;
	priv_p->p_rsfd = sfd;
	return 0;
}


static void
__mrwc_information(struct mrwc_private *priv_p, char *msg_buf, struct umessage_mrw_information *info_msg)
{
	char *log_header = "__mrwc_information";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct mrwcg_interface *mrwcg_p = priv_p->p_mrwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_mrw_hdr *msghdr;
	int ctype = NID_CTYPE_MRW, rc;
	char nothing_back;
	struct mrwg_interface *mrwg_p;
	char *mrw_name;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_mrw_hdr *)info_msg;
	assert(msghdr->um_req == UMSG_MRW_CMD_INFORMATION);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_MRW_HEADER_LEN);
	if (cmd_len > UMSG_MRW_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_MRW_HEADER_LEN, cmd_len - UMSG_MRW_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d",
		log_header, cmd_len, msghdr->um_req_code);
	mrw_name = info_msg->um_mrw_name;
	mrwg_p = mrwcg_p->mrwcg_op->mrwcg_get_mrwg(mrwcg_p);
	if (!mrwg_p)
		goto out;

	switch (msghdr->um_req_code) {
	case UMSG_MRW_CODE_STAT: {
		struct umessage_mrw_information_stat_resp info_stat;
		struct mrw_stat  info;
		nid_log_warning("%s: got UMSG_MRW_CODE_STAT",  log_header);
		mrwg_p->rwg_op->rwg_get_mrw_status(mrwg_p, mrw_name, &info);
		info_stat.um_seq_wfp_num = info.sent_wfp_num;
		info_stat.um_seq_wop_num = info.sent_wop_num;
		info_stat.um_seq_mrw =1;
		msghdr = (struct umessage_mrw_hdr *)&info_stat;
		msghdr->um_req = UMSG_MRW_CMD_INFORMATION;
		msghdr->um_req_code = UMSG_MRW_CODE_RESP_STAT;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		nid_log_warning("%s: wfp num:%lu, wop num:%lu, char2:%d char3:%d char4:%d char5:%d",
			log_header, info_stat.um_seq_wfp_num, info_stat.um_seq_wop_num,
			msg_buf[2], msg_buf[3], msg_buf[4], msg_buf[5]);
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);
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
__mrwc_add_mrw(struct mrwc_private *priv_p, char *msg_buf, struct umessage_mrw_add *add_msg)
{
	char *log_header = "__mrwc_add_mrw";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct mrwcg_interface *mrwcg_p = priv_p->p_mrwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_mrw_hdr *msghdr;
	int ctype = NID_CTYPE_MRW, rc;
	struct umessage_mrw_add_resp add_resp;
	char nothing_back;
	struct mrwg_interface *mrwg_p;
	char *mrw_name;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_mrw_hdr *)add_msg;
	assert(msghdr->um_req == UMSG_MRW_CMD_ADD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_MRW_HEADER_LEN);
	if (cmd_len > UMSG_MRW_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_MRW_HEADER_LEN, cmd_len - UMSG_MRW_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, mrw_name:%s",
		log_header, cmd_len, msghdr->um_req_code, add_msg->um_mrw_name);
	mrw_name = add_msg->um_mrw_name;
	mrwg_p = mrwcg_p->mrwcg_op->mrwcg_get_mrwg(mrwcg_p);

	msghdr = (struct umessage_mrw_hdr *)&add_resp;
	msghdr->um_req = UMSG_MRW_CMD_ADD;
	msghdr->um_req_code = UMSG_MRW_CODE_RESP_ADD;
	memcpy(add_resp.um_mrw_name, add_msg->um_mrw_name, add_msg->um_mrw_name_len);
	add_resp.um_mrw_name_len  = add_msg->um_mrw_name_len;

	add_resp.um_resp_code = mrwg_p->rwg_op->rwg_add_mrw(mrwg_p, mrw_name);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__mrwc_display(struct mrwc_private *priv_p, char *msg_buf, struct umessage_mrw_display *dis_msg)
{
	char *log_header = "__mrwc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct mrwcg_interface *mrwcg_p = priv_p->p_mrwcg;
	struct mrwg_interface *mrwg_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_mrw_hdr *msghdr;
	int ctype = NID_CTYPE_MRW, rc;
	char nothing_back;
	struct umessage_mrw_display dis_resp;
	struct mrw_setup *mrw_setup_p;
	int *working_mrw = NULL, cur_index = 0;
	int num_mrw = 0, num_working_mrw = 0, get_it = 0;
	int i, j;

	nid_log_warning("%s: start ...", log_header);
	mrwg_p = mrwcg_p->mrwcg_op->mrwcg_get_mrwg(mrwcg_p);
	msghdr = (struct umessage_mrw_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_MRW_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_MRW_HEADER_LEN);

	if (cmd_len > UMSG_MRW_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_MRW_HEADER_LEN, cmd_len - UMSG_MRW_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, mrw_name: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_mrw_name);

	switch (msghdr->um_req_code){
	case UMSG_MRW_CODE_S_DISP:
		if (dis_msg->um_mrw_name == NULL)
			goto out;
		mrw_setup_p = mrwg_p->rwg_op->rwg_get_all_mrw_setup(mrwg_p, &num_mrw);
		if (mrw_setup_p == NULL)
			goto out;
		for (i = 0; i < num_mrw; i++, mrw_setup_p++) {
			if (!strcmp(mrw_setup_p->name ,dis_msg->um_mrw_name)) {
				get_it = 1;
				break;
			}
		}
		if (get_it) {
			dis_resp.um_mrw_name_len = strlen(mrw_setup_p->name);
			memcpy(dis_resp.um_mrw_name, mrw_setup_p->name, dis_resp.um_mrw_name_len);

			msghdr = (struct umessage_mrw_hdr *)&dis_resp;
			msghdr->um_req = UMSG_MRW_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_MRW_CODE_S_RESP_DISP;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;

	case UMSG_MRW_CODE_S_DISP_ALL:
		mrw_setup_p = mrwg_p->rwg_op->rwg_get_all_mrw_setup(mrwg_p, &num_mrw);
		for (i = 0; i < num_mrw; i++, mrw_setup_p++) {
			dis_resp.um_mrw_name_len = strlen(mrw_setup_p->name);
			memcpy(dis_resp.um_mrw_name, mrw_setup_p->name, dis_resp.um_mrw_name_len);

			msghdr = (struct umessage_mrw_hdr *)&dis_resp;
			msghdr->um_req = UMSG_MRW_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_MRW_CODE_S_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_mrw_hdr *)&dis_resp;
		msghdr->um_req = UMSG_MRW_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_MRW_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_MRW_CODE_W_DISP:
		if (dis_msg->um_mrw_name == NULL)
			goto out;
		mrw_setup_p = mrwg_p->rwg_op->rwg_get_all_mrw_setup(mrwg_p, &num_mrw);
		working_mrw = mrwg_p->rwg_op->rwg_get_working_mrw_index(mrwg_p, &num_working_mrw);
		for (i = 0; i < num_mrw; i++, mrw_setup_p++) {
			if (!strcmp(mrw_setup_p->name, dis_msg->um_mrw_name))
				break;
		}

		if (i != num_mrw) {
			for (j = 0; j < num_working_mrw; j++) {
				if (working_mrw[j] == i)
					break;
			}
		}
		else {
			goto out;
		}

		if (j != num_working_mrw) {
			dis_resp.um_mrw_name_len = strlen(mrw_setup_p->name);
			memcpy(dis_resp.um_mrw_name, mrw_setup_p->name, dis_resp.um_mrw_name_len);

			msghdr = (struct umessage_mrw_hdr *)&dis_resp;
			msghdr->um_req = UMSG_MRW_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_MRW_CODE_W_RESP_DISP;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		else {
			goto out;
		}
		break;			

	case UMSG_MRW_CODE_W_DISP_ALL:
		mrw_setup_p = mrwg_p->rwg_op->rwg_get_all_mrw_setup(mrwg_p, &num_mrw);
		working_mrw = mrwg_p->rwg_op->rwg_get_working_mrw_index(mrwg_p, &num_working_mrw);

		for (i = 0; i < num_working_mrw; i++) {
			cur_index = working_mrw[i];
			dis_resp.um_mrw_name_len = strlen(mrw_setup_p[cur_index].name);
			memcpy(dis_resp.um_mrw_name, mrw_setup_p[cur_index].name, dis_resp.um_mrw_name_len);

			msghdr = (struct umessage_mrw_hdr *)&dis_resp;
			msghdr->um_req = UMSG_MRW_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_MRW_CODE_W_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_mrw_hdr *)&dis_resp;
		msghdr->um_req = UMSG_MRW_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_MRW_CODE_DISP_END;

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
	free(working_mrw);
	close(sfd);
}

static void
__mrwc_hello(struct mrwc_private *priv_p, char *msg_buf, struct umessage_mrw_hello *hello_msg)
{
	char *log_header = "__mrwc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_mrw_hdr *msghdr;
	int ctype = NID_CTYPE_MRW, rc;
	char nothing_back;
	struct umessage_mrw_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_mrw_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_MRW_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_MRW_HEADER_LEN);

	if (cmd_len > UMSG_MRW_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_MRW_HEADER_LEN, cmd_len - UMSG_MRW_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_MRW_CODE_HELLO:
		msghdr = (struct umessage_mrw_hdr *)&hello_resp;
		msghdr->um_req = UMSG_MRW_CMD_HELLO;
		msghdr->um_req_code = UMSG_MRW_CODE_RESP_HELLO;

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
mrwc_do_channel(struct mrwc_interface *mrwc_p, struct scg_interface *scg_p)
{
	char *log_header = "mrwc_do_channel";
	struct mrwc_private *priv_p = (struct mrwc_private *)mrwc_p->w_private;
	struct mrwcg_interface *mrwcg_p = priv_p->p_mrwcg;
	struct mrwg_interface *mrwg_p;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_mrw mrw_msg;
	struct umessage_mrw_hdr *msghdr = (struct umessage_mrw_hdr *)&mrw_msg;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_MRW_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;

	mrwg_p = mrwcg_p->mrwcg_op->mrwcg_get_mrwg(mrwcg_p);
	if (!mrwg_p) {
		nid_log_warning("%s support_mrw should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr->um_req) {

	case UMSG_MRW_CMD_INFORMATION:
		__mrwc_information(priv_p, msg_buf, (struct umessage_mrw_information *)msghdr);
		break;

	case UMSG_MRW_CMD_ADD:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__mrwc_add_mrw(priv_p, msg_buf, (struct umessage_mrw_add *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_MRW_CMD_DISPLAY:
		__mrwc_display(priv_p, msg_buf, (struct umessage_mrw_display *)msghdr);
		break;

	case UMSG_MRW_CMD_HELLO:
		__mrwc_hello(priv_p, msg_buf, (struct umessage_mrw_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
	}
}

static void
mrwc_cleanup(struct mrwc_interface *mrwc_p)
{
	nid_log_debug("mrwc_cleanup start, mrwc_p:%p", mrwc_p);
	if (mrwc_p->w_private != NULL) {
		free(mrwc_p->w_private);
		mrwc_p->w_private = NULL;
	}
}

struct mrwc_operations mrwc_op = {
	.w_accept_new_channel = mrwc_accept_new_channel,
	.w_do_channel = mrwc_do_channel,
	.w_cleanup = mrwc_cleanup,
};

int
mrwc_initialization(struct mrwc_interface *mrwc_p, struct mrwc_setup *setup)
{
	char *log_header = "mrwc_initialization";
	struct mrwc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	mrwc_p->w_private = priv_p;
	mrwc_p->w_op = &mrwc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_mrwcg = setup->mrwcg;
	return 0;
}
