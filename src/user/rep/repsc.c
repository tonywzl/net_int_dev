/*
 * repsc.c
 *	Implementation of Replication Source Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "util_nw.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_reps_if.h"
#include "reps_if.h"
#include "repsg_if.h"
#include "repsc_if.h"
#include "lstn_if.h"

struct repsc_private {
	struct repsg_interface	*p_repsg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
repsc_accept_new_channel(struct repsc_interface *rsc_p, int sfd)
{
	char *log_header = "repsc_accept_new_channel";
	struct repsc_private *priv_p = rsc_p->r_private;

	nid_log_debug("%s: start", log_header);
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__repsc_start(struct repsc_private *priv_p, char *msg_buf, struct umessage_reps_start *start_msg)
{
	char *log_header = "__repsc_start";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct repsg_interface	*repsg = priv_p->p_repsg;
	struct reps_interface	*reps = NULL;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_reps_hdr *msghdr;
	int ctype = NID_CTYPE_REPS, rc;
	char nothing_back;
	struct umessage_reps_start_resp start_resp;
	char *snap_name = NULL, *snap_name2 = NULL;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_reps_hdr *)start_msg;
	assert(msghdr->um_req == UMSG_REPS_CMD_START);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPS_HEADER_LEN);

	if (cmd_len > UMSG_REPS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPS_HEADER_LEN, cmd_len - UMSG_REPS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}
	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	if (repsg == NULL)
		goto out;
	else
		reps = repsg->rsg_op->rsg_search_and_create_rs(repsg, start_msg->um_rs_name);

	if (reps == NULL) {
		nid_log_warning("%s: reps %s not found", log_header, start_msg->um_rs_name);
		goto out;
	}

	switch (msghdr->um_req_code) {
	case UMSG_REPS_CODE_START:
		start_resp.um_resp_code  = reps->rs_op->rs_start(reps);

		msghdr = (struct umessage_reps_hdr *)&start_resp;
		msghdr->um_req = UMSG_REPS_CMD_START;
		msghdr->um_req_code = UMSG_REPS_CODE_RESP_START;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}

		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_REPS_CODE_START_SNAP:
		snap_name = ((struct umessage_reps_start_snap *)start_msg)->um_sp_name;
		start_resp.um_resp_code  = reps->rs_op->rs_start_snap(reps, snap_name);

		msghdr = (struct umessage_reps_hdr *)&start_resp;
		msghdr->um_req = UMSG_REPS_CMD_START;
		msghdr->um_req_code = UMSG_REPS_CODE_RESP_START;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}

		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_REPS_CODE_START_SNAP_DIFF:
		snap_name = ((struct umessage_reps_start_snap_diff *)start_msg)->um_sp_name;
		snap_name2 = ((struct umessage_reps_start_snap_diff *)start_msg)->um_sp_name2;
		start_resp.um_resp_code  = reps->rs_op->rs_start_snapdiff(reps, snap_name, snap_name2);

		msghdr = (struct umessage_reps_hdr *)&start_resp;
		msghdr->um_req = UMSG_REPS_CMD_START;
		msghdr->um_req_code = UMSG_REPS_CODE_RESP_START;

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
__repsc_pause(struct repsc_private *priv_p, char *msg_buf, struct umessage_reps_pause *pause_msg)
{
	char *log_header = "__repsc_pause";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct repsg_interface	*repsg = priv_p->p_repsg;
	struct reps_interface	*reps = NULL;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_reps_hdr *msghdr;
	int ctype = NID_CTYPE_REPS, rc;
	char nothing_back;
	struct umessage_reps_pause_resp pause_resp;

	nid_log_warning("%s: pause ...", log_header);
	msghdr = (struct umessage_reps_hdr *)pause_msg;
	assert(msghdr->um_req == UMSG_REPS_CMD_PAUSE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPS_HEADER_LEN);

	if (cmd_len > UMSG_REPS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPS_HEADER_LEN, cmd_len - UMSG_REPS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}
	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	if (repsg == NULL)
		goto out;
	else
		reps = repsg->rsg_op->rsg_search_and_create_rs(repsg, pause_msg->um_rs_name);

	if (reps == NULL) {
		nid_log_warning("%s: reps %s not found", log_header, pause_msg->um_rs_name);
		goto out;
	}

	switch (msghdr->um_req_code) {
	case UMSG_REPS_CODE_PAUSE:
		pause_resp.um_resp_code  = reps->rs_op->rs_set_pause(reps);

		msghdr = (struct umessage_reps_hdr *)&pause_resp;
		msghdr->um_req = UMSG_REPS_CMD_PAUSE;
		msghdr->um_req_code = UMSG_REPS_CODE_RESP_PAUSE;

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
__repsc_continue(struct repsc_private *priv_p, char *msg_buf, struct umessage_reps_continue *continue_msg)
{
	char *log_header = "__repsc_continue";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct repsg_interface	*repsg = priv_p->p_repsg;
	struct reps_interface	*reps = NULL;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_reps_hdr *msghdr;
	int ctype = NID_CTYPE_REPS, rc;
	char nothing_back;
	struct umessage_reps_continue_resp continue_resp;

	nid_log_warning("%s: continue ...", log_header);
	msghdr = (struct umessage_reps_hdr *)continue_msg;
	assert(msghdr->um_req == UMSG_REPS_CMD_CONTINUE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPS_HEADER_LEN);

	if (cmd_len > UMSG_REPS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPS_HEADER_LEN, cmd_len - UMSG_REPS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}
	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	if (repsg == NULL)
		goto out;
	else
		reps = repsg->rsg_op->rsg_search_and_create_rs(repsg, continue_msg->um_rs_name);

	if (reps == NULL) {
		nid_log_warning("%s: reps %s not found", log_header, continue_msg->um_rs_name);
		goto out;
	}

	switch (msghdr->um_req_code) {
	case UMSG_REPS_CODE_CONTINUE:
		continue_resp.um_resp_code  = reps->rs_op->rs_set_continue(reps);

		msghdr = (struct umessage_reps_hdr *)&continue_resp;
		msghdr->um_req = UMSG_REPS_CMD_CONTINUE;
		msghdr->um_req_code = UMSG_REPS_CODE_RESP_CONTINUE;

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
__repsc_display(struct repsc_private *priv_p, char *msg_buf, struct umessage_reps_display *dis_msg)
{
	char *log_header = "__repsc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct repsg_interface *repsg_p = priv_p->p_repsg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_reps_hdr *msghdr;
	int ctype = NID_CTYPE_REPS, rc;
	char nothing_back;
	struct umessage_reps_display_resp dis_resp;
	struct reps_setup *reps_setup_p, *cur_setup_p;
	struct reps_interface *reps_p;
	char **working_reps = NULL;
	int num_reps = 0, num_working_reps = 0, i, j;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_reps_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_REPS_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPS_HEADER_LEN);

	if (cmd_len > UMSG_REPS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPS_HEADER_LEN, cmd_len - UMSG_REPS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, rs_name: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_rs_name);

	switch (msghdr->um_req_code){
	case UMSG_REPS_CODE_S_DISP:
		if (dis_msg->um_rs_name == NULL)
			goto out;
		reps_setup_p = repsg_p->rsg_op->rsg_get_all_rs_setup(repsg_p, &num_reps);
		if (reps_setup_p == NULL)
			goto out;
		for (i = 0; i < num_reps; i++, reps_setup_p++) {
			if (!strcmp(dis_msg->um_rs_name, reps_setup_p->rs_name)) {
				dis_resp.um_rs_name_len = strlen(reps_setup_p->rs_name);
				memcpy(dis_resp.um_rs_name, reps_setup_p->rs_name, dis_resp.um_rs_name_len);
				dis_resp.um_rt_name_len = strlen(reps_setup_p->rt_name);
				memcpy(dis_resp.um_rt_name, reps_setup_p->rt_name, dis_resp.um_rt_name_len);
				dis_resp.um_dxa_name_len = strlen(reps_setup_p->dxa_name);
				memcpy(dis_resp.um_dxa_name, reps_setup_p->dxa_name , dis_resp.um_dxa_name_len);
				dis_resp.um_voluuid_len = strlen(reps_setup_p->voluuid);
				memcpy(dis_resp.um_voluuid, reps_setup_p->voluuid, dis_resp.um_voluuid_len);
				dis_resp.um_snapuuid_len = strlen(reps_setup_p->snapuuid);
				memcpy(dis_resp.um_snapuuid, reps_setup_p->snapuuid, dis_resp.um_snapuuid_len);
				dis_resp.um_snapuuid2_len = strlen(reps_setup_p->snapuuid2);
				memcpy(dis_resp.um_snapuuid2, reps_setup_p->snapuuid2, dis_resp.um_snapuuid2_len);
				dis_resp.um_bitmap_len = reps_setup_p->bitmap_len;
				dis_resp.um_vol_len = reps_setup_p->vol_len;

				msghdr = (struct umessage_reps_hdr *)&dis_resp;
				msghdr->um_req = UMSG_REPS_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_REPS_CODE_S_RESP_DISP;

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

	case UMSG_REPS_CODE_S_DISP_ALL:
		reps_setup_p = repsg_p->rsg_op->rsg_get_all_rs_setup(repsg_p, &num_reps);
		for (i = 0; i < num_reps; i++, reps_setup_p++) {
			dis_resp.um_rs_name_len = strlen(reps_setup_p->rs_name);
			memcpy(dis_resp.um_rs_name, reps_setup_p->rs_name, dis_resp.um_rs_name_len);
			dis_resp.um_rt_name_len = strlen(reps_setup_p->rt_name);
			memcpy(dis_resp.um_rt_name, reps_setup_p->rt_name, dis_resp.um_rt_name_len);
			dis_resp.um_dxa_name_len = strlen(reps_setup_p->dxa_name);
			memcpy(dis_resp.um_dxa_name, reps_setup_p->dxa_name , dis_resp.um_dxa_name_len);
			dis_resp.um_voluuid_len = strlen(reps_setup_p->voluuid);
			memcpy(dis_resp.um_voluuid, reps_setup_p->voluuid, dis_resp.um_voluuid_len);
			dis_resp.um_snapuuid_len = strlen(reps_setup_p->snapuuid);
			memcpy(dis_resp.um_snapuuid, reps_setup_p->snapuuid, dis_resp.um_snapuuid_len);
			dis_resp.um_snapuuid2_len = strlen(reps_setup_p->snapuuid2);
			memcpy(dis_resp.um_snapuuid2, reps_setup_p->snapuuid2, dis_resp.um_snapuuid2_len);
			dis_resp.um_bitmap_len = reps_setup_p->bitmap_len;
			dis_resp.um_vol_len = reps_setup_p->vol_len;

			msghdr = (struct umessage_reps_hdr *)&dis_resp;
			msghdr->um_req = UMSG_REPS_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_REPS_CODE_S_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_reps_hdr *)&dis_resp;
		msghdr->um_req = UMSG_REPS_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_REPS_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_REPS_CODE_W_DISP:
		if (dis_msg->um_rs_name == NULL)
			goto out;
		reps_p = repsg_p->rsg_op->rsg_search_rs(repsg_p, dis_msg->um_rs_name);
		if (reps_p == NULL)
			goto out;
		reps_setup_p = repsg_p->rsg_op->rsg_get_all_rs_setup(repsg_p, &num_reps);
		for (i = 0; i < num_reps; i++, reps_setup_p++) {
			if (!strcmp(dis_msg->um_rs_name, reps_setup_p->rs_name)) {
				dis_resp.um_rs_name_len = strlen(reps_setup_p->rs_name);
				memcpy(dis_resp.um_rs_name, reps_setup_p->rs_name, dis_resp.um_rs_name_len);
				dis_resp.um_rt_name_len = strlen(reps_setup_p->rt_name);
				memcpy(dis_resp.um_rt_name, reps_setup_p->rt_name, dis_resp.um_rt_name_len);
				dis_resp.um_dxa_name_len = strlen(reps_setup_p->dxa_name);
				memcpy(dis_resp.um_dxa_name, reps_setup_p->dxa_name , dis_resp.um_dxa_name_len);
				dis_resp.um_voluuid_len = strlen(reps_setup_p->voluuid);
				memcpy(dis_resp.um_voluuid, reps_setup_p->voluuid, dis_resp.um_voluuid_len);
				dis_resp.um_snapuuid_len = strlen(reps_setup_p->snapuuid);
				memcpy(dis_resp.um_snapuuid, reps_setup_p->snapuuid, dis_resp.um_snapuuid_len);
				dis_resp.um_snapuuid2_len = strlen(reps_setup_p->snapuuid2);
				memcpy(dis_resp.um_snapuuid2, reps_setup_p->snapuuid2, dis_resp.um_snapuuid2_len);
				dis_resp.um_bitmap_len = reps_setup_p->bitmap_len;
				dis_resp.um_vol_len = reps_setup_p->vol_len;

				msghdr = (struct umessage_reps_hdr *)&dis_resp;
				msghdr->um_req = UMSG_REPS_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_REPS_CODE_W_RESP_DISP;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				write(sfd, msg_buf, cmd_len);
				read(sfd, &nothing_back, 1);
			}
		}
		break;

	case UMSG_REPS_CODE_W_DISP_ALL:
		working_reps = repsg_p->rsg_op->rsg_get_working_rs_name(repsg_p, &num_working_reps);
		reps_setup_p = repsg_p->rsg_op->rsg_get_all_rs_setup(repsg_p, &num_reps);
		for (i = 0; i < num_working_reps; i++) {
			cur_setup_p = reps_setup_p;
			for (j = 0; j < num_reps; j++, cur_setup_p++) {
				if (!strcmp(working_reps[i], cur_setup_p->rs_name)) {
					dis_resp.um_rs_name_len = strlen(cur_setup_p->rs_name);
					memcpy(dis_resp.um_rs_name, cur_setup_p->rs_name, dis_resp.um_rs_name_len);
					dis_resp.um_rt_name_len = strlen(cur_setup_p->rt_name);
					memcpy(dis_resp.um_rt_name, cur_setup_p->rt_name, dis_resp.um_rt_name_len);
					dis_resp.um_dxa_name_len = strlen(cur_setup_p->dxa_name);
					memcpy(dis_resp.um_dxa_name, cur_setup_p->dxa_name , dis_resp.um_dxa_name_len);
					dis_resp.um_voluuid_len = strlen(cur_setup_p->voluuid);
					memcpy(dis_resp.um_voluuid, cur_setup_p->voluuid, dis_resp.um_voluuid_len);
					dis_resp.um_snapuuid_len = strlen(cur_setup_p->snapuuid);
					memcpy(dis_resp.um_snapuuid, cur_setup_p->snapuuid, dis_resp.um_snapuuid_len);
					dis_resp.um_snapuuid2_len = strlen(cur_setup_p->snapuuid2);
					memcpy(dis_resp.um_snapuuid2, cur_setup_p->snapuuid2, dis_resp.um_snapuuid2_len);
					dis_resp.um_bitmap_len = cur_setup_p->bitmap_len;
					dis_resp.um_vol_len = cur_setup_p->vol_len;

					msghdr = (struct umessage_reps_hdr *)&dis_resp;
					msghdr->um_req = UMSG_REPS_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_REPS_CODE_W_RESP_DISP_ALL;

					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					write(sfd, msg_buf, cmd_len);
				}
			}
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_reps_hdr *)&dis_resp;
		msghdr->um_req = UMSG_REPS_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_REPS_CODE_DISP_END;

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
	free(working_reps);
	close(sfd);
}

static void
__repsc_hello(struct repsc_private *priv_p, char *msg_buf, struct umessage_reps_hello *hello_msg)
{
	char *log_header = "__repsc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_reps_hdr *msghdr;
	int ctype = NID_CTYPE_REPS, rc;
	char nothing_back;
	struct umessage_reps_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_reps_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_REPS_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPS_HEADER_LEN);

	if (cmd_len > UMSG_REPS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPS_HEADER_LEN, cmd_len - UMSG_REPS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_REPS_CODE_HELLO:
		msghdr = (struct umessage_reps_hdr *)&hello_resp;
		msghdr->um_req = UMSG_REPS_CMD_HELLO;
		msghdr->um_req_code = UMSG_REPS_CODE_RESP_HELLO;

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
__repsc_info(struct repsc_private *priv_p, char *msg_buf, struct umessage_reps_info *info_msg)
{
	char *log_header = "__repsc_info";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct repsg_interface *repsg_p = priv_p->p_repsg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_reps_hdr *msghdr;
	int ctype = NID_CTYPE_REPS, rc;
	char nothing_back;
	struct reps_interface *reps_p;
	struct umessage_reps_info_pro_resp info_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_reps_hdr *)info_msg;
	assert(msghdr->um_req == UMSG_REPS_CMD_INFO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_REPS_HEADER_LEN);

	if (cmd_len > UMSG_REPS_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_REPS_HEADER_LEN, cmd_len - UMSG_REPS_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, reps_name:%s", log_header, cmd_len, msghdr->um_req_code, info_msg->um_rs_name);

	reps_p = repsg_p->rsg_op->rsg_search_and_create_rs(repsg_p, info_msg->um_rs_name);
	if (reps_p == NULL) {
		nid_log_warning("%s: reps %s not found", log_header, info_msg->um_rs_name);
		goto out;
	}

	switch (msghdr->um_req_code) {
	case UMSG_REPS_CODE_INFO_PRO:
		msghdr = (struct umessage_reps_hdr *)&info_resp;
		msghdr->um_req = UMSG_REPS_CMD_INFO;
		msghdr->um_req_code = UMSG_REPS_CODE_RESP_INFO_PRO;

		info_resp.um_progress = reps_p->rs_op->rs_get_progress(reps_p);

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
repsc_do_channel(struct repsc_interface *repsc_p)
{
	char *log_header = "repsc_do_channel";
	struct repsc_private *priv_p = (struct repsc_private *)repsc_p->r_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_reps reps_msg;
	struct umessage_reps_hdr *msghdr_p;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_REPS_HEADER_LEN);

	msghdr_p = (struct umessage_reps_hdr *) &reps_msg;
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = *(uint32_t *)p;
	nid_log_warning("%s: req:%d, req_code:%d, um_len:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code, msghdr_p->um_len);

	if (!priv_p->p_repsg) {
		nid_log_warning("%s support_reps should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr_p->um_req) {
	case UMSG_REPS_CMD_HELLO:
		__repsc_hello(priv_p, msg_buf, (struct umessage_reps_hello *)msghdr_p);
		break;

	case UMSG_REPS_CMD_START:
		__repsc_start(priv_p, msg_buf, (struct umessage_reps_start *)msghdr_p);
		break;

	case UMSG_REPS_CMD_DISPLAY:
		__repsc_display(priv_p, msg_buf, (struct umessage_reps_display *)msghdr_p);
		break;

	case UMSG_REPS_CMD_INFO:
		__repsc_info(priv_p, msg_buf, (struct umessage_reps_info *)msghdr_p);
		break;

	case UMSG_REPS_CMD_PAUSE:
		__repsc_pause(priv_p, msg_buf, (struct umessage_reps_pause *)msghdr_p);
		break;

	case UMSG_REPS_CMD_CONTINUE:
		__repsc_continue(priv_p, msg_buf, (struct umessage_reps_continue *)msghdr_p);
		break;

	default:
		nid_log_warning("%s: got wrong req:%d", log_header, msghdr_p->um_req);
		break;
	}
}

static void
repsc_cleanup(struct repsc_interface *repsc_p)
{
	char *log_header = "repsc_cleanup";

	nid_log_debug("%s start, repsc_p: %p", log_header, repsc_p);
	if (repsc_p->r_private != NULL) {
		free(repsc_p->r_private);
		repsc_p->r_private = NULL;
	}
}

struct repsc_operations repsc_op = {
	.rsc_accept_new_channel = repsc_accept_new_channel,
	.rsc_do_channel = repsc_do_channel,
	.rsc_cleanup = repsc_cleanup,
};

int
repsc_initialization(struct repsc_interface *repsc_p, struct repsc_setup *setup)
{
	char *log_header = "repsc_initialization";
	struct repsc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = calloc(1, sizeof(*priv_p));
	repsc_p->r_private = priv_p;
	repsc_p->r_op = &repsc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_repsg = setup->repsg;

	return 0;
}
