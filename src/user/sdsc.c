/*
 * sdsc.c
 * 	Implementation of Split Data Stream Channel Module
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
#include "umpk_sds_if.h"
#include "sdsc_if.h"
#include "sdsg_if.h"
#include "sds_if.h"
#include "ds_if.h"

struct sdsc_private {
	struct sdsg_interface	*p_sdsg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
sdsc_accept_new_channel(struct sdsc_interface *sdsc_p, int sfd)
{
	struct sdsc_private *priv_p = sdsc_p->sdsc_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__sdsc_add_sds(struct sdsc_private *priv_p, char *msg_buf, struct umessage_sds_add *add_msg)
{
	char *log_header = "__sdsc_add_sds";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sds_hdr *msghdr;
	int ctype = NID_CTYPE_SDS, rc;
	struct umessage_sds_add_resp add_resp;
	char nothing_back;
	char *sds_name, *pp_name, *wc_uuid, *rc_uuid;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sds_hdr *)add_msg;
	assert(msghdr->um_req == UMSG_SDS_CMD_ADD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SDS_HEADER_LEN);
	if (cmd_len > UMSG_SDS_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_SDS_HEADER_LEN, cmd_len - UMSG_SDS_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, sds_name:%s",
		log_header, cmd_len, msghdr->um_req_code, add_msg->um_sds_name);
	sds_name = add_msg->um_sds_name;
	pp_name = add_msg->um_pp_name;
	wc_uuid = add_msg->um_wc_uuid;
	rc_uuid = add_msg->um_rc_uuid;

	msghdr = (struct umessage_sds_hdr *)&add_resp;
	msghdr->um_req = UMSG_SDS_CMD_ADD;
	msghdr->um_req_code = UMSG_SDS_CODE_RESP_ADD;
	memcpy(add_resp.um_sds_name, add_msg->um_sds_name, add_msg->um_sds_name_len);
	add_resp.um_sds_name_len  = add_msg->um_sds_name_len;

	add_resp.um_resp_code = sdsg_p->dg_op->dg_add_sds(sdsg_p, sds_name, pp_name, wc_uuid, rc_uuid);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__sdsc_delete_sds(struct sdsc_private *priv_p, char *msg_buf, struct umessage_sds_delete *delete_msg)
{
	char *log_header = "__sdsc_delete";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sds_hdr *msghdr;
	int ctype = NID_CTYPE_SDS, rc;
	char nothing_back;
	struct umessage_sds_delete_resp delete_resp;
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sds_hdr *)delete_msg;
	assert(msghdr->um_req == UMSG_SDS_CMD_DELETE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SDS_HEADER_LEN);

	if (cmd_len > UMSG_SDS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_SDS_HEADER_LEN, cmd_len - UMSG_SDS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, sds_name:%s", log_header, cmd_len, msghdr->um_req_code, delete_msg->um_sds_name);

	switch (msghdr->um_req_code) {
	case UMSG_SDS_CODE_DELETE:
		delete_resp.um_sds_name_len = strlen(delete_msg->um_sds_name);
		strcpy(delete_resp.um_sds_name, delete_msg->um_sds_name);
		delete_resp.um_resp_code = sdsg_p->dg_op->dg_delete_sds(sdsg_p, delete_msg->um_sds_name);

		msghdr = (struct umessage_sds_hdr *)&delete_resp;
		msghdr->um_req = UMSG_SDS_CMD_DELETE;
		msghdr->um_req_code = UMSG_SDS_CODE_RESP_DELETE;

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
__sdsc_display(struct sdsc_private *priv_p, char *msg_buf, struct umessage_sds_display *dis_msg)
{
	char *log_header = "__sdsc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sds_hdr *msghdr;
	int ctype = NID_CTYPE_SDS, rc;
	char nothing_back;
	struct umessage_sds_display_resp dis_resp;
	struct ds_interface *sds_p;
	struct sds_setup *sds_setup_p;
	int *working_sds = NULL, cur_index = 0;
	int num_sds = 0, num_working_sds = 0, get_it = 0, found_sds = 0;
	int i;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sds_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_SDS_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SDS_HEADER_LEN);

	if (cmd_len > UMSG_SDS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_SDS_HEADER_LEN, cmd_len - UMSG_SDS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, sds_name: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_sds_name);

	switch (msghdr->um_req_code){
	case UMSG_SDS_CODE_S_DISP:
		if (dis_msg->um_sds_name == NULL)
			goto out;
		sds_setup_p = sdsg_p->dg_op->dg_get_all_sds_setup(sdsg_p, &num_sds);
		if (sds_setup_p == NULL)
			goto out;
		for (i = 0; i < SDSG_MAX_SDS; i++, sds_setup_p++) {
			if (found_sds == num_sds)
				break;
			if (strlen(sds_setup_p->sds_name) != 0) {
				found_sds++;
				if (!strcmp(sds_setup_p->sds_name ,dis_msg->um_sds_name)) {
					get_it = 1;
					break;
				}
			}
		}
		if (get_it) {
			dis_resp.um_sds_name_len = strlen(sds_setup_p->sds_name);
			memcpy(dis_resp.um_sds_name, sds_setup_p->sds_name, dis_resp.um_sds_name_len);
			dis_resp.um_pp_name_len = strlen(sds_setup_p->pp_name);
			memcpy(dis_resp.um_pp_name, sds_setup_p->pp_name, dis_resp.um_pp_name_len);
			dis_resp.um_wc_uuid_len = strlen(sds_setup_p->wc_name);
			memcpy(dis_resp.um_wc_uuid, sds_setup_p->wc_name, dis_resp.um_wc_uuid_len);
			dis_resp.um_rc_uuid_len = strlen(sds_setup_p->rc_name);
			memcpy(dis_resp.um_rc_uuid, sds_setup_p->rc_name, dis_resp.um_rc_uuid_len);

			msghdr = (struct umessage_sds_hdr *)&dis_resp;
			msghdr->um_req = UMSG_SDS_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_SDS_CODE_S_RESP_DISP;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;

	case UMSG_SDS_CODE_S_DISP_ALL:
		sds_setup_p = sdsg_p->dg_op->dg_get_all_sds_setup(sdsg_p, &num_sds);
		for (i = 0; i < SDSG_MAX_SDS; i++, sds_setup_p++) {
			if (found_sds == num_sds)
				break;
			if (strlen(sds_setup_p->sds_name) != 0) {
				dis_resp.um_sds_name_len = strlen(sds_setup_p->sds_name);
				memcpy(dis_resp.um_sds_name, sds_setup_p->sds_name, dis_resp.um_sds_name_len);
				dis_resp.um_pp_name_len = strlen(sds_setup_p->pp_name);
				memcpy(dis_resp.um_pp_name, sds_setup_p->pp_name, dis_resp.um_pp_name_len);
				dis_resp.um_wc_uuid_len = strlen(sds_setup_p->wc_name);
				memcpy(dis_resp.um_wc_uuid, sds_setup_p->wc_name, dis_resp.um_wc_uuid_len);
				dis_resp.um_rc_uuid_len = strlen(sds_setup_p->rc_name);
				memcpy(dis_resp.um_rc_uuid, sds_setup_p->rc_name, dis_resp.um_rc_uuid_len);

				msghdr = (struct umessage_sds_hdr *)&dis_resp;
				msghdr->um_req = UMSG_SDS_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_SDS_CODE_S_RESP_DISP_ALL;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				write(sfd, msg_buf, cmd_len);
				found_sds++;
			}
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_sds_hdr *)&dis_resp;
		msghdr->um_req = UMSG_SDS_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_SDS_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SDS_CODE_W_DISP:
		if (dis_msg->um_sds_name == NULL)
			goto out;
		sds_p = sdsg_p->dg_op->dg_search_sds(sdsg_p, dis_msg->um_sds_name);

		if (sds_p == NULL)
			goto out;

		sds_setup_p = sdsg_p->dg_op->dg_get_all_sds_setup(sdsg_p, &num_sds);
		for (i = 0; i <num_sds; i++, sds_setup_p++) {
			if (!strcmp(sds_setup_p->sds_name, dis_msg->um_sds_name)) {
				dis_resp.um_sds_name_len = strlen(sds_setup_p->sds_name);
				memcpy(dis_resp.um_sds_name, sds_setup_p->sds_name, dis_resp.um_sds_name_len);
				dis_resp.um_pp_name_len = strlen(sds_setup_p->pp_name);
				memcpy(dis_resp.um_pp_name, sds_setup_p->pp_name, dis_resp.um_pp_name_len);
				dis_resp.um_wc_uuid_len = strlen(sds_setup_p->wc_name);
				memcpy(dis_resp.um_wc_uuid, sds_setup_p->wc_name, dis_resp.um_wc_uuid_len);
				dis_resp.um_rc_uuid_len = strlen(sds_setup_p->rc_name);
				memcpy(dis_resp.um_rc_uuid, sds_setup_p->rc_name, dis_resp.um_rc_uuid_len);

				msghdr = (struct umessage_sds_hdr *)&dis_resp;
				msghdr->um_req = UMSG_SDS_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_SDS_CODE_W_RESP_DISP;

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

	case UMSG_SDS_CODE_W_DISP_ALL:
		sds_setup_p = sdsg_p->dg_op->dg_get_all_sds_setup(sdsg_p, &num_sds);
		working_sds = sdsg_p->dg_op->dg_get_working_sds_index(sdsg_p, &num_working_sds);

		for (i = 0; i < num_working_sds; i++) {
			cur_index = working_sds[i];
			dis_resp.um_sds_name_len = strlen(sds_setup_p[cur_index].sds_name);
			memcpy(dis_resp.um_sds_name, sds_setup_p[cur_index].sds_name, dis_resp.um_sds_name_len);
			dis_resp.um_pp_name_len = strlen(sds_setup_p[cur_index].pp_name);
			memcpy(dis_resp.um_pp_name, sds_setup_p[cur_index].pp_name, dis_resp.um_pp_name_len);
			dis_resp.um_wc_uuid_len = strlen(sds_setup_p[cur_index].wc_name);
			memcpy(dis_resp.um_wc_uuid, sds_setup_p[cur_index].wc_name, dis_resp.um_wc_uuid_len);
			dis_resp.um_rc_uuid_len = strlen(sds_setup_p[cur_index].rc_name);
			memcpy(dis_resp.um_rc_uuid, sds_setup_p[cur_index].rc_name, dis_resp.um_rc_uuid_len);

			msghdr = (struct umessage_sds_hdr *)&dis_resp;
			msghdr->um_req = UMSG_SDS_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_SDS_CODE_W_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_sds_hdr *)&dis_resp;
		msghdr->um_req = UMSG_SDS_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_SDS_CODE_DISP_END;

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
	free(working_sds);
	close(sfd);
}

static void
__sdsc_hello(struct sdsc_private *priv_p, char *msg_buf, struct umessage_sds_hello *hello_msg)
{
	char *log_header = "__sdsc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sds_hdr *msghdr;
	int ctype = NID_CTYPE_SDS, rc;
	char nothing_back;
	struct umessage_sds_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sds_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_SDS_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SDS_HEADER_LEN);

	if (cmd_len > UMSG_SDS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_SDS_HEADER_LEN, cmd_len - UMSG_SDS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_SDS_CODE_HELLO:
		msghdr = (struct umessage_sds_hdr *)&hello_resp;
		msghdr->um_req = UMSG_SDS_CMD_HELLO;
		msghdr->um_req_code = UMSG_SDS_CODE_RESP_HELLO;

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
sdsc_do_channel(struct sdsc_interface *sdsc_p, struct scg_interface *scg_p)
{
	char *log_header = "sdsc_do_channel";
	struct sdsc_private *priv_p = (struct sdsc_private *)sdsc_p->sdsc_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_sds sds_msg;
	struct umessage_sds_hdr *msghdr = (struct umessage_sds_hdr *)&sds_msg;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_SDS_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	nid_log_error("um_req: %u", msghdr->um_req);

	if (!priv_p->p_sdsg) {
		nid_log_warning("%s support_sds should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr->um_req) {
	case UMSG_SDS_CMD_ADD:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__sdsc_add_sds(priv_p, msg_buf, (struct umessage_sds_add *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_SDS_CMD_DELETE:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__sdsc_delete_sds(priv_p, msg_buf, (struct umessage_sds_delete *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_SDS_CMD_DISPLAY:
		__sdsc_display(priv_p, msg_buf, (struct umessage_sds_display *)msghdr);
		break;

	case UMSG_SDS_CMD_HELLO:
		__sdsc_hello(priv_p, msg_buf, (struct umessage_sds_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
	}
}

static void
sdsc_cleanup(struct sdsc_interface *sdsc_p)
{
	nid_log_debug("sdsc_cleanup start, sdsc_p:%p", sdsc_p);
	if (sdsc_p->sdsc_private != NULL) {
		free(sdsc_p->sdsc_private);
		sdsc_p->sdsc_private = NULL;
	}
}

struct sdsc_operations sdsc_op = {
	.sdsc_accept_new_channel = sdsc_accept_new_channel,
	.sdsc_do_channel = sdsc_do_channel,
	.sdsc_cleanup = sdsc_cleanup,
};

int
sdsc_initialization(struct sdsc_interface *sdsc_p, struct sdsc_setup *setup)
{
	char *log_header = "sdsc_initialization";
	struct sdsc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	sdsc_p->sdsc_private = priv_p;
	sdsc_p->sdsc_op = &sdsc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_sdsg = setup->sdsg;
	return 0;
}
