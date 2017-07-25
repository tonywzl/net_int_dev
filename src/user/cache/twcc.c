/*
 * twcc.c
 * 	Implementation of Write Through Cache (twc) Channel Module	
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
#include "umpk_twc_if.h"
#include "sac_if.h"
#include "tp_if.h"
#include "bio_if.h"
#include "twc_if.h"
#include "twcc_if.h"
#include "twcg_if.h"

struct twcc_private {
	struct twcg_interface	*p_twcg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
twcc_accept_new_channel(struct twcc_interface *twcc_p, int sfd)
{
	struct twcc_private *priv_p = twcc_p->w_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__twcc_information(struct twcc_private *priv_p, char *msg_buf, struct umessage_twc_information *info_msg)
{
	char *log_header = "__twcc_information";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct twcg_interface *twcg_p = priv_p->p_twcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_twc_hdr *msghdr;
	int ctype = NID_CTYPE_TWC;
	char nothing_back;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_twc_hdr *)info_msg;
	assert(msghdr->um_req == UMSG_TWC_CMD_INFORMATION);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TWC_HEADER_LEN);
	if (cmd_len > UMSG_TWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_TWC_HEADER_LEN, cmd_len - UMSG_TWC_HEADER_LEN);

	umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, info_msg->um_twc_uuid);
	switch (msghdr->um_req_code) {
	case UMSG_TWC_CODE_FLUSHING:
		nid_log_warning("%s: got UMSG_TWC_CODE_FLUSHING, uuid:%s",  log_header, info_msg->um_twc_uuid);
		close(sfd);
		break;

	case UMSG_TWC_CODE_STAT: {
		struct umessage_twc_information_resp_stat info_stat;
		nid_log_warning("%s: got UMSG_TWC_CODE_STAT, uuid:%s",  log_header, info_msg->um_twc_uuid);
		if (twcg_p) {
	
			twcg_p->wg_op->wg_get_info_stat(twcg_p, info_msg->um_twc_uuid, &info_stat);
			info_stat.um_twc_uuid_len  = info_msg->um_twc_uuid_len;
			memcpy(info_stat.um_twc_uuid, info_msg->um_twc_uuid, info_msg->um_twc_uuid_len);
			msghdr = (struct umessage_twc_hdr *)&info_stat;
			msghdr->um_req = UMSG_TWC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_TWC_CODE_RESP_STAT;
			umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			nid_log_warning("%s: seq_assigned:%lu, seq_flushed:%lu, char2:%d char3:%d char4:%d char5:%d",
				log_header, info_stat.um_seq_assigned, info_stat.um_seq_flushed,
				msg_buf[2], msg_buf[3], msg_buf[4], msg_buf[5]);
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		close(sfd);
		break;
	}

	case UMSG_TWC_CODE_RW_STAT: {
		struct umessage_twc_information_rw_stat info_stat;
		nid_log_warning("%s: got UMSG_TWC_CODE_RW_STAT, uuid:%s",  log_header, info_msg->um_twc_uuid);
		if (twcg_p) {
			struct twc_rw_stat info;
			twcg_p->wg_op->wg_get_rw(twcg_p, info_msg->um_twc_uuid, info_msg->um_chan_uuid, &info);
			info_stat.um_twc_uuid_len  = info_msg->um_twc_uuid_len;
			memcpy(info_stat.um_twc_uuid, info_msg->um_twc_uuid, info_msg->um_twc_uuid_len);
			info_stat.um_chan_uuid_len  = info_msg->um_chan_uuid_len;
			memcpy(info_stat.um_chan_uuid, info_msg->um_chan_uuid, info_msg->um_chan_uuid_len);
			msghdr = (struct umessage_twc_hdr *)&info_stat;
			msghdr->um_req = UMSG_TWC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_TWC_CODE_RW_STAT_RSLT;
			info_stat.res = info.res;
			if (info_stat.res){
                                info_stat.um_chan_data_write = info.all_write_counts;
                                info_stat.um_chan_data_read = info.all_read_counts;
			} else{
				info_stat.um_chan_data_write = 0;
				info_stat.um_chan_data_read = 0;
			}

			umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			nid_log_warning("%s: written bytes:%lu, read bytes:%lu",
				log_header, info_stat.um_chan_data_write, info_stat.um_chan_data_read);

			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		close(sfd);
		break;
	}
	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}
}

static void
__twcc_display(struct twcc_private *priv_p, char *msg_buf, struct umessage_twc_display *dis_msg)
{
	char *log_header = "__twcc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct twcg_interface *twcg_p = priv_p->p_twcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_twc_hdr *msghdr;
	int ctype = NID_CTYPE_TWC, rc;
	char nothing_back;
	struct umessage_twc_display_resp dis_resp;
	struct wc_interface *twc_p;
	struct twc_setup *twc_setup_p;
	int *working_twc = NULL, cur_index = 0;
	int num_twc = 0, num_working_twc = 0, get_it = 0;
	int i;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_twc_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_TWC_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TWC_HEADER_LEN);

	if (cmd_len > UMSG_TWC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TWC_HEADER_LEN, cmd_len - UMSG_TWC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, twc_uuid: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_twc_uuid);

	switch (msghdr->um_req_code){
	case UMSG_TWC_CODE_S_DISP:
		if (dis_msg->um_twc_uuid == NULL)
			goto out;
		twc_setup_p = twcg_p->wg_op->wg_get_all_twc_setup(twcg_p, &num_twc);
		if (twc_setup_p == NULL)
			goto out;
		for (i = 0; i < num_twc; i++, twc_setup_p++) {
			if (!strcmp(twc_setup_p->uuid ,dis_msg->um_twc_uuid)) {
				get_it = 1;
				break;
			}
		}
		if (get_it) {
			dis_resp.um_twc_uuid_len = strlen(twc_setup_p->uuid);
			memcpy(dis_resp.um_twc_uuid, twc_setup_p->uuid, dis_resp.um_twc_uuid_len);
			dis_resp.um_tp_name_len = strlen(twc_setup_p->tp_name);
			memcpy(dis_resp.um_tp_name, twc_setup_p->tp_name, dis_resp.um_tp_name_len);
			dis_resp.um_do_fp = twc_setup_p->do_fp;

			msghdr = (struct umessage_twc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_TWC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_TWC_CODE_S_RESP_DISP;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;

	case UMSG_TWC_CODE_S_DISP_ALL:
		twc_setup_p = twcg_p->wg_op->wg_get_all_twc_setup(twcg_p, &num_twc);
		for (i = 0; i < num_twc; i++, twc_setup_p++) {
			memset(&dis_resp, 0, sizeof(dis_resp));
			dis_resp.um_twc_uuid_len = strlen(twc_setup_p->uuid);
			memcpy(dis_resp.um_twc_uuid, twc_setup_p->uuid, dis_resp.um_twc_uuid_len);
			dis_resp.um_tp_name_len = strlen(twc_setup_p->tp_name);
			memcpy(dis_resp.um_tp_name, twc_setup_p->tp_name, dis_resp.um_tp_name_len);
			dis_resp.um_do_fp = twc_setup_p->do_fp;

			msghdr = (struct umessage_twc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_TWC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_TWC_CODE_S_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_twc_hdr *)&dis_resp;
		msghdr->um_req = UMSG_TWC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_TWC_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_TWC_CODE_W_DISP:
		if (dis_msg->um_twc_uuid == NULL)
			goto out;
		twc_p = twcg_p->wg_op->wg_search_twc(twcg_p, dis_msg->um_twc_uuid);

		if (twc_p == NULL)
			goto out;

		twc_setup_p = twcg_p->wg_op->wg_get_all_twc_setup(twcg_p, &num_twc);
		for (i = 0; i <num_twc; i++, twc_setup_p++) {
			if (!strcmp(twc_setup_p->uuid, dis_msg->um_twc_uuid)) {
				dis_resp.um_twc_uuid_len = strlen(twc_setup_p->uuid);
				memcpy(dis_resp.um_twc_uuid, twc_setup_p->uuid, dis_resp.um_twc_uuid_len);
				dis_resp.um_tp_name_len = strlen(twc_setup_p->tp_name);
				memcpy(dis_resp.um_tp_name, twc_setup_p->tp_name, dis_resp.um_tp_name_len);
				dis_resp.um_do_fp = twc_setup_p->do_fp;

				msghdr = (struct umessage_twc_hdr *)&dis_resp;
				msghdr->um_req = UMSG_TWC_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_TWC_CODE_W_RESP_DISP;

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

	case UMSG_TWC_CODE_W_DISP_ALL:
		twc_setup_p = twcg_p->wg_op->wg_get_all_twc_setup(twcg_p, &num_twc);
		working_twc = twcg_p->wg_op->wg_get_working_twc_index(twcg_p, &num_working_twc);

		for (i = 0; i < num_working_twc; i++) {
			memset(&dis_resp, 0, sizeof(dis_resp));
			cur_index = working_twc[i];
			dis_resp.um_twc_uuid_len = strlen(twc_setup_p[cur_index].uuid);
			memcpy(dis_resp.um_twc_uuid, twc_setup_p[cur_index].uuid, dis_resp.um_twc_uuid_len);
			dis_resp.um_tp_name_len = strlen(twc_setup_p[cur_index].tp_name);
			memcpy(dis_resp.um_tp_name, twc_setup_p[cur_index].tp_name, dis_resp.um_tp_name_len);
			dis_resp.um_do_fp = twc_setup_p[cur_index].do_fp;

			msghdr = (struct umessage_twc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_TWC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_TWC_CODE_W_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_twc_hdr *)&dis_resp;
		msghdr->um_req = UMSG_TWC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_TWC_CODE_DISP_END;

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
	free(working_twc);
	close(sfd);
}

static void
__twcc_hello(struct twcc_private *priv_p, char *msg_buf, struct umessage_twc_hello *hello_msg)
{
	char *log_header = "__twcc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_twc_hdr *msghdr;
	int ctype = NID_CTYPE_TWC, rc;
	char nothing_back;
	struct umessage_twc_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_twc_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_TWC_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TWC_HEADER_LEN);

	if (cmd_len > UMSG_TWC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TWC_HEADER_LEN, cmd_len - UMSG_TWC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_TWC_CODE_HELLO:
		msghdr = (struct umessage_twc_hdr *)&hello_resp;
		msghdr->um_req = UMSG_TWC_CMD_HELLO;
		msghdr->um_req_code = UMSG_TWC_CODE_RESP_HELLO;

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
twcc_do_channel(struct twcc_interface *twcc_p)
{
	char *log_header = "twcc_do_channel";
	struct twcc_private *priv_p = (struct twcc_private *)twcc_p->w_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_twc twc_msg; 
	struct umessage_twc_hdr *msghdr = (struct umessage_twc_hdr *)&twc_msg;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_TWC_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;

	if (!priv_p->p_twcg) {
		nid_log_warning("%s support_twc should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr->um_req) {
	case UMSG_TWC_CMD_INFORMATION:
		__twcc_information(priv_p, msg_buf, (struct umessage_twc_information *)msghdr);
		break;

	case UMSG_TWC_CMD_DISPLAY:
		__twcc_display(priv_p, msg_buf, (struct umessage_twc_display *)msghdr);
		break;

	case UMSG_TWC_CMD_HELLO:
		__twcc_hello(priv_p, msg_buf, (struct umessage_twc_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
	}
}

static void
twcc_cleanup(struct twcc_interface *twcc_p)
{
	nid_log_debug("twcc_cleanup start, twcc_p:%p", twcc_p);
	if (twcc_p->w_private != NULL) {
		free(twcc_p->w_private);
		twcc_p->w_private = NULL;
	}
}

struct twcc_operations twcc_op = {
	.w_accept_new_channel = twcc_accept_new_channel,
	.w_do_channel = twcc_do_channel,
	.w_cleanup = twcc_cleanup,
};

int
twcc_initialization(struct twcc_interface *twcc_p, struct twcc_setup *setup)
{
	char *log_header = "twcc_initialization";
	struct twcc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	twcc_p->w_private = priv_p;
	twcc_p->w_op = &twcc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_twcg = setup->twcg;
	return 0;
}
