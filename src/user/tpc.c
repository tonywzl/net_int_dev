/*
 * tpc.c
 *	Implementation of Thread Pool Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "util_nw.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_tp_if.h"
#include "tp_if.h"
#include "tpc_if.h"
#include "tpg_if.h"

struct tpc_private {
	struct tpg_interface	*p_tpg;
	int 			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
tpc_accept_new_channel(struct tpc_interface *tpc_p, int sfd)
{
	char *log_header = "tpc_accept_new_channel";
	struct tpc_private *priv_p = tpc_p->t_private;

	nid_log_debug("%s: start", log_header);
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__tpc_information(struct tpc_private *priv_p, char *msg_buf, struct umessage_tp_information *info_msg)
{
	char *log_header = "__tpc_information";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct tpg_interface *tpg_p = priv_p->p_tpg;
	struct tp_interface *tp_p;
	struct tp_interface **tps;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_tp_hdr *msghdr;
	int ctype = NID_CTYPE_TP, rc;
	char nothing_back;
	int num_tp, i;
	char *out_uuid;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_tp_hdr *)info_msg;
	assert(msghdr->um_req == UMSG_TP_CMD_INFORMATION);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TP_HEADER_LEN);

	if (cmd_len > UMSG_TP_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TP_HEADER_LEN, cmd_len - UMSG_TP_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s", log_header, cmd_len, msghdr->um_req_code, info_msg->um_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_TP_CODE_STAT: {
		struct tp_stat resp_stat;
		struct umessage_tp_information_resp_stat info_stat;
		tp_p = NULL;
		tp_p = tpg_p->tg_op->tpg_search_tp(tpg_p, info_msg->um_uuid);
		if (tp_p != NULL) {
			tp_p->t_op->t_get_stat(tp_p, &resp_stat);
			info_stat.um_uuid_len = info_msg->um_uuid_len;
			memcpy(info_stat.um_uuid, info_msg->um_uuid, info_msg->um_uuid_len);
			info_stat.um_nused = resp_stat.s_nused;
			info_stat.um_nfree = resp_stat.s_nfree;
			info_stat.um_workers = resp_stat.s_workers;
			info_stat.um_max_workers = resp_stat.s_max_workers;
			info_stat.um_no_free = resp_stat.s_no_free;

			msghdr = (struct umessage_tp_hdr *)&info_stat;
			msghdr->um_req = UMSG_TP_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_TP_CODE_RESP_STAT;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		} else {
			msghdr = (struct umessage_tp_hdr *)&info_stat;
			msghdr->um_req = UMSG_TP_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_TP_CODE_RESP_NOT_FOUND;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		
	}
		break;

	case UMSG_TP_CODE_STAT_ALL: {
		struct tp_stat resp_stat;
		struct umessage_tp_information_resp_stat info_stat;
		tps = tpg_p->tg_op->tpg_get_all_tp(tpg_p, &num_tp);
		for (i = 0; i < num_tp; i++) {
			tp_p = tps[i];
			tp_p->t_op->t_get_stat(tp_p, &resp_stat);
			out_uuid = tp_p->t_op->t_get_name(tp_p);
			info_stat.um_uuid_len = strlen(out_uuid);
			memcpy(info_stat.um_uuid, out_uuid, info_stat.um_uuid_len);
			info_stat.um_nused = resp_stat.s_nused;
			info_stat.um_nfree = resp_stat.s_nfree;
			info_stat.um_workers = resp_stat.s_workers;
			info_stat.um_max_workers = resp_stat.s_max_workers;
			info_stat.um_no_free = resp_stat.s_no_free;

			msghdr = (struct umessage_tp_hdr *)&info_stat;
			msghdr->um_req = UMSG_TP_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_TP_CODE_RESP_STAT_ALL;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		msghdr = (struct umessage_tp_hdr *)&info_stat;
		msghdr->um_req = UMSG_TP_CMD_INFORMATION;
		msghdr->um_req_code = UMSG_TP_CODE_RESP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
	
		read(sfd, &nothing_back, 1);
	}
		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

out:
	close(sfd);
}

static void
__tpc_add(struct tpc_private *priv_p, char *msg_buf, struct umessage_tp_add *add_msg)
{
	char *log_header = "__tpc_add";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_tp_hdr *msghdr;
	int ctype = NID_CTYPE_TP, rc;
	char nothing_back;
	struct umessage_tp_add_resp add_resp;
	struct tpg_interface *tpg_p = priv_p->p_tpg;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_tp_hdr *)add_msg;
	assert(msghdr->um_req == UMSG_TP_CMD_ADD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TP_HEADER_LEN);

	if (cmd_len > UMSG_TP_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TP_HEADER_LEN, cmd_len - UMSG_TP_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, uuid:%s", log_header, cmd_len, msghdr->um_req_code, add_msg->um_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_TP_CODE_ADD:
		add_resp.um_uuid_len = strlen(add_msg->um_uuid);
		strcpy(add_resp.um_uuid, add_msg->um_uuid);
		add_resp.um_resp_code = tpg_p->tg_op->tpg_add_tp(tpg_p, add_msg->um_uuid, add_msg->um_workers);

		msghdr = (struct umessage_tp_hdr *)&add_resp;
		msghdr->um_req = UMSG_TP_CMD_ADD;
		msghdr->um_req_code = UMSG_TP_CODE_RESP_ADD;

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
__tpc_delete(struct tpc_private *priv_p, char *msg_buf, struct umessage_tp_delete *delete_msg)
{
	char *log_header = "__tpc_delete";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_tp_hdr *msghdr;
	int ctype = NID_CTYPE_TP, rc;
	char nothing_back;
	struct umessage_tp_delete_resp delete_resp;
	struct tpg_interface *tpg_p = priv_p->p_tpg;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_tp_hdr *)delete_msg;
	assert(msghdr->um_req == UMSG_TP_CMD_DELETE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TP_HEADER_LEN);

	if (cmd_len > UMSG_TP_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TP_HEADER_LEN, cmd_len - UMSG_TP_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, uuid:%s", log_header, cmd_len, msghdr->um_req_code, delete_msg->um_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_TP_CODE_DELETE:
		delete_resp.um_uuid_len = strlen(delete_msg->um_uuid);
		strcpy(delete_resp.um_uuid, delete_msg->um_uuid);
		delete_resp.um_resp_code = tpg_p->tg_op->tpg_delete_tp(tpg_p, delete_msg->um_uuid);

		msghdr = (struct umessage_tp_hdr *)&delete_resp;
		msghdr->um_req = UMSG_TP_CMD_DELETE;
		msghdr->um_req_code = UMSG_TP_CODE_RESP_DELETE;

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
__tpc_display(struct tpc_private *priv_p, char *msg_buf, struct umessage_tp_display *dis_msg)
{
	char *log_header = "__tpc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct tpg_interface *tpg_p =  priv_p->p_tpg;
	struct umessage_tp_display_resp resp_msg;
	struct tp_setup *setup_p, *cur_setup_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_tp_hdr *msghdr;
	int ctype = NID_CTYPE_TP, rc;
	char nothing_back;
	int num_tp, num_working_tp, i, j;
	char *out_uuid, **working_tp = NULL;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_tp_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_TP_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TP_HEADER_LEN);

	if (cmd_len > UMSG_TP_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TP_HEADER_LEN, cmd_len - UMSG_TP_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_TP_CODE_S_DISP:
		setup_p = tpg_p->tg_op->tpg_get_all_tp_setup(tpg_p, &num_tp);
		for (i = 0; i < num_tp; i++, setup_p++) {
			if (!strcmp(setup_p->name, dis_msg->um_uuid)) {
				out_uuid = setup_p->name;
				resp_msg.um_uuid_len = strlen(out_uuid);
				memcpy(resp_msg.um_uuid, out_uuid, resp_msg.um_uuid_len);
				resp_msg.um_max_workers = setup_p->max_workers;
				resp_msg.um_min_workers = setup_p->min_workers;
				resp_msg.um_extend = setup_p->extend;
				resp_msg.um_delay = setup_p->delay;

				msghdr = (struct umessage_tp_hdr *)&resp_msg;
				msghdr->um_req = UMSG_TP_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_TP_CODE_S_RESP_DISP;
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

	case UMSG_TP_CODE_S_DISP_ALL:
		setup_p = tpg_p->tg_op->tpg_get_all_tp_setup(tpg_p, &num_tp);
		for (i = 0; i < num_tp; i++, setup_p++) {
			out_uuid = setup_p->name;
			resp_msg.um_uuid_len = strlen(out_uuid);
			memcpy(resp_msg.um_uuid, out_uuid, resp_msg.um_uuid_len);
			resp_msg.um_max_workers = setup_p->max_workers;
			resp_msg.um_min_workers = setup_p->min_workers;
			resp_msg.um_extend = setup_p->extend;
			resp_msg.um_delay = setup_p->delay;

			msghdr = (struct umessage_tp_hdr *)&resp_msg;
			msghdr->um_req = UMSG_TP_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_TP_CODE_S_RESP_DISP_ALL;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&resp_msg, 0, sizeof(resp_msg));
		msghdr = (struct umessage_tp_hdr *)&resp_msg;
		msghdr->um_req = UMSG_TP_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_TP_CODE_DISP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);	
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_TP_CODE_W_DISP:
		working_tp = tpg_p->tg_op->tpg_get_working_tp_name(tpg_p, &num_working_tp);
		for (i = 0; i < num_working_tp; i++) {
			if (!strcmp(working_tp[i], dis_msg->um_uuid)) {
				break;
			}
		}

		if (i < num_working_tp) {
			setup_p = tpg_p->tg_op->tpg_get_all_tp_setup(tpg_p, &num_tp);
			for (i = 0; i < num_tp; i++, setup_p++) {
				if (!strcmp(setup_p->name, dis_msg->um_uuid)) {
					out_uuid = setup_p->name;
					resp_msg.um_uuid_len = strlen(out_uuid);
					memcpy(resp_msg.um_uuid, out_uuid, resp_msg.um_uuid_len);
					resp_msg.um_max_workers = setup_p->max_workers;
					resp_msg.um_min_workers = setup_p->min_workers;
					resp_msg.um_extend = setup_p->extend;
					resp_msg.um_delay = setup_p->delay;

					msghdr = (struct umessage_tp_hdr *)&resp_msg;
					msghdr->um_req = UMSG_TP_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_TP_CODE_W_RESP_DISP;
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

	case UMSG_TP_CODE_W_DISP_ALL:
		setup_p = tpg_p->tg_op->tpg_get_all_tp_setup(tpg_p, &num_tp);
		working_tp = tpg_p->tg_op->tpg_get_working_tp_name(tpg_p, &num_working_tp);
		for (i = 0; i < num_working_tp; i++) {
			cur_setup_p = setup_p;
			for (j = 0; j < num_tp; j++, cur_setup_p++) {
				if (!strcmp(working_tp[i], cur_setup_p->name)) {
					out_uuid = cur_setup_p->name;
					resp_msg.um_uuid_len = strlen(out_uuid);
					memcpy(resp_msg.um_uuid, out_uuid, resp_msg.um_uuid_len);
					resp_msg.um_max_workers = cur_setup_p->max_workers;
					resp_msg.um_min_workers = cur_setup_p->min_workers;
					resp_msg.um_extend = cur_setup_p->extend;
					resp_msg.um_delay = cur_setup_p->delay;

					msghdr = (struct umessage_tp_hdr *)&resp_msg;
					msghdr->um_req = UMSG_TP_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_TP_CODE_W_RESP_DISP_ALL;
					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					write(sfd, msg_buf, cmd_len);
				}
			}
		}

		memset(&resp_msg, 0, sizeof(resp_msg));
		msghdr = (struct umessage_tp_hdr *)&resp_msg;
		msghdr->um_req = UMSG_TP_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_TP_CODE_DISP_END;
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
	free(working_tp);
	close(sfd);
}

static void
__tpc_hello(struct tpc_private *priv_p, char *msg_buf, struct umessage_tp_hello *hello_msg)
{
	char *log_header = "__tpc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_tp_hdr *msghdr;
	int ctype = NID_CTYPE_TP, rc;
	char nothing_back;
	struct umessage_tp_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_tp_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_TP_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_TP_HEADER_LEN);

	if (cmd_len > UMSG_TP_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_TP_HEADER_LEN, cmd_len - UMSG_TP_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_TP_CODE_HELLO:
		msghdr = (struct umessage_tp_hdr *)&hello_resp;
		msghdr->um_req = UMSG_TP_CMD_HELLO;
		msghdr->um_req_code = UMSG_TP_CODE_RESP_HELLO;

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
tpc_do_channel(struct tpc_interface *tpc_p, struct scg_interface *scg_p)
{
	char *log_header = "tpc_do_channel";
	struct tpc_private *priv_p = (struct tpc_private *)tpc_p->t_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_tp tp_msg;
	struct umessage_tp_hdr *msghdr_p;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_TP_HEADER_LEN);

	msghdr_p = (struct umessage_tp_hdr *)&tp_msg;
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = *(uint32_t *)p;
	nid_log_warning("%s: req:%d, req_code:%d, um_len:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code, msghdr_p->um_len);

	if (!priv_p->p_tpg) {
		nid_log_warning("%s support_tp should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr_p->um_req) {
	case UMSG_TP_CMD_INFORMATION:
		__tpc_information(priv_p, msg_buf, (struct umessage_tp_information *)msghdr_p);
		break;

	case UMSG_TP_CMD_ADD:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__tpc_add(priv_p, msg_buf, (struct umessage_tp_add *)msghdr_p);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_TP_CMD_DELETE:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__tpc_delete(priv_p, msg_buf, (struct umessage_tp_delete *)msghdr_p);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_TP_CMD_DISPLAY:
		__tpc_display(priv_p, msg_buf, (struct umessage_tp_display *)msghdr_p);
		break;

	case UMSG_TP_CMD_HELLO:
		__tpc_hello(priv_p, msg_buf, (struct umessage_tp_hello *)msghdr_p);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr_p->um_req);
	}

}

static void
tpc_cleanup(struct tpc_interface *tpc_p)
{
	char *log_header = "tpc_cleanup";

	nid_log_debug("%s start, tpc_p:%p", log_header, tpc_p);
	if (tpc_p->t_private != NULL) {
		free(tpc_p->t_private);
		tpc_p->t_private = NULL;
	}
}

struct tpc_operations tpc_op = {
	.t_accept_new_channel = tpc_accept_new_channel,
	.t_do_channel = tpc_do_channel,
	.t_cleanup = tpc_cleanup,
};

int
tpc_initialization(struct tpc_interface *tpc_p, struct tpc_setup *setup)
{
	char *log_header = "tpc_initialization";
	struct tpc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	tpc_p->t_private = priv_p;
	tpc_p->t_op = &tpc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_tpg = setup->tpg;

	return 0; 
}
