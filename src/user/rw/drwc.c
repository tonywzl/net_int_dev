/*
 * drwc.c
 * 	Implementation of Device Read Write Channel Module
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
#include "umpk_drw_if.h"
#include "drwcg_if.h"
#include "drwc_if.h"
#include "drwg_if.h"
#include "drw_if.h"

struct drwc_private {
	struct drwcg_interface	*p_drwcg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
drwc_accept_new_channel(struct drwc_interface *drwc_p, int sfd)
{
	struct drwc_private *priv_p = drwc_p->drwc_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__drwc_add_drw(struct drwc_private *priv_p, char *msg_buf, struct umessage_drw_add *add_msg)
{
	char *log_header = "__drwc_add_drw";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct drwcg_interface *drwcg_p = priv_p->p_drwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_drw_hdr *msghdr;
	int ctype = NID_CTYPE_DRW, rc;
	struct umessage_drw_add_resp add_resp;
	char nothing_back;
	struct drwg_interface *drwg_p;
	char *drw_name, *exportname;
	int device_provision;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_drw_hdr *)add_msg;
	assert(msghdr->um_req == UMSG_DRW_CMD_ADD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_DRW_HEADER_LEN);
	if (cmd_len > UMSG_DRW_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_DRW_HEADER_LEN, cmd_len - UMSG_DRW_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, drw_name:%s",
		log_header, cmd_len, msghdr->um_req_code, add_msg->um_drw_name);
	drw_name = add_msg->um_drw_name;
	exportname = add_msg->um_exportname;
	device_provision = (int)add_msg->um_device_provision;
	drwg_p = drwcg_p->drwcg_op->drwcg_get_drwg(drwcg_p);

	msghdr = (struct umessage_drw_hdr *)&add_resp;
	msghdr->um_req = UMSG_DRW_CMD_ADD;
	msghdr->um_req_code = UMSG_DRW_CODE_RESP_ADD;
	memcpy(add_resp.um_drw_name, add_msg->um_drw_name, add_msg->um_drw_name_len);
	add_resp.um_drw_name_len  = add_msg->um_drw_name_len;

	add_resp.um_resp_code = drwg_p->rwg_op->rwg_add_drw(drwg_p, drw_name, exportname, device_provision);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__drwc_delete_drw(struct drwc_private *priv_p, char *msg_buf, struct umessage_drw_delete *delete_msg)
{
	char *log_header = "__drwc_delete";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_drw_hdr *msghdr;
	int ctype = NID_CTYPE_DRW, rc;
	char nothing_back;
	struct umessage_drw_delete_resp delete_resp;
	struct drwcg_interface *drwcg_p = priv_p->p_drwcg;
	struct drwg_interface *drwg_p;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_drw_hdr *)delete_msg;
	assert(msghdr->um_req == UMSG_DRW_CMD_DELETE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_DRW_HEADER_LEN);

	if (cmd_len > UMSG_DRW_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_DRW_HEADER_LEN, cmd_len - UMSG_DRW_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, drw_name:%s", log_header, cmd_len, msghdr->um_req_code, delete_msg->um_drw_name);

	switch (msghdr->um_req_code) {
	case UMSG_DRW_CODE_DELETE:
		drwg_p = drwcg_p->drwcg_op->drwcg_get_drwg(drwcg_p);
		delete_resp.um_drw_name_len = strlen(delete_msg->um_drw_name);
		strcpy(delete_resp.um_drw_name, delete_msg->um_drw_name);
		delete_resp.um_resp_code = drwg_p->rwg_op->rwg_delete_drw(drwg_p, delete_msg->um_drw_name);

		msghdr = (struct umessage_drw_hdr *)&delete_resp;
		msghdr->um_req = UMSG_DRW_CMD_DELETE;
		msghdr->um_req_code = UMSG_DRW_CODE_RESP_DELETE;

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
__drwc_do_device_ready(struct drwc_private *priv_p, char *msg_buf, struct umessage_drw_ready *ready_msg)
{
	char *log_header = "__drwc_do_device_ready";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct drwcg_interface *drwcg_p = priv_p->p_drwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_drw_hdr *msghdr;
	int ctype = NID_CTYPE_DRW, rc;
	struct umessage_drw_ready_resp ready_resp;
	char nothing_back;
	struct drwg_interface *drwg_p;
	char *drw_name;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_drw_hdr *)ready_msg;
	assert(msghdr->um_req == UMSG_DRW_CMD_READY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_DRW_HEADER_LEN);
	if (cmd_len > UMSG_DRW_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_DRW_HEADER_LEN, cmd_len - UMSG_DRW_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, drw_name:%s",
		log_header, cmd_len, msghdr->um_req_code, ready_msg->um_drw_name);
	drw_name = ready_msg->um_drw_name;
	drwg_p = drwcg_p->drwcg_op->drwcg_get_drwg(drwcg_p);

	msghdr = (struct umessage_drw_hdr *)&ready_resp;
	msghdr->um_req = UMSG_DRW_CMD_READY;
	msghdr->um_req_code = UMSG_DRW_CODE_RESP_READY;
	memcpy(ready_resp.um_drw_name, ready_msg->um_drw_name, ready_msg->um_drw_name_len);
	ready_resp.um_drw_name_len  = ready_msg->um_drw_name_len;

	ready_resp.um_resp_code = drwg_p->rwg_op->rwg_do_device_ready(drwg_p, drw_name);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__drwc_display(struct drwc_private *priv_p, char *msg_buf, struct umessage_drw_display *dis_msg)
{
	char *log_header = "__drwc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct drwcg_interface *drwcg_p = priv_p->p_drwcg;
	struct drwg_interface *drwg_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_drw_hdr *msghdr;
	int ctype = NID_CTYPE_DRW, rc;
	char nothing_back;
	struct umessage_drw_display_resp dis_resp;
	struct drw_setup *drw_setup_p;
	int *working_drw = NULL, cur_index = 0;
	int num_drw = 0, num_working_drw = 0, get_it = 0, found_drw = 0;
	int i, j;

	nid_log_warning("%s: start ...", log_header);
	drwg_p = drwcg_p->drwcg_op->drwcg_get_drwg(drwcg_p);
	msghdr = (struct umessage_drw_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_DRW_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_DRW_HEADER_LEN);

	if (cmd_len > UMSG_DRW_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_DRW_HEADER_LEN, cmd_len - UMSG_DRW_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, drw_name: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_drw_name);

	switch (msghdr->um_req_code){
	case UMSG_DRW_CODE_S_DISP:
		if (dis_msg->um_drw_name == NULL)
			goto out;
		drw_setup_p = drwg_p->rwg_op->rwg_get_all_drw_setup(drwg_p, &num_drw);
		if (drw_setup_p == NULL)
			goto out;
		for (i = 0; i < DRWG_MAX_DRW; i++, drw_setup_p++) {
			if (found_drw == num_drw)
				break;
			if (strlen(drw_setup_p->name) != 0) {
				found_drw++;
				if (!strcmp(drw_setup_p->name ,dis_msg->um_drw_name)) {
					get_it = 1;
					break;
				}
			}
		}
		if (get_it) {
			dis_resp.um_drw_name_len = strlen(drw_setup_p->name);
			memcpy(dis_resp.um_drw_name, drw_setup_p->name, dis_resp.um_drw_name_len);
			dis_resp.um_exportname_len = strlen(drw_setup_p->exportname);
			memcpy(dis_resp.um_exportname, drw_setup_p->exportname, dis_resp.um_exportname_len);
			dis_resp.um_simulate_async = drw_setup_p->simulate_async;
			dis_resp.um_simulate_delay = drw_setup_p->simulate_delay;
			dis_resp.um_simulate_delay_min_gap = drw_setup_p->simulate_delay_min_gap;
			dis_resp.um_simulate_delay_max_gap = drw_setup_p->simulate_delay_max_gap;
			dis_resp.um_simulate_delay_time_us = drw_setup_p->simulate_delay_time_us;
			dis_resp.um_device_provision = drw_setup_p->device_provision;

			msghdr = (struct umessage_drw_hdr *)&dis_resp;
			msghdr->um_req = UMSG_DRW_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_DRW_CODE_S_RESP_DISP;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;

	case UMSG_DRW_CODE_S_DISP_ALL:
		drw_setup_p = drwg_p->rwg_op->rwg_get_all_drw_setup(drwg_p, &num_drw);
		for (i = 0; i < DRWG_MAX_DRW; i++, drw_setup_p++) {
			if (found_drw == num_drw)
				break;
			if (strlen(drw_setup_p->name) != 0) {
				dis_resp.um_drw_name_len = strlen(drw_setup_p->name);
				memcpy(dis_resp.um_drw_name, drw_setup_p->name, dis_resp.um_drw_name_len);
				dis_resp.um_exportname_len = strlen(drw_setup_p->exportname);
				memcpy(dis_resp.um_exportname, drw_setup_p->exportname, dis_resp.um_exportname_len);
				dis_resp.um_simulate_async = drw_setup_p->simulate_async;
				dis_resp.um_simulate_delay = drw_setup_p->simulate_delay;
				dis_resp.um_simulate_delay_min_gap = drw_setup_p->simulate_delay_min_gap;
				dis_resp.um_simulate_delay_max_gap = drw_setup_p->simulate_delay_max_gap;
				dis_resp.um_simulate_delay_time_us = drw_setup_p->simulate_delay_time_us;
				dis_resp.um_device_provision = drw_setup_p->device_provision;

				msghdr = (struct umessage_drw_hdr *)&dis_resp;
				msghdr->um_req = UMSG_DRW_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_DRW_CODE_S_RESP_DISP_ALL;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				write(sfd, msg_buf, cmd_len);
				found_drw++;
			}
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_drw_hdr *)&dis_resp;
		msghdr->um_req = UMSG_DRW_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_DRW_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_DRW_CODE_W_DISP:
		if (dis_msg->um_drw_name == NULL)
			goto out;
		drw_setup_p = drwg_p->rwg_op->rwg_get_all_drw_setup(drwg_p, &num_drw);
		working_drw = drwg_p->rwg_op->rwg_get_working_drw_index(drwg_p, &num_working_drw);
		for (i = 0; i < num_drw; i++, drw_setup_p++) {
			if (!strcmp(drw_setup_p->name, dis_msg->um_drw_name))
				break;
		}

		if (i != num_drw) {
			for (j = 0; j < num_working_drw; j++) {
				if (working_drw[j] == i)
					break;
			}
		}
		else {
			goto out;
		}

		if (j != num_working_drw) {
			dis_resp.um_drw_name_len = strlen(drw_setup_p->name);
			memcpy(dis_resp.um_drw_name, drw_setup_p->name, dis_resp.um_drw_name_len);
			dis_resp.um_exportname_len = strlen(drw_setup_p->exportname);
			memcpy(dis_resp.um_exportname, drw_setup_p->exportname, dis_resp.um_exportname_len);
			dis_resp.um_simulate_async = drw_setup_p->simulate_async;
			dis_resp.um_simulate_delay = drw_setup_p->simulate_delay;
			dis_resp.um_simulate_delay_min_gap = drw_setup_p->simulate_delay_min_gap;
			dis_resp.um_simulate_delay_max_gap = drw_setup_p->simulate_delay_max_gap;
			dis_resp.um_simulate_delay_time_us = drw_setup_p->simulate_delay_time_us;
			dis_resp.um_device_provision = drw_setup_p->device_provision;
			msghdr = (struct umessage_drw_hdr *)&dis_resp;
			msghdr->um_req = UMSG_DRW_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_DRW_CODE_W_RESP_DISP;

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

	case UMSG_DRW_CODE_W_DISP_ALL:
		drw_setup_p = drwg_p->rwg_op->rwg_get_all_drw_setup(drwg_p, &num_drw);
		working_drw = drwg_p->rwg_op->rwg_get_working_drw_index(drwg_p, &num_working_drw);

		for (i = 0; i < num_working_drw; i++) {
			cur_index = working_drw[i];
			dis_resp.um_drw_name_len = strlen(drw_setup_p[cur_index].name);
			memcpy(dis_resp.um_drw_name, drw_setup_p[cur_index].name, dis_resp.um_drw_name_len);
			dis_resp.um_exportname_len = strlen(drw_setup_p[cur_index].exportname);
			memcpy(dis_resp.um_exportname, drw_setup_p[cur_index].exportname, dis_resp.um_exportname_len);
			dis_resp.um_simulate_async = drw_setup_p[cur_index].simulate_async;
			dis_resp.um_simulate_delay = drw_setup_p[cur_index].simulate_delay;
			dis_resp.um_simulate_delay_min_gap = drw_setup_p[cur_index].simulate_delay_min_gap;
			dis_resp.um_simulate_delay_max_gap = drw_setup_p[cur_index].simulate_delay_max_gap;
			dis_resp.um_simulate_delay_time_us = drw_setup_p[cur_index].simulate_delay_time_us;
			dis_resp.um_device_provision = drw_setup_p[cur_index].device_provision;

			msghdr = (struct umessage_drw_hdr *)&dis_resp;
			msghdr->um_req = UMSG_DRW_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_DRW_CODE_W_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_drw_hdr *)&dis_resp;
		msghdr->um_req = UMSG_DRW_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_DRW_CODE_DISP_END;

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
	free(working_drw);
	close(sfd);
}

static void
__drwc_hello(struct drwc_private *priv_p, char *msg_buf, struct umessage_drw_hello *hello_msg)
{
	char *log_header = "__drwc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_drw_hdr *msghdr;
	int ctype = NID_CTYPE_DRW, rc;
	char nothing_back;
	struct umessage_drw_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_drw_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_DRW_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_DRW_HEADER_LEN);

	if (cmd_len > UMSG_DRW_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_DRW_HEADER_LEN, cmd_len - UMSG_DRW_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_DRW_CODE_HELLO:
		msghdr = (struct umessage_drw_hdr *)&hello_resp;
		msghdr->um_req = UMSG_DRW_CMD_HELLO;
		msghdr->um_req_code = UMSG_DRW_CODE_RESP_HELLO;

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
drwc_do_channel(struct drwc_interface *drwc_p, struct scg_interface *scg_p)
{
	char *log_header = "drwc_do_channel";
	struct drwc_private *priv_p = (struct drwc_private *)drwc_p->drwc_private;
	struct drwcg_interface *drwcg_p = priv_p->p_drwcg;
	struct drwg_interface *drwg_p;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_drw drw_msg;
	struct umessage_drw_hdr *msghdr = (struct umessage_drw_hdr *)&drw_msg;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_DRW_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	nid_log_error("um_req: %u", msghdr->um_req);

	drwg_p = drwcg_p->drwcg_op->drwcg_get_drwg(drwcg_p);
	if (!drwg_p) {
		nid_log_warning("%s support_drw should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr->um_req) {
	case UMSG_DRW_CMD_ADD:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__drwc_add_drw(priv_p, msg_buf, (struct umessage_drw_add *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_DRW_CMD_DELETE:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__drwc_delete_drw(priv_p, msg_buf, (struct umessage_drw_delete *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_DRW_CMD_READY:
		__drwc_do_device_ready(priv_p, msg_buf, (struct umessage_drw_ready *)msghdr);
		break;

	case UMSG_DRW_CMD_DISPLAY:
		__drwc_display(priv_p, msg_buf, (struct umessage_drw_display *)msghdr);
		break;

	case UMSG_DRW_CMD_HELLO:
		__drwc_hello(priv_p, msg_buf, (struct umessage_drw_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
	}
}

static void
drwc_cleanup(struct drwc_interface *drwc_p)
{
	nid_log_debug("drwc_cleanup start, drwc_p:%p", drwc_p);
	if (drwc_p->drwc_private != NULL) {
		free(drwc_p->drwc_private);
		drwc_p->drwc_private = NULL;
	}
}

struct drwc_operations drwc_op = {
	.drwc_accept_new_channel = drwc_accept_new_channel,
	.drwc_do_channel = drwc_do_channel,
	.drwc_cleanup = drwc_cleanup,
};

int
drwc_initialization(struct drwc_interface *drwc_p, struct drwc_setup *setup)
{
	char *log_header = "drwc_initialization";
	struct drwc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	drwc_p->drwc_private = priv_p;
	drwc_p->drwc_op = &drwc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_drwcg = setup->drwcg;
	return 0;
}
