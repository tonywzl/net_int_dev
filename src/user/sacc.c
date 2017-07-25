/*
 * sacc.c
 * 	Implementation of Server Agent Channel (sac) Channel Module
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
#include "umpk_sac_if.h"
#include "saccg_if.h"
#include "sacc_if.h"
#include "sacg_if.h"
#include "sac_if.h"
#include "wc_if.h"

struct sacc_private {
	struct sacg_interface	*p_sacg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
sacc_accept_new_channel(struct sacc_interface *sacc_p, int sfd)
{
	struct sacc_private *priv_p = sacc_p->sacc_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__sacc_information(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_information *info_msg)
{
	char *log_header = "__sacc_information_sac";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	struct sac_interface *sac_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr;
	int ctype = NID_CTYPE_SAC, rc;
	struct sac_stat stat;
	char nothing_back;
	char *chan_uuid, **working_sac = NULL;
	int num_sac = 0, i;
	struct timeval now;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sac_hdr *)info_msg;
	assert(msghdr->um_req == UMSG_SAC_CMD_INFORMATION);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);
	if (cmd_len > UMSG_SAC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, info_msg->um_chan_uuid);
	switch (msghdr->um_req_code) {
	case UMSG_SAC_CODE_STAT: {
		struct umessage_sac_information_resp info_resp;
		memset(&info_resp, 0, sizeof(info_resp));
		chan_uuid = info_msg->um_chan_uuid;
		rc = sacg_p->sag_op->sag_get_counter(sacg_p, chan_uuid, &info_resp.um_bfe_page_counter, &info_resp.um_bre_page_counter);
		if (rc)
			goto out;

		sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, chan_uuid);
		sac_p->sa_op->sa_get_stat(sac_p, &stat);

		msghdr = (struct umessage_sac_hdr *)&info_resp;
		msghdr->um_req = UMSG_SAC_CMD_INFORMATION;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_STAT;

		info_resp.um_chan_uuid_len  = info_msg->um_chan_uuid_len;
		memcpy(info_resp.um_chan_uuid, info_msg->um_chan_uuid, info_msg->um_chan_uuid_len);
		info_resp.um_state = stat.sa_stat;
		if (stat.sa_stat == NID_STAT_INACTIVE)
			goto do_encode;

		if (stat.sa_ipaddr) {
			info_resp.um_ip_len  = strlen(stat.sa_ipaddr);
			memcpy(info_resp.um_ip, stat.sa_ipaddr, info_resp.um_ip_len);
		}
		info_resp.um_alevel = stat.sa_alevel;

		info_resp.um_rcounter = stat.sa_read_counter;
		info_resp.um_r_rcounter = stat.sa_read_resp_counter;
		info_resp.um_rreadycounter = stat.sa_read_ready_counter;

		info_resp.um_wcounter = stat.sa_write_counter;
		info_resp.um_wreadycounter = stat.sa_write_ready_counter;
		info_resp.um_r_wcounter = stat.sa_write_resp_counter;

		info_resp.um_kcounter = stat.sa_keepalive_counter;
		info_resp.um_r_kcounter = stat.sa_keepalive_resp_counter;

		info_resp.um_recv_sequence = stat.sa_recv_sequence;
		info_resp.um_wait_sequence = stat.sa_wait_sequence;

		gettimeofday(&now, NULL);
		info_resp.um_live_time = now.tv_sec - stat.sa_start_tv.tv_sec;

		if (stat.sa_io_type == IO_TYPE_BUFFER) {
			info_resp.um_rtree_segsz = stat.sa_io_stat.s_rtree_segsz;
			info_resp.um_rtree_nseg = stat.sa_io_stat.s_rtree_nseg;
			info_resp.um_rtree_nfree = stat.sa_io_stat.s_rtree_nfree;
			info_resp.um_rtree_nused = stat.sa_io_stat.s_rtree_nused;

			info_resp.um_btn_segsz = stat.sa_io_stat.s_btn_segsz;
			info_resp.um_btn_nseg = stat.sa_io_stat.s_btn_nseg;
			info_resp.um_btn_nfree = stat.sa_io_stat.s_btn_nfree;
			info_resp.um_btn_nused = stat.sa_io_stat.s_btn_nused;
		}

do_encode:
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);
		break;
	}

	case UMSG_SAC_CODE_STAT_ALL: {
		struct umessage_sac_information_resp info_resp;
		memset(&info_resp, 0, sizeof(info_resp));
		working_sac = sacg_p->sag_op->sag_get_working_sac_name(sacg_p, &num_sac);
		for (i = 0; i < num_sac; i++) {
			chan_uuid = working_sac[i];
			rc = sacg_p->sag_op->sag_get_counter(sacg_p, chan_uuid, &info_resp.um_bfe_page_counter, &info_resp.um_bre_page_counter);
			if (rc)
				goto out;

			sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, working_sac[i]);
			sac_p->sa_op->sa_get_stat(sac_p, &stat);

			msghdr = (struct umessage_sac_hdr *)&info_resp;
			msghdr->um_req = UMSG_SAC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_SAC_CODE_RESP_STAT_ALL;

			info_resp.um_chan_uuid_len  = strlen(working_sac[i]);
			memcpy(info_resp.um_chan_uuid, working_sac[i], info_resp.um_chan_uuid_len);
			info_resp.um_state = stat.sa_stat;
			if (stat.sa_stat == NID_STAT_INACTIVE)
				goto all_do_encode;

			if (stat.sa_ipaddr) {
				info_resp.um_ip_len  = strlen(stat.sa_ipaddr);
				memcpy(info_resp.um_ip, stat.sa_ipaddr, info_resp.um_ip_len);
			}
			info_resp.um_alevel = stat.sa_alevel;

			info_resp.um_rcounter = stat.sa_read_counter;
			info_resp.um_r_rcounter = stat.sa_read_resp_counter;
			info_resp.um_rreadycounter = stat.sa_read_ready_counter;

			info_resp.um_wcounter = stat.sa_write_counter;
			info_resp.um_wreadycounter = stat.sa_write_ready_counter;
			info_resp.um_r_wcounter = stat.sa_write_resp_counter;

			info_resp.um_kcounter = stat.sa_keepalive_counter;
			info_resp.um_r_kcounter = stat.sa_keepalive_resp_counter;

			info_resp.um_recv_sequence = stat.sa_recv_sequence;
			info_resp.um_wait_sequence = stat.sa_wait_sequence;

			gettimeofday(&now, NULL);
			info_resp.um_live_time = now.tv_sec - stat.sa_start_tv.tv_sec;

			if (stat.sa_io_type == IO_TYPE_BUFFER) {
				info_resp.um_rtree_segsz = stat.sa_io_stat.s_rtree_segsz;
				info_resp.um_rtree_nseg = stat.sa_io_stat.s_rtree_nseg;
				info_resp.um_rtree_nfree = stat.sa_io_stat.s_rtree_nfree;
				info_resp.um_rtree_nused = stat.sa_io_stat.s_rtree_nused;

				info_resp.um_btn_segsz = stat.sa_io_stat.s_btn_segsz;
				info_resp.um_btn_nseg = stat.sa_io_stat.s_btn_nseg;
				info_resp.um_btn_nfree = stat.sa_io_stat.s_btn_nfree;
				info_resp.um_btn_nused = stat.sa_io_stat.s_btn_nused;
			}
all_do_encode:
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}
		memset(&info_resp, 0, sizeof(info_resp));
		msghdr = (struct umessage_sac_hdr *)&info_resp;
		msghdr->um_req = UMSG_SAC_CMD_INFORMATION;
		msghdr->um_req_code = UMSG_SAC_CODE_STAT_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);
		break;
	}

	case UMSG_SAC_CODE_LIST_STAT: {
		struct umessage_sac_list_stat_resp info_resp;
		memset(&info_resp, 0, sizeof(info_resp));
		chan_uuid = info_msg->um_chan_uuid;
		rc = sacg_p->sag_op->sag_get_list_stat(sacg_p, chan_uuid, &info_resp);
		if (rc)
			goto out;

		msghdr = (struct umessage_sac_hdr *)&info_resp;
		msghdr->um_req = UMSG_SAC_CMD_INFORMATION;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_LIST_STAT;

		info_resp.um_chan_uuid_len  = info_msg->um_chan_uuid_len;
		memcpy(info_resp.um_chan_uuid, info_msg->um_chan_uuid, info_msg->um_chan_uuid_len);

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);
		break;
	}
	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}
out:
	free(working_sac);
	close(sfd);
}

static void
__sacc_add_sac(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_add *add_msg)
{
	char *log_header = "__sacc_add_sac";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr;
	int ctype = NID_CTYPE_SAC, rc;
	struct umessage_sac_add_resp add_resp;
	char nothing_back;
	char do_sync, direct_io, enable_kill_myself;
	int alignment, dev_size;
	char *chan_uuid, *ds_name, *dev_name, *exportname, *tp_name;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sac_hdr *)add_msg;
	assert(msghdr->um_req == UMSG_SAC_CMD_ADD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);
	if (cmd_len > UMSG_SAC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, add_msg->um_chan_uuid);
	chan_uuid = add_msg->um_chan_uuid;
	do_sync = (char)add_msg->um_sync;
	direct_io = (char)add_msg->um_direct_io;
	enable_kill_myself = (char)add_msg->um_enable_kill_myself;
	alignment = (int)add_msg->um_alignment;
	ds_name = add_msg->um_ds_name;
	dev_name = add_msg->um_dev_name;
	exportname = add_msg->um_exportname;
	dev_size = add_msg->um_dev_size;
	tp_name = add_msg->um_tp_name;

	msghdr = (struct umessage_sac_hdr *)&add_resp;
	msghdr->um_req = UMSG_SAC_CMD_ADD;
	msghdr->um_req_code = UMSG_SAC_CODE_RESP_ADD;
	memcpy(add_resp.um_chan_uuid, add_msg->um_chan_uuid, add_msg->um_chan_uuid_len);
	add_resp.um_chan_uuid_len  = add_msg->um_chan_uuid_len;

	add_resp.um_resp_code = sacg_p->sag_op->sag_add_sac(sacg_p, chan_uuid, do_sync, direct_io, enable_kill_myself,
			alignment, ds_name, dev_name, exportname, dev_size, tp_name);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__sacc_delete_sac(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_delete *del_msg)
{
	char *log_header = "__sacc_delete_sac";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr;
	int ctype = NID_CTYPE_SAC, rc;
	struct umessage_sac_delete_resp del_resp;
	char nothing_back;
	char *chan_uuid;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sac_hdr *)del_msg;
	assert(msghdr->um_req == UMSG_SAC_CMD_DELETE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);
	if (cmd_len > UMSG_SAC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, del_msg->um_chan_uuid);
	chan_uuid = del_msg->um_chan_uuid;

	msghdr = (struct umessage_sac_hdr *)&del_resp;
	msghdr->um_req = UMSG_SAC_CMD_DELETE;
	msghdr->um_req_code = UMSG_SAC_CODE_RESP_DELETE;
	memcpy(del_resp.um_chan_uuid, del_msg->um_chan_uuid, del_msg->um_chan_uuid_len);
	del_resp.um_chan_uuid_len  = del_msg->um_chan_uuid_len;

	del_resp.um_resp_code = sacg_p->sag_op->sag_delete_sac(sacg_p, chan_uuid);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}


static void
__sacc_set_keepalive_sac(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_set_keepalive *set_keepalive_msg)
{
	char *log_header = "__sacc_set_keepalive_sac";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr;
	int ctype = NID_CTYPE_SAC, rc;
	struct umessage_sac_set_keepalive_resp set_keepalive_resp;
	char nothing_back;
	char *chan_uuid;
	uint16_t enable_keepalive, max_keepalive;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sac_hdr *)set_keepalive_msg;
	assert(msghdr->um_req == UMSG_SAC_CMD_SET_KEEPALIVE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);
	if (cmd_len > UMSG_SAC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, set_keepalive_msg);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, set_keepalive_msg->um_chan_uuid);
	chan_uuid = set_keepalive_msg->um_chan_uuid;

	msghdr = (struct umessage_sac_hdr *)&set_keepalive_resp;
	msghdr->um_req = UMSG_SAC_CMD_SET_KEEPALIVE;
	msghdr->um_req_code = UMSG_SAC_CODE_RESP_SET_KEEPALIVE;
	memcpy(set_keepalive_resp.um_chan_uuid, set_keepalive_msg->um_chan_uuid, set_keepalive_msg->um_chan_uuid_len);
	set_keepalive_resp.um_chan_uuid_len  = set_keepalive_msg->um_chan_uuid_len;

	enable_keepalive = set_keepalive_msg->um_enable_keepalive;
	max_keepalive = set_keepalive_msg->um_max_keepalive;

	set_keepalive_resp.um_resp_code = sacg_p->sag_op->sag_set_keepalive_sac(sacg_p, chan_uuid, enable_keepalive, max_keepalive);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__sacc_switch_sac(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_switch *swm_msg)
{
	char *log_header = "__sacc_switch_sac";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr;
	int ctype = NID_CTYPE_SAC, rc;
	struct umessage_sac_delete_resp swm_resp;
	char nothing_back;
	char *chan_uuid, *bwc_uuid;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sac_hdr *)swm_msg;
	assert(msghdr->um_req == UMSG_SAC_CMD_SWITCH);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);
	if (cmd_len > UMSG_SAC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, swm_msg->um_chan_uuid);
	chan_uuid = swm_msg->um_chan_uuid;
	bwc_uuid = swm_msg->um_bwc_uuid;

	msghdr = (struct umessage_sac_hdr *)&swm_resp;
	msghdr->um_req = UMSG_SAC_CMD_SWITCH;
	msghdr->um_req_code = UMSG_SAC_CODE_RESP_SWITCH;
	memcpy(swm_resp.um_chan_uuid, swm_msg->um_chan_uuid, swm_msg->um_chan_uuid_len);
	swm_resp.um_chan_uuid_len  = swm_msg->um_chan_uuid_len;

	swm_resp.um_resp_code = sacg_p->sag_op->sag_switch_sac(sacg_p, chan_uuid, bwc_uuid, WC_TYPE_NONE_MEMORY);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__sacc_fast_release(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_fast_release *dis_msg)
{
	char *log_header = "__sacc_fast_release";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr;
	int ctype = NID_CTYPE_SAC, rc;
	char nothing_back;
	struct umessage_sac_fast_release_resp dis_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sac_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_SAC_CMD_FAST_RELEASE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);

	if (cmd_len > UMSG_SAC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, chan_uuid: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_chan_uuid);

	switch (msghdr->um_req_code){
	case UMSG_SAC_CODE_START:
		if (dis_msg->um_chan_uuid == NULL)
			goto out;
		dis_resp.um_resp_code = sacg_p->sag_op->sag_start_fast_release(sacg_p, dis_msg->um_chan_uuid);

		msghdr = (struct umessage_sac_hdr *)&dis_resp;
		msghdr->um_req = UMSG_SAC_CMD_FAST_RELEASE;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_START;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_STOP:
		if (dis_msg->um_chan_uuid == NULL)
			goto out;
		dis_resp.um_resp_code = sacg_p->sag_op->sag_stop_fast_release(sacg_p, dis_msg->um_chan_uuid);

		msghdr = (struct umessage_sac_hdr *)&dis_resp;
		msghdr->um_req = UMSG_SAC_CMD_FAST_RELEASE;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_STOP;

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
__sacc_ioinfo(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_ioinfo *c_msg)
{
	char *log_header = "__sacc_ioinfo";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	struct sac_interface *sac_p;
	struct sac_req_stat stat_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr = (struct umessage_sac_hdr *)c_msg;
	struct umessage_sac_ioinfo_resp msg_resp;
	int ctype = NID_CTYPE_SAC, rc;
	char nothing_back;
	char **working_sac = NULL;
	int num_working_sac = 0, i;

	assert(msghdr->um_req == UMSG_SAC_CMD_IOINFO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);
	if (cmd_len > UMSG_SAC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)c_msg);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_SAC_CODE_IOINFO_START:
		sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, c_msg->um_chan_uuid);
		if (!sac_p) {
			nid_log_warning("%s: did not find sac %s", log_header, c_msg->um_chan_uuid);
			goto out;
		}
		sac_p->sa_op->sa_start_req_counter(sac_p);
		sac_p->sa_op->sa_check_req_counter(sac_p, &stat_p);

		msghdr = (struct umessage_sac_hdr *)&msg_resp;
		msghdr->um_req = UMSG_SAC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_START;

		msg_resp.um_chan_uuid_len = c_msg->um_chan_uuid_len;
		memcpy(msg_resp.um_chan_uuid, c_msg->um_chan_uuid, c_msg->um_chan_uuid_len);
		msg_resp.um_is_running = stat_p.sa_is_running;
		msg_resp.um_req_num = stat_p.sa_req_num;
		msg_resp.um_req_len = stat_p.sa_req_len;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_IOINFO_STOP:
		sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, c_msg->um_chan_uuid);
		if (!sac_p) {
			nid_log_warning("%s: did not find sac %s", log_header, c_msg->um_chan_uuid);
			goto out;
		}
		sac_p->sa_op->sa_stop_req_counter(sac_p);
		sac_p->sa_op->sa_check_req_counter(sac_p, &stat_p);

		msghdr = (struct umessage_sac_hdr *)&msg_resp;
		msghdr->um_req = UMSG_SAC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_STOP;

		msg_resp.um_chan_uuid_len = c_msg->um_chan_uuid_len;
		memcpy(msg_resp.um_chan_uuid, c_msg->um_chan_uuid, c_msg->um_chan_uuid_len);
		msg_resp.um_is_running = stat_p.sa_is_running;
		msg_resp.um_req_num = stat_p.sa_req_num;
		msg_resp.um_req_len = stat_p.sa_req_len;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_IOINFO_CHECK:
		sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, c_msg->um_chan_uuid);
		if (!sac_p) {
			nid_log_warning("%s: did not find sac %s", log_header, c_msg->um_chan_uuid);
			goto out;
		}
		sac_p->sa_op->sa_check_req_counter(sac_p, &stat_p);

		msghdr = (struct umessage_sac_hdr *)&msg_resp;
		msghdr->um_req = UMSG_SAC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_CHECK;

		msg_resp.um_chan_uuid_len = c_msg->um_chan_uuid_len;
		memcpy(msg_resp.um_chan_uuid, c_msg->um_chan_uuid, c_msg->um_chan_uuid_len);
		msg_resp.um_is_running = stat_p.sa_is_running;
		msg_resp.um_req_num = stat_p.sa_req_num;
		msg_resp.um_req_len = stat_p.sa_req_len;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_IOINFO_START_ALL:
		working_sac = sacg_p->sag_op->sag_get_working_sac_name(sacg_p, &num_working_sac);
		for (i = 0; i < num_working_sac; i++) {
			sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, working_sac[i]);
			if (!sac_p)
				continue;
			sac_p->sa_op->sa_start_req_counter(sac_p);
			sac_p->sa_op->sa_check_req_counter(sac_p, &stat_p);

			msghdr = (struct umessage_sac_hdr *)&msg_resp;
			msghdr->um_req = UMSG_SAC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_START_ALL;

			msg_resp.um_chan_uuid_len = strlen(working_sac[i]);
			memcpy(msg_resp.um_chan_uuid, working_sac[i], msg_resp.um_chan_uuid_len);
			msg_resp.um_is_running = stat_p.sa_is_running;
			msg_resp.um_req_num = stat_p.sa_req_num;
			msg_resp.um_req_len = stat_p.sa_req_len;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}

		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_sac_hdr *)&msg_resp;
		msghdr->um_req = UMSG_SAC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_IOINFO_STOP_ALL:
		working_sac = sacg_p->sag_op->sag_get_working_sac_name(sacg_p, &num_working_sac);
		for (i = 0; i < num_working_sac; i++) {
			sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, working_sac[i]);
			if (!sac_p)
				continue;
			sac_p->sa_op->sa_stop_req_counter(sac_p);
			sac_p->sa_op->sa_check_req_counter(sac_p, &stat_p);

			msghdr = (struct umessage_sac_hdr *)&msg_resp;
			msghdr->um_req = UMSG_SAC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_STOP_ALL;

			msg_resp.um_chan_uuid_len = strlen(working_sac[i]);
			memcpy(msg_resp.um_chan_uuid, working_sac[i], msg_resp.um_chan_uuid_len);
			msg_resp.um_is_running = stat_p.sa_is_running;
			msg_resp.um_req_num = stat_p.sa_req_num;
			msg_resp.um_req_len = stat_p.sa_req_len;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}

		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_sac_hdr *)&msg_resp;
		msghdr->um_req = UMSG_SAC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_IOINFO_CHECK_ALL:
		working_sac = sacg_p->sag_op->sag_get_working_sac_name(sacg_p, &num_working_sac);
		for (i = 0; i < num_working_sac; i++) {
			sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, working_sac[i]);
			if (!sac_p)
				continue;
			sac_p->sa_op->sa_check_req_counter(sac_p, &stat_p);

			msghdr = (struct umessage_sac_hdr *)&msg_resp;
			msghdr->um_req = UMSG_SAC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_CHECK_ALL;

			msg_resp.um_chan_uuid_len = strlen(working_sac[i]);
			memcpy(msg_resp.um_chan_uuid, working_sac[i], msg_resp.um_chan_uuid_len);
			msg_resp.um_is_running = stat_p.sa_is_running;
			msg_resp.um_req_num = stat_p.sa_req_num;
			msg_resp.um_req_len = stat_p.sa_req_len;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}

		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_sac_hdr *)&msg_resp;
		msghdr->um_req = UMSG_SAC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
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
__sacc_display(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_display *dis_msg)
{
	char *log_header = "__sacc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr;
	int ctype = NID_CTYPE_SAC, rc;
	char nothing_back;
	struct umessage_sac_display_resp dis_resp;
	struct sac_info *sac_info_p, *cur_info_p;
	struct sac_interface *sac_p;
	char **working_sac = NULL;
	int num_sac = 0, num_working_sac = 0, i, j;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sac_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_SAC_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);

	if (cmd_len > UMSG_SAC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, chan_uuid: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_chan_uuid);

	switch (msghdr->um_req_code){
	case UMSG_SAC_CODE_S_DISP:
		if (dis_msg->um_chan_uuid == NULL)
			goto out;
		sac_info_p = sacg_p->sag_op->sag_get_sac_info(sacg_p, dis_msg->um_chan_uuid);
		if (sac_info_p == NULL)
			goto out;
		dis_resp.um_chan_uuid_len = strlen(sac_info_p->uuid);
		memcpy(dis_resp.um_chan_uuid, sac_info_p->uuid, dis_resp.um_chan_uuid_len);
		dis_resp.um_sync = sac_info_p->sync;
		dis_resp.um_direct_io = sac_info_p->direct_io;
		dis_resp.um_enable_kill_myself = sac_info_p->enable_kill_myself;
		dis_resp.um_alignment = sac_info_p->alignment;
		dis_resp.um_ds_name_len = strlen(sac_info_p->ds_name);
		memcpy(dis_resp.um_ds_name, sac_info_p->ds_name , dis_resp.um_ds_name_len);
		dis_resp.um_dev_name_len = strlen(sac_info_p->dev_name);
		memcpy(dis_resp.um_dev_name, sac_info_p->dev_name, dis_resp.um_dev_name_len);
		dis_resp.um_export_name_len = strlen(sac_info_p->export_name);
		memcpy(dis_resp.um_export_name, sac_info_p->export_name, dis_resp.um_export_name_len);
		dis_resp.um_dev_size = sac_info_p->dev_size;
		dis_resp.um_tp_name_len = strlen(sac_info_p->tp_name);
		memcpy(dis_resp.um_tp_name, sac_info_p->tp_name, dis_resp.um_tp_name_len);
		dis_resp.um_wc_uuid_len = strlen(sac_info_p->wc_uuid);
		memcpy(dis_resp.um_wc_uuid, sac_info_p->wc_uuid, dis_resp.um_wc_uuid_len);
		dis_resp.um_rc_uuid_len = strlen(sac_info_p->rc_uuid);
		memcpy(dis_resp.um_rc_uuid, sac_info_p->rc_uuid, dis_resp.um_rc_uuid_len);
		dis_resp.um_io_type = sac_info_p->io_type;
		dis_resp.um_ds_type = sac_info_p->ds_type;
		dis_resp.um_wc_type = sac_info_p->wc_type;
		dis_resp.um_rc_type = sac_info_p->rc_type;
		dis_resp.um_ready = sac_info_p->ready;

		msghdr = (struct umessage_sac_hdr *)&dis_resp;
		msghdr->um_req = UMSG_SAC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_SAC_CODE_S_RESP_DISP;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_S_DISP_ALL:
		sac_info_p = sacg_p->sag_op->sag_get_all_sac_info(sacg_p, &num_sac);
		for (i = 0; i < num_sac; i++, sac_info_p++) {
			dis_resp.um_chan_uuid_len = strlen(sac_info_p->uuid);
			memcpy(dis_resp.um_chan_uuid, sac_info_p->uuid, dis_resp.um_chan_uuid_len);
			dis_resp.um_sync = sac_info_p->sync;
			dis_resp.um_direct_io = sac_info_p->direct_io;
			dis_resp.um_enable_kill_myself = sac_info_p->enable_kill_myself;
			dis_resp.um_alignment = sac_info_p->alignment;
			dis_resp.um_ds_name_len = strlen(sac_info_p->ds_name);
			memcpy(dis_resp.um_ds_name, sac_info_p->ds_name , dis_resp.um_ds_name_len);
			dis_resp.um_dev_name_len = strlen(sac_info_p->dev_name);
			memcpy(dis_resp.um_dev_name, sac_info_p->dev_name, dis_resp.um_dev_name_len);
			dis_resp.um_export_name_len = strlen(sac_info_p->export_name);
			memcpy(dis_resp.um_export_name, sac_info_p->export_name, dis_resp.um_export_name_len);
			dis_resp.um_dev_size = sac_info_p->dev_size;
			dis_resp.um_tp_name_len = strlen(sac_info_p->tp_name);
			memcpy(dis_resp.um_tp_name, sac_info_p->tp_name, dis_resp.um_tp_name_len);
			dis_resp.um_wc_uuid_len = strlen(sac_info_p->wc_uuid);
			memcpy(dis_resp.um_wc_uuid, sac_info_p->wc_uuid, dis_resp.um_wc_uuid_len);
			dis_resp.um_rc_uuid_len = strlen(sac_info_p->rc_uuid);
			memcpy(dis_resp.um_rc_uuid, sac_info_p->rc_uuid, dis_resp.um_rc_uuid_len);
			dis_resp.um_io_type = sac_info_p->io_type;
			dis_resp.um_ds_type = sac_info_p->ds_type;
			dis_resp.um_wc_type = sac_info_p->wc_type;
			dis_resp.um_rc_type = sac_info_p->rc_type;
			dis_resp.um_ready = sac_info_p->ready;

			msghdr = (struct umessage_sac_hdr *)&dis_resp;
			msghdr->um_req = UMSG_SAC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_SAC_CODE_S_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_sac_hdr *)&dis_resp;
		msghdr->um_req = UMSG_SAC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_W_DISP:
		if (dis_msg->um_chan_uuid == NULL)
			goto out;
		sac_p = sacg_p->sag_op->sag_get_sac(sacg_p, dis_msg->um_chan_uuid);
		if (sac_p == NULL)
			goto out;
		sac_info_p = sacg_p->sag_op->sag_get_sac_info(sacg_p, dis_msg->um_chan_uuid);
		dis_resp.um_chan_uuid_len = strlen(sac_info_p->uuid);
		memcpy(dis_resp.um_chan_uuid, sac_info_p->uuid, dis_resp.um_chan_uuid_len);
		dis_resp.um_sync = sac_info_p->sync;
		dis_resp.um_direct_io = sac_info_p->direct_io;
		dis_resp.um_enable_kill_myself = sac_info_p->enable_kill_myself;
		dis_resp.um_alignment = sac_info_p->alignment;
		dis_resp.um_ds_name_len = strlen(sac_info_p->ds_name);
		memcpy(dis_resp.um_ds_name, sac_info_p->ds_name , dis_resp.um_ds_name_len);
		dis_resp.um_dev_name_len = strlen(sac_info_p->dev_name);
		memcpy(dis_resp.um_dev_name, sac_info_p->dev_name, dis_resp.um_dev_name_len);
		dis_resp.um_export_name_len = strlen(sac_info_p->export_name);
		memcpy(dis_resp.um_export_name, sac_info_p->export_name, dis_resp.um_export_name_len);
		dis_resp.um_dev_size = sac_info_p->dev_size;
		dis_resp.um_tp_name_len = strlen(sac_info_p->tp_name);
		memcpy(dis_resp.um_tp_name, sac_info_p->tp_name, dis_resp.um_tp_name_len);
		dis_resp.um_wc_uuid_len = strlen(sac_info_p->wc_uuid);
		memcpy(dis_resp.um_wc_uuid, sac_info_p->wc_uuid, dis_resp.um_wc_uuid_len);
		dis_resp.um_rc_uuid_len = strlen(sac_info_p->rc_uuid);
		memcpy(dis_resp.um_rc_uuid, sac_info_p->rc_uuid, dis_resp.um_rc_uuid_len);
		dis_resp.um_io_type = sac_info_p->io_type;
		dis_resp.um_ds_type = sac_info_p->ds_type;
		dis_resp.um_wc_type = sac_info_p->wc_type;
		dis_resp.um_rc_type = sac_info_p->rc_type;
		dis_resp.um_ready = sac_info_p->ready;

		msghdr = (struct umessage_sac_hdr *)&dis_resp;
		msghdr->um_req = UMSG_SAC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_SAC_CODE_W_RESP_DISP;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_SAC_CODE_W_DISP_ALL:
		working_sac = sacg_p->sag_op->sag_get_working_sac_name(sacg_p, &num_working_sac);
		sac_info_p = sacg_p->sag_op->sag_get_all_sac_info(sacg_p, &num_sac);
		for (i = 0; i < num_working_sac; i++) {
			cur_info_p = sac_info_p;
			for (j = 0; j < num_sac; j++, cur_info_p++) {
				if (!strcmp(working_sac[i], cur_info_p->uuid)) {
					dis_resp.um_chan_uuid_len = strlen(cur_info_p->uuid);
					memcpy(dis_resp.um_chan_uuid, cur_info_p->uuid, dis_resp.um_chan_uuid_len);
					dis_resp.um_sync = cur_info_p->sync;
					dis_resp.um_direct_io = cur_info_p->direct_io;
					dis_resp.um_enable_kill_myself = sac_info_p->enable_kill_myself;
					dis_resp.um_alignment = cur_info_p->alignment;
					dis_resp.um_ds_name_len = strlen(cur_info_p->ds_name);
					memcpy(dis_resp.um_ds_name, cur_info_p->ds_name , dis_resp.um_ds_name_len);
					dis_resp.um_dev_name_len = strlen(cur_info_p->dev_name);
					memcpy(dis_resp.um_dev_name, cur_info_p->dev_name, dis_resp.um_dev_name_len);
					dis_resp.um_export_name_len = strlen(cur_info_p->export_name);
					memcpy(dis_resp.um_export_name, cur_info_p->export_name, dis_resp.um_export_name_len);
					dis_resp.um_dev_size = cur_info_p->dev_size;
					dis_resp.um_tp_name_len = strlen(cur_info_p->tp_name);
					memcpy(dis_resp.um_tp_name, cur_info_p->tp_name, dis_resp.um_tp_name_len);
					dis_resp.um_wc_uuid_len = strlen(cur_info_p->wc_uuid);
					memcpy(dis_resp.um_wc_uuid, cur_info_p->wc_uuid, dis_resp.um_wc_uuid_len);
					dis_resp.um_rc_uuid_len = strlen(cur_info_p->rc_uuid);
					memcpy(dis_resp.um_rc_uuid, cur_info_p->rc_uuid, dis_resp.um_rc_uuid_len);
					dis_resp.um_io_type = cur_info_p->io_type;
					dis_resp.um_ds_type = cur_info_p->ds_type;
					dis_resp.um_wc_type = cur_info_p->wc_type;
					dis_resp.um_rc_type = cur_info_p->rc_type;
					dis_resp.um_ready = cur_info_p->ready;

					msghdr = (struct umessage_sac_hdr *)&dis_resp;
					msghdr->um_req = UMSG_SAC_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_SAC_CODE_W_RESP_DISP_ALL;

					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					write(sfd, msg_buf, cmd_len);
				}
			}
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_sac_hdr *)&dis_resp;
		msghdr->um_req = UMSG_SAC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_END;

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
	free(working_sac);
	close(sfd);
}

static void
__sacc_hello(struct sacc_private *priv_p, char *msg_buf, struct umessage_sac_hello *hello_msg)
{
	char *log_header = "__sacc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_sac_hdr *msghdr;
	int ctype = NID_CTYPE_SAC, rc;
	char nothing_back;
	struct umessage_sac_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_sac_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_SAC_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_SAC_HEADER_LEN);

	if (cmd_len > UMSG_SAC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_SAC_HEADER_LEN, cmd_len - UMSG_SAC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_SAC_CODE_HELLO:
		msghdr = (struct umessage_sac_hdr *)&hello_resp;
		msghdr->um_req = UMSG_SAC_CMD_HELLO;
		msghdr->um_req_code = UMSG_SAC_CODE_RESP_HELLO;

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
sacc_do_channel(struct sacc_interface *sacc_p, struct scg_interface *scg_p)
{
	char *log_header = "sacc_do_channel";
	struct sacc_private *priv_p = (struct sacc_private *)sacc_p->sacc_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_sac sac_msg;
	struct umessage_sac_hdr *msghdr = (struct umessage_sac_hdr *)&sac_msg;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_SAC_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	nid_log_error("um_req: %u", msghdr->um_req);

	if (!priv_p->p_sacg) {
		nid_log_warning("%s support_sac should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr->um_req) {
	case UMSG_SAC_CMD_INFORMATION:
		__sacc_information(priv_p, msg_buf, (struct umessage_sac_information *)msghdr);
		break;

	case UMSG_SAC_CMD_ADD:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__sacc_add_sac(priv_p, msg_buf, (struct umessage_sac_add *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_SAC_CMD_DELETE:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__sacc_delete_sac(priv_p, msg_buf, (struct umessage_sac_delete *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_SAC_CMD_SWITCH:
		__sacc_switch_sac(priv_p, msg_buf, (struct umessage_sac_switch *)msghdr);
		break;

	case UMSG_SAC_CMD_SET_KEEPALIVE:
		__sacc_set_keepalive_sac(priv_p, msg_buf, (struct umessage_sac_set_keepalive *)msghdr);
		break;

	case UMSG_SAC_CMD_FAST_RELEASE:
		__sacc_fast_release(priv_p, msg_buf, (struct umessage_sac_fast_release *)msghdr);
		break;

	case UMSG_SAC_CMD_IOINFO:
		__sacc_ioinfo(priv_p, msg_buf, (struct umessage_sac_ioinfo *)msghdr);
		break;

	case UMSG_SAC_CMD_DISPLAY:
		__sacc_display(priv_p, msg_buf, (struct umessage_sac_display *)msghdr);
		break;

	case UMSG_SAC_CMD_HELLO:
		__sacc_hello(priv_p, msg_buf, (struct umessage_sac_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
	}
}

static void
sacc_cleanup(struct sacc_interface *sacc_p)
{
	nid_log_debug("sacc_cleanup start, sacc_p:%p", sacc_p);
	if (sacc_p->sacc_private != NULL) {
		free(sacc_p->sacc_private);
		sacc_p->sacc_private = NULL;
	}
}

struct sacc_operations sacc_op = {
	.sacc_accept_new_channel = sacc_accept_new_channel,
	.sacc_do_channel = sacc_do_channel,
	.sacc_cleanup = sacc_cleanup,
};

int
sacc_initialization(struct sacc_interface *sacc_p, struct sacc_setup *setup)
{
	char *log_header = "sacc_initialization";
	struct sacc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	sacc_p->sacc_private = priv_p;
	sacc_p->sacc_op = &sacc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_sacg = setup->sacg;
	return 0;
}
