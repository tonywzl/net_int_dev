/*
 * bwcc.c
 * 	Implementation of Memoey Write Cache (bwc) Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "util_nw.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_bwc_if.h"
#include "bwc_if.h"
#include "bwcc_if.h"
#include "bwcg_if.h"
#include "wc_if.h"
#include "io_if.h"

struct bwcc_private {
	struct bwcg_interface	*p_bwcg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
bwcc_accept_new_channel(struct bwcc_interface *bwcc_p, int sfd)
{
	struct bwcc_private *priv_p = bwcc_p->w_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__bwcc_admin_snapshot(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_snapshot *snapshot_msg)
{
	char *log_header = "__bwcc_admin_snapshot";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)snapshot_msg;
	int ctype = NID_CTYPE_BWC, rc;
	struct umessage_bwc_snapshot_resp snapshot_resp;

	assert(msghdr->um_req == UMSG_BWC_CMD_SNAPSHOT);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)snapshot_msg);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_1:
		nid_log_debug("%s: got UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_1, bwc_uuid:%s, chan_uuid:%s",
				log_header, snapshot_msg->um_bwc_uuid, snapshot_msg->um_chan_uuid);
		rc = bwcg_p->wg_op->wg_freeze_snapshot_stage1(bwcg_p, snapshot_msg->um_bwc_uuid, snapshot_msg->um_chan_uuid);
		break;

	case UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_2:
		nid_log_debug("%s: got UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_2, bwc_uuid:%s, chan_uuid:%s",
				log_header, snapshot_msg->um_bwc_uuid, snapshot_msg->um_chan_uuid);
		rc = bwcg_p->wg_op->wg_freeze_snapshot_stage2(bwcg_p, snapshot_msg->um_bwc_uuid, snapshot_msg->um_chan_uuid);
		break;

	case UMSG_BWC_CODE_SNAPSHOT_UNFREEZE:
		nid_log_debug("%s: got UMSG_BWC_CODE_SNAPSHOT_UNFREEZE, bwc_uuid:%s, chan_uuid:%s",
		log_header, snapshot_msg->um_bwc_uuid, snapshot_msg->um_chan_uuid);
		rc = bwcg_p->wg_op->wg_unfreeze_snapshot(bwcg_p, snapshot_msg->um_bwc_uuid, snapshot_msg->um_chan_uuid);
		break;

	default:
		rc = -1;
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

	snapshot_resp.um_header.um_req = msghdr->um_req;
	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_1:
	case UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_2:

		snapshot_resp.um_header.um_req_code = UMSG_BWC_CODE_SNAPSHOT_FREEZE_RESP;
		break;

	case UMSG_BWC_CODE_SNAPSHOT_UNFREEZE:
		snapshot_resp.um_header.um_req_code = UMSG_BWC_CODE_SNAPSHOT_UNFREEZE_RESP;
		break;
	}
	snapshot_resp.um_bwc_uuid_len = snapshot_msg->um_bwc_uuid_len;
	memcpy(snapshot_resp.um_bwc_uuid, snapshot_msg->um_bwc_uuid, snapshot_resp.um_bwc_uuid_len);
	snapshot_resp.um_chan_uuid_len = snapshot_msg->um_chan_uuid_len;
	memcpy(snapshot_resp.um_chan_uuid, snapshot_msg->um_chan_uuid, snapshot_resp.um_chan_uuid_len);
	snapshot_resp.um_resp_code = rc;
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, &snapshot_resp);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);

out:
	close(sfd);
}


static void
__bwcc_dropcache(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_dropcache *dc_msg)
{
	char *log_header = "__bwcc_dropcache";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)dc_msg;
	int ctype = NID_CTYPE_BWC, rc;

	assert(msghdr->um_req == UMSG_BWC_CMD_DROPCACHE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)dc_msg);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_START:
		nid_log_debug("%s: got UMSG_BWC_CODE_START, uuid:%s",  log_header, dc_msg->um_chan_uuid);
		bwcg_p->wg_op->wg_dropcache_start(bwcg_p, dc_msg->um_bwc_uuid, dc_msg->um_chan_uuid, 0);
		break;

	case UMSG_BWC_CODE_START_SYNC:
		nid_log_debug("%s: got UMSG_BWC_CODE_START_SYNC, uuid:%s",  log_header, dc_msg->um_chan_uuid);
		bwcg_p->wg_op->wg_dropcache_start(bwcg_p, dc_msg->um_bwc_uuid, dc_msg->um_chan_uuid, 1);
		break;

	case UMSG_BWC_CODE_STOP:
		nid_log_debug("%s: got UMSG_BWC_CODE_STOP, uuid:%s",  log_header, dc_msg->um_chan_uuid);
		bwcg_p->wg_op->wg_dropcache_stop(bwcg_p, dc_msg->um_bwc_uuid, dc_msg->um_chan_uuid);
		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

out:
	close(sfd);
}

static void
__bwcc_information(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_information *info_msg)
{
	char *log_header = "__bwcc_information";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr;
	int ctype = NID_CTYPE_BWC, rc;
	char nothing_back;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)info_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_INFORMATION);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, info_msg->um_bwc_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_FLUSHING:
		nid_log_warning("%s: got UMSG_BWC_CODE_FLUSHING, uuid:%s",  log_header, info_msg->um_bwc_uuid);
		break;

	case UMSG_BWC_CODE_STAT: {
		struct umessage_bwc_information_resp_stat info_stat;
		memset(&info_stat, 0 , sizeof(info_stat));
		nid_log_warning("%s: got UMSG_BWC_CODE_STAT, uuid:%s",  log_header, info_msg->um_bwc_uuid);
		if (bwcg_p) {
			bwcg_p->wg_op->wg_get_info_stat(bwcg_p, info_msg->um_bwc_uuid, &info_stat);
			info_stat.um_bwc_uuid_len  = info_msg->um_bwc_uuid_len;
			memcpy(info_stat.um_bwc_uuid, info_msg->um_bwc_uuid, info_msg->um_bwc_uuid_len);
			msghdr = (struct umessage_bwc_hdr *)&info_stat;
			msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_BWC_CODE_RESP_STAT;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			nid_log_warning("%s: seq_assigned:%lu, seq_flushed:%lu, char2:%d char3:%d char4:%d char5:%d",
				log_header, info_stat.um_seq_assigned, info_stat.um_seq_flushed,
				msg_buf[2], msg_buf[3], msg_buf[4], msg_buf[5]);
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;
	}

	case UMSG_BWC_CODE_LIST_STAT: {
		struct umessage_bwc_information_list_resp_stat info_stat;
		memset(&info_stat, 0 , sizeof(info_stat));
		nid_log_warning("%s: got UMSG_BWC_CODE_LIST_STAT, uuid:%s,",  log_header, info_msg->um_bwc_uuid);
		if (bwcg_p) {
			bwcg_p->wg_op->wg_info_list_stat(bwcg_p, info_msg->um_bwc_uuid, info_msg->um_chan_uuid, &info_stat);
			info_stat.um_bwc_uuid_len  = info_msg->um_bwc_uuid_len;
			memcpy(info_stat.um_bwc_uuid, info_msg->um_bwc_uuid, info_msg->um_bwc_uuid_len);
			info_stat.um_chan_uuid_len  = info_msg->um_chan_uuid_len;
			memcpy(info_stat.um_chan_uuid, info_msg->um_chan_uuid, info_msg->um_chan_uuid_len);
			msghdr = (struct umessage_bwc_hdr *)&info_stat;
			msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_BWC_CODE_LIST_RESP_STAT;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
		//	nid_log_warning("%s: seq_assigned:%lu, seq_flushed:%lu, char2:%d char3:%d char4:%d char5:%d",
	//			log_header, info_stat.um_seq_assigned, info_stat.um_seq_flushed,
		//		msg_buf[2], msg_buf[3], msg_buf[4], msg_buf[5]);
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;
	}

	case UMSG_BWC_CODE_THROUGHPUT_STAT: {
		struct umessage_bwc_information_resp_throughput_stat info_stat;
		nid_log_warning("%s: got UMSG_BWC_CODE_THROUGHPUT_STAT, uuid:%s",  log_header, info_msg->um_bwc_uuid);
		if (bwcg_p) {
			struct bwc_throughput_stat info;
			bwcg_p->wg_op->wg_get_throughput(bwcg_p, info_msg->um_bwc_uuid, &info);
			info_stat.um_bwc_uuid_len  = info_msg->um_bwc_uuid_len;
			memcpy(info_stat.um_bwc_uuid, info_msg->um_bwc_uuid, info_msg->um_bwc_uuid_len);
			msghdr = (struct umessage_bwc_hdr *)&info_stat;
			msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_BWC_CODE_THROUGHPUT_STAT_RSLT;
			info_stat.um_seq_data_flushed = info.data_flushed;
			info_stat.um_seq_data_pkg_flushed = info.data_pkg_flushed;
			info_stat.um_seq_nondata_pkg_flushed = info.nondata_pkg_flushed;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			nid_log_warning("%s: data_flushed:%lu, data_pkg_flushed:%lu, nondata_pkg_flushed:%lu, char2:%d char3:%d char4:%d char5:%d",
				log_header, info_stat.um_seq_data_flushed, info_stat.um_seq_data_pkg_flushed, info_stat.um_seq_nondata_pkg_flushed,
				msg_buf[2], msg_buf[3], msg_buf[4], msg_buf[5]);
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;
	}

	case UMSG_BWC_CODE_RW_STAT: {
		struct umessage_bwc_information_resp_rw_stat info_stat;
		nid_log_warning("%s: got UMSG_BWC_CODE_RW_STAT, uuid:%s",  log_header, info_msg->um_bwc_uuid);
		if (bwcg_p) {
			struct bwc_rw_stat info;
			bwcg_p->wg_op->wg_get_rw(bwcg_p, info_msg->um_bwc_uuid, info_msg->um_chan_uuid, &info);
			info_stat.um_bwc_uuid_len  = info_msg->um_bwc_uuid_len;
			memcpy(info_stat.um_bwc_uuid, info_msg->um_bwc_uuid, info_msg->um_bwc_uuid_len);
			info_stat.um_chan_uuid_len  = info_msg->um_chan_uuid_len;
			memcpy(info_stat.um_chan_uuid, info_msg->um_chan_uuid, info_msg->um_chan_uuid_len);
			msghdr = (struct umessage_bwc_hdr *)&info_stat;
			msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_BWC_CODE_RW_STAT_RSLT;
			info_stat.res = info.res;
			if (info_stat.res){
				info_stat.um_seq_overwritten_counter= info.overwritten_num;
				info_stat.um_seq_overwritten_back_counter = info.overwritten_back_num;
				info_stat.um_seq_coalesce_flush_num = info.coalesce_num;
				info_stat.um_seq_coalesce_flush_back_num = info.coalesce_back_num;
				info_stat.um_seq_flush_num = info.flush_num;
				info_stat.um_seq_flush_back_num = info.flush_back_num;
				info_stat.um_seq_flush_page_num = info.flush_page;
				info_stat.um_seq_not_ready_num = info.not_ready_num;
			} else{
				info_stat.um_seq_overwritten_counter= 0;
				info_stat.um_seq_overwritten_back_counter = 0;
				info_stat.um_seq_coalesce_flush_num = 0;
				info_stat.um_seq_coalesce_flush_back_num = 0;
				info_stat.um_seq_flush_num = 0;
				info_stat.um_seq_flush_back_num = 0;
				info_stat.um_seq_flush_page_num = 0;
				info_stat.um_seq_not_ready_num = 0;
			}

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			nid_log_warning("%s: overwritten:%lu, overwritten_back:%lu, coalesce:%lu, coalesce_back:%lu, flush:%lu, flush_back:%lu, char2:%d char3:%d char4:%d char5:%d",
				log_header, info_stat.um_seq_overwritten_counter, info_stat.um_seq_overwritten_back_counter, info_stat.um_seq_coalesce_flush_num, info_stat.um_seq_coalesce_flush_back_num,
				info_stat.um_seq_flush_num, info_stat.um_seq_flush_back_num,
				msg_buf[2], msg_buf[3], msg_buf[4], msg_buf[5]);

			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;
	}

	case UMSG_BWC_CODE_DELAY_STAT: {
		struct umessage_bwc_information_resp_delay_stat info_stat;
		memset(&info_stat, 0 , sizeof(info_stat));
		nid_log_warning("%s: got UMSG_BWC_CODE_DELAY_STAT, uuid:%s",  log_header, info_msg->um_bwc_uuid);
		if (bwcg_p) {
			struct bwc_delay_stat info;
			memset(&info, 0, sizeof(info));
			bwcg_p->wg_op->wg_get_delay(bwcg_p, info_msg->um_bwc_uuid, &info);
			info_stat.um_bwc_uuid_len  = info_msg->um_bwc_uuid_len;
			memcpy(info_stat.um_bwc_uuid, info_msg->um_bwc_uuid, info_msg->um_bwc_uuid_len);

			info_stat.um_write_delay_first_level = info.write_delay_first_level;
			info_stat.um_write_delay_second_level = info.write_delay_second_level;
			info_stat.um_write_delay_first_level_max = info.write_delay_first_level_max_us;
			info_stat.um_write_delay_second_level_max = info.write_delay_second_level_max_us;
			info_stat.um_write_delay_first_level_increase_interval = info.write_delay_first_level_increase_interval;
			info_stat.um_write_delay_second_level_increase_interval = info.write_delay_second_level_increase_interval;
			info_stat.um_write_delay_second_level_start = info.write_delay_second_level_start_ms;
			info_stat.um_write_delay_time = info.write_delay_time;

			msghdr = (struct umessage_bwc_hdr *)&info_stat;
			msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
			msghdr->um_req_code = UMSG_BWC_CODE_RESP_DELAY_STAT;
			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
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
__bwcc_throughput(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_throughput *tpt_msg)
{
	char *log_header = "__bwcc_throughput";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr;
	int ctype = NID_CTYPE_BWC, rc;
	//char nothing_back;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)tpt_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_THROUGHPUT);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, tpt_msg->um_bwc_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_THROUGHPUT_RESET: {
		nid_log_warning("%s: got UMSG_BWC_CODE_THROUGHPUT_RESET, uuid:%s",
			log_header, tpt_msg->um_bwc_uuid);
		if (bwcg_p) {
			bwcg_p->wg_op->wg_reset_throughput(bwcg_p, tpt_msg->um_bwc_uuid);
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
__bwcc_update_water_mark(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_water_mark * wm_msg)
{
	char *log_header = "__bwcc_update_water_mark";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	char nothing_back;
	struct umessage_bwc_hdr *msghdr;
	struct umessage_bwc_water_mark_resp wm_resp;
	int ctype = NID_CTYPE_BWC, rc, rc1 = 0;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)wm_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_UPDATE_WATER_MARK);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, wm_msg->um_bwc_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_UPDATE_WATER_MARK:
		nid_log_warning("%s: got UMSG_BWC_CODE_UPDATE_WATER_MARK, uuid:%s",
			log_header, wm_msg->um_bwc_uuid);
		rc1 = bwcg_p->wg_op->wg_update_water_mark(bwcg_p, wm_msg->um_bwc_uuid, wm_msg->um_high_water_mark, wm_msg->um_low_water_mark);

		msghdr = (struct umessage_bwc_hdr *)&wm_resp;
		msghdr->um_req = UMSG_BWC_CMD_UPDATE_WATER_MARK;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_UPDATE_WATER_MARK;
		memcpy(wm_resp.um_bwc_uuid, wm_msg->um_bwc_uuid, wm_msg->um_bwc_uuid_len);
		wm_resp.um_bwc_uuid_len = wm_msg->um_bwc_uuid_len;

		wm_resp.um_resp_code = rc1,

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
__bwcc_add(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_add *add_msg)
{
	char *log_header = "__bwcc_add";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
//	struct bwc_interface *bwc_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr;
	int ctype = NID_CTYPE_BWC, rc;
	struct umessage_bwc_add_resp add_resp;
	char nothing_back;
	char *bwc_uuid, *bufdevice, *tp_name;
	int bufdevicesz, rw_sync, two_step_read, do_fp, bfp_type, max_flush_size, ssd_mode, write_delay_first_level, write_delay_second_level, high_water_mark, low_water_mark;
	double coalesce_ratio, load_ratio_max, load_ratio_min, load_ctrl_level, flush_delay_ctl, throttle_ratio;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)add_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_ADD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, add_msg->um_bwc_uuid);
	bwc_uuid = add_msg->um_bwc_uuid;
	bufdevice = add_msg->um_bufdev;
	bufdevicesz = (int)add_msg->um_bufdev_sz;
	rw_sync = (int)add_msg->um_rw_sync;
	two_step_read = (int)add_msg->um_two_step_read;
	do_fp = (int)add_msg->um_do_fp;
	bfp_type = (int)add_msg->um_bfp_type;
	coalesce_ratio = add_msg->um_coalesce_ratio;
	load_ratio_max = add_msg->um_load_ratio_max;
	load_ratio_min = add_msg->um_load_ratio_min;
	load_ctrl_level = add_msg->um_load_ctrl_level;
	flush_delay_ctl = add_msg->um_flush_delay_ctl;
	throttle_ratio = add_msg->um_throttle_ratio;
	tp_name = add_msg->um_tp_name;
	max_flush_size = add_msg->um_max_flush_size;
	ssd_mode = add_msg->um_ssd_mode;
	write_delay_first_level = add_msg->um_write_delay_first_level;
	write_delay_second_level = add_msg->um_write_delay_second_level;
	high_water_mark = add_msg->um_high_water_mark;
	low_water_mark = add_msg->um_low_water_mark;

	msghdr = (struct umessage_bwc_hdr *)&add_resp;
	msghdr->um_req = UMSG_BWC_CMD_ADD;
	msghdr->um_req_code = UMSG_BWC_CODE_RESP_ADD;
	memcpy(add_resp.um_bwc_uuid, add_msg->um_bwc_uuid, add_msg->um_bwc_uuid_len);
	add_resp.um_bwc_uuid_len  = add_msg->um_bwc_uuid_len;

	add_resp.um_resp_code = bwcg_p->wg_op->wg_add_bwc(bwcg_p, bwc_uuid, bufdevice, bufdevicesz,
			rw_sync, two_step_read, do_fp, bfp_type, coalesce_ratio, load_ratio_max,
			load_ratio_min, load_ctrl_level, flush_delay_ctl, throttle_ratio, tp_name,
			max_flush_size, ssd_mode, write_delay_first_level, write_delay_second_level,
			high_water_mark, low_water_mark);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__bwcc_remove(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_remove *rm_msg)
{
	char *log_header = "__bwcc_remove";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
//	struct bwc_interface *bwc_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr;
	int ctype = NID_CTYPE_BWC, rc;
	struct umessage_bwc_remove_resp rm_resp;
	char nothing_back;
	char *bwc_uuid;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)rm_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_REMOVE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, rm_msg->um_bwc_uuid);
	bwc_uuid = rm_msg->um_bwc_uuid;
//	bwc_p = bwccg_p->wcg_op->wcg_get_bwc(bwccg_p, bwc_uuid);

	msghdr = (struct umessage_bwc_hdr *)&rm_resp;
	msghdr->um_req = UMSG_BWC_CMD_REMOVE;
	msghdr->um_req_code = UMSG_BWC_CODE_RESP_REMOVE;
	memcpy(rm_resp.um_bwc_uuid, rm_msg->um_bwc_uuid, rm_msg->um_bwc_uuid_len);
	rm_resp.um_bwc_uuid_len  = rm_msg->um_bwc_uuid_len;

	rm_resp.um_resp_code = bwcg_p->wg_op->wg_remove_bwc(bwcg_p, bwc_uuid);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__bwcc_fflush(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_fflush *ff_msg)
{
	char *log_header = "__bwcc_fflush";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)ff_msg;
	int ctype = NID_CTYPE_BWC, rc;
	char nothing_back;

	assert(msghdr->um_req == UMSG_BWC_CMD_FFLUSH);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)ff_msg);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_FF_START:
		nid_log_debug("%s: got UMSG_BWC_CODE_FF_START, uuid:%s",  log_header, ff_msg->um_chan_uuid);
		bwcg_p->wg_op->wg_fflush_start(bwcg_p, ff_msg->um_bwc_uuid, ff_msg->um_chan_uuid);
		break;

	case UMSG_BWC_CODE_FF_GET: {
		struct umessage_bwc_fflush_resp_get get_resp;
		int rc2;
		uint8_t ff_state_code;

		nid_log_debug("%s: got UMSG_BWC_CODE_FF_GET, uuid:%s",  log_header, ff_msg->um_chan_uuid);
		rc2 = bwcg_p->wg_op->wg_fflush_get(bwcg_p, ff_msg->um_bwc_uuid, ff_msg->um_chan_uuid);
		ff_state_code = rc2 >= 0 ? (uint8_t)rc2 : 0;
		get_resp.um_bwc_uuid_len = ff_msg->um_bwc_uuid_len;
		memcpy(get_resp.um_bwc_uuid, ff_msg->um_bwc_uuid, ff_msg->um_bwc_uuid_len);
		get_resp.um_chan_uuid_len = ff_msg->um_chan_uuid_len;
		memcpy(get_resp.um_chan_uuid, ff_msg->um_chan_uuid, ff_msg->um_chan_uuid_len);
		msghdr = (struct umessage_bwc_hdr *)&get_resp;
		msghdr->um_req = UMSG_BWC_CMD_FFLUSH;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_FF_GET;
		get_resp.um_ff_state = ff_state_code;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		nid_log_warning("%s: fast flush state:%u", log_header, ff_state_code);
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);
		break;
	}

	case UMSG_BWC_CODE_FF_STOP:
		nid_log_debug("%s: got UMSG_BWC_CODE_STOP, uuid:%s",  log_header, ff_msg->um_chan_uuid);
		bwcg_p->wg_op->wg_fflush_stop(bwcg_p, ff_msg->um_bwc_uuid, ff_msg->um_chan_uuid);
		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

out:
	close(sfd);
}

static void
__bwcc_recover(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_recover *rcv_msg)
{
	char *log_header = "__bwcc_recover";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)rcv_msg;
	int ctype = NID_CTYPE_BWC, rc;
	char nothing_back;

	assert(msghdr->um_req == UMSG_BWC_CMD_RECOVER);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)rcv_msg);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_RCV_START:
		nid_log_debug("%s: got UMSG_BWC_CODE_RCV_START, uuid:%s",  log_header, rcv_msg->um_bwc_uuid);
		bwcg_p->wg_op->wg_recover_bwc(bwcg_p, rcv_msg->um_bwc_uuid);
		break;

	case UMSG_BWC_CODE_RCV_GET: {
		struct umessage_bwc_recover_resp_get get_resp;
		int rc2;
		uint8_t rcv_state_code;

		nid_log_debug("%s: got UMSG_BWC_CODE_RCV_GET, uuid:%s",  log_header, rcv_msg->um_bwc_uuid);
		rc2 = bwcg_p->wg_op->wg_get_recover_bwc(bwcg_p, rcv_msg->um_bwc_uuid);
		rcv_state_code = rc2 >= 0 ? (uint8_t)rc2 : 0;
		get_resp.um_bwc_uuid_len = rcv_msg->um_bwc_uuid_len;
		memcpy(get_resp.um_bwc_uuid, rcv_msg->um_bwc_uuid, rcv_msg->um_bwc_uuid_len);
		msghdr = (struct umessage_bwc_hdr *)&get_resp;
		msghdr->um_req = UMSG_BWC_CMD_RECOVER;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_RCV_GET;
		get_resp.um_rcv_state = rcv_state_code;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		nid_log_warning("%s: recovery state:%u", log_header, rcv_state_code);
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
__bwcc_ioinfo(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_ioinfo *c_msg)
{
	char *log_header = "__bwcc_ioinfo";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p, **all_bwc = NULL;
	struct bwc_req_stat stat_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)c_msg;
	struct umessage_bwc_ioinfo_resp msg_resp;
	struct io_vec_stat vstat, *vstat_p, *all_vstat = NULL;
	int ctype = NID_CTYPE_BWC, rc;
	int num_bwc, num_bfe, i, j;
	char nothing_back, *bwc_uuid;

	assert(msghdr->um_req == UMSG_BWC_CMD_IOINFO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)c_msg);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_IOINFO_START:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_p->bw_op->bw_start_ioinfo(bwc_p);
		bwc_p->bw_op->bw_check_ioinfo(bwc_p, &stat_p);

		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_START;

		msg_resp.um_bwc_uuid_len = c_msg->um_bwc_uuid_len;
		memcpy(msg_resp.um_bwc_uuid, c_msg->um_bwc_uuid, c_msg->um_bwc_uuid_len);
		msg_resp.um_is_running = stat_p.is_running;
		msg_resp.um_req_num = stat_p.req_num;
		msg_resp.um_req_len = stat_p.req_len;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_STOP:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_p->bw_op->bw_stop_ioinfo(bwc_p);
		bwc_p->bw_op->bw_check_ioinfo(bwc_p, &stat_p);

		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_STOP;

		msg_resp.um_bwc_uuid_len = c_msg->um_bwc_uuid_len;
		memcpy(msg_resp.um_bwc_uuid, c_msg->um_bwc_uuid, c_msg->um_bwc_uuid_len);
		msg_resp.um_is_running = stat_p.is_running;
		msg_resp.um_req_num = stat_p.req_num;
		msg_resp.um_req_len = stat_p.req_len;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_CHECK:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_p->bw_op->bw_check_ioinfo(bwc_p, &stat_p);

		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_CHECK;

		msg_resp.um_bwc_uuid_len = c_msg->um_bwc_uuid_len;
		memcpy(msg_resp.um_bwc_uuid, c_msg->um_bwc_uuid, c_msg->um_bwc_uuid_len);
 		msg_resp.um_is_running = stat_p.is_running;
 		msg_resp.um_req_num = stat_p.req_num;
 		msg_resp.um_req_len = stat_p.req_len;

 		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_START_ALL:
		all_bwc = bwcg_p->wg_op->wg_get_all_bwc(bwcg_p, &num_bwc);
		for (i = 0; i < num_bwc; i++) {
			bwc_p = all_bwc[i];
			bwc_p->bw_op->bw_start_ioinfo(bwc_p);
			bwc_p->bw_op->bw_check_ioinfo(bwc_p, &stat_p);

			msghdr = (struct umessage_bwc_hdr *)&msg_resp;
			msghdr->um_req = UMSG_BWC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_START_ALL;

			bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
			msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
			memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
			msg_resp.um_is_running = stat_p.is_running;
			msg_resp.um_req_num = stat_p.req_num;
			msg_resp.um_req_len = stat_p.req_len;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_STOP_ALL:
		all_bwc = bwcg_p->wg_op->wg_get_all_bwc(bwcg_p, &num_bwc);
		for (i = 0; i < num_bwc; i++) {
			bwc_p = all_bwc[i];
			bwc_p->bw_op->bw_stop_ioinfo(bwc_p);
			bwc_p->bw_op->bw_check_ioinfo(bwc_p, &stat_p);

			msghdr = (struct umessage_bwc_hdr *)&msg_resp;
			msghdr->um_req = UMSG_BWC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_STOP_ALL;

			bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
			msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
			memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
			msg_resp.um_is_running = stat_p.is_running;
			msg_resp.um_req_num = stat_p.req_num;
			msg_resp.um_req_len = stat_p.req_len;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_CHECK_ALL:
		all_bwc = bwcg_p->wg_op->wg_get_all_bwc(bwcg_p, &num_bwc);
		for (i = 0; i < num_bwc; i++) {
			bwc_p = all_bwc[i];
			bwc_p->bw_op->bw_check_ioinfo(bwc_p, &stat_p);

			msghdr = (struct umessage_bwc_hdr *)&msg_resp;
			msghdr->um_req = UMSG_BWC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_CHECK_ALL;

			bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
			msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
			memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
 			msg_resp.um_is_running = stat_p.is_running;
 			msg_resp.um_req_num = stat_p.req_num;
 			msg_resp.um_req_len = stat_p.req_len;

 			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;


	case UMSG_BWC_CODE_IOINFO_BFE_START:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_p->bw_op->bw_vec_start(bwc_p, c_msg->um_chan_uuid);
		bwc_p->bw_op->bw_get_vec_stat_by_uuid(bwc_p, c_msg->um_chan_uuid, &vstat);

		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_START;

		msg_resp.um_bwc_uuid_len = c_msg->um_bwc_uuid_len;
		memcpy(msg_resp.um_bwc_uuid, c_msg->um_bwc_uuid, c_msg->um_bwc_uuid_len);
		msg_resp.um_chan_uuid_len = c_msg->um_chan_uuid_len;
		memcpy(msg_resp.um_chan_uuid, c_msg->um_chan_uuid, c_msg->um_chan_uuid_len);
		msg_resp.um_is_running = vstat.s_stat;
		msg_resp.um_req_num = vstat.s_flush_num;
		msg_resp.um_req_len = vstat.s_write_size;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
				goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_BFE_STOP:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_p->bw_op->bw_vec_stop(bwc_p, c_msg->um_chan_uuid);
		bwc_p->bw_op->bw_get_vec_stat_by_uuid(bwc_p, c_msg->um_chan_uuid, &vstat);

		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP;

		msg_resp.um_bwc_uuid_len = c_msg->um_bwc_uuid_len;
		memcpy(msg_resp.um_bwc_uuid, c_msg->um_bwc_uuid, c_msg->um_bwc_uuid_len);
		msg_resp.um_chan_uuid_len = c_msg->um_chan_uuid_len;
		memcpy(msg_resp.um_chan_uuid, c_msg->um_chan_uuid, c_msg->um_chan_uuid_len);
		msg_resp.um_is_running = vstat.s_stat;
		msg_resp.um_req_num = vstat.s_flush_num;
		msg_resp.um_req_len = vstat.s_write_size;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_BFE_CHECK:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_p->bw_op->bw_get_vec_stat_by_uuid(bwc_p, c_msg->um_chan_uuid, &vstat);

		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK;

		msg_resp.um_bwc_uuid_len = c_msg->um_bwc_uuid_len;
		memcpy(msg_resp.um_bwc_uuid, c_msg->um_bwc_uuid, c_msg->um_bwc_uuid_len);
		msg_resp.um_chan_uuid_len = c_msg->um_chan_uuid_len;
		memcpy(msg_resp.um_chan_uuid, c_msg->um_chan_uuid, c_msg->um_chan_uuid_len);
		msg_resp.um_is_running = vstat.s_stat;
		msg_resp.um_req_num = vstat.s_flush_num;
		msg_resp.um_req_len = vstat.s_write_size;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_START_ALL:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
		bwc_p->bw_op->bw_vec_start(bwc_p, NULL);
		all_vstat = (struct io_vec_stat *)bwc_p->bw_op->bw_get_all_vec_stat(bwc_p, &num_bfe);
		for (i = 0; i < num_bfe; i++) {
			msghdr = (struct umessage_bwc_hdr *)&msg_resp;
			msghdr->um_req = UMSG_BWC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_START_ALL;

			vstat_p = &all_vstat[i];
			msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
			memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
			msg_resp.um_chan_uuid_len = strlen(vstat_p->s_chan_uuid);
			memcpy(msg_resp.um_chan_uuid, vstat_p->s_chan_uuid, msg_resp.um_chan_uuid_len);
			msg_resp.um_is_running = vstat_p->s_stat;
			msg_resp.um_req_num = vstat_p->s_flush_num;
			msg_resp.um_req_len = vstat_p->s_write_size;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_STOP_ALL:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
		bwc_p->bw_op->bw_vec_stop(bwc_p, NULL);
		all_vstat = (struct io_vec_stat *)bwc_p->bw_op->bw_get_all_vec_stat(bwc_p, &num_bfe);
		for (i = 0; i < num_bfe; i++) {
			msghdr = (struct umessage_bwc_hdr *)&msg_resp;
			msghdr->um_req = UMSG_BWC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_STOP_ALL;

			vstat_p = &all_vstat[i];
			msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
			memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
			msg_resp.um_chan_uuid_len = strlen(vstat_p->s_chan_uuid);
			memcpy(msg_resp.um_chan_uuid, vstat_p->s_chan_uuid, msg_resp.um_chan_uuid_len);
			msg_resp.um_is_running = vstat_p->s_stat;
			msg_resp.um_req_num = vstat_p->s_flush_num;
			msg_resp.um_req_len = vstat_p->s_write_size;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_CHECK_ALL:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, c_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, c_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
		all_vstat = (struct io_vec_stat *)bwc_p->bw_op->bw_get_all_vec_stat(bwc_p, &num_bfe);
		for (i = 0; i < num_bfe; i++) {
			msghdr = (struct umessage_bwc_hdr *)&msg_resp;
			msghdr->um_req = UMSG_BWC_CMD_IOINFO;
			msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_CHECK_ALL;

			vstat_p = &all_vstat[i];
			msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
			memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
			msg_resp.um_chan_uuid_len = strlen(vstat_p->s_chan_uuid);
			memcpy(msg_resp.um_chan_uuid, vstat_p->s_chan_uuid, msg_resp.um_chan_uuid_len);
			msg_resp.um_is_running = vstat_p->s_stat;
			msg_resp.um_req_num = vstat_p->s_flush_num;
			msg_resp.um_req_len = vstat_p->s_write_size;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc)
				goto out;
			write(sfd, msg_buf, cmd_len);
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_BFE_START_ALL:
		all_bwc = bwcg_p->wg_op->wg_get_all_bwc(bwcg_p, &num_bwc);
		for (i = 0; i < num_bwc; i++) {
			bwc_p = all_bwc[i];
			bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
			bwc_p->bw_op->bw_vec_start(bwc_p, NULL);
			all_vstat = (struct io_vec_stat *)bwc_p->bw_op->bw_get_all_vec_stat(bwc_p, &num_bfe);
			for (j = 0; j < num_bfe; j++) {
				msghdr = (struct umessage_bwc_hdr *)&msg_resp;
				msghdr->um_req = UMSG_BWC_CMD_IOINFO;
				msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_START_ALL;

				vstat_p = &all_vstat[j];
				msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
				memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
				msg_resp.um_chan_uuid_len = strlen(vstat_p->s_chan_uuid);
				memcpy(msg_resp.um_chan_uuid, vstat_p->s_chan_uuid, msg_resp.um_chan_uuid_len);
				msg_resp.um_is_running = vstat_p->s_stat;
				msg_resp.um_req_num = vstat_p->s_flush_num;
				msg_resp.um_req_len = vstat_p->s_write_size;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc)
					goto out;
				write(sfd, msg_buf, cmd_len);
			}
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_BFE_STOP_ALL:
		all_bwc = bwcg_p->wg_op->wg_get_all_bwc(bwcg_p, &num_bwc);
		for (i = 0; i < num_bwc; i++) {
			bwc_p = all_bwc[i];
			bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
			bwc_p->bw_op->bw_vec_stop(bwc_p, NULL);
			all_vstat = (struct io_vec_stat *)bwc_p->bw_op->bw_get_all_vec_stat(bwc_p, &num_bfe);
			for (j = 0; j < num_bfe; j++) {
				msghdr = (struct umessage_bwc_hdr *)&msg_resp;
				msghdr->um_req = UMSG_BWC_CMD_IOINFO;
				msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP_ALL;

				vstat_p = &all_vstat[j];
				msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
				memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
				msg_resp.um_chan_uuid_len = strlen(vstat_p->s_chan_uuid);
				memcpy(msg_resp.um_chan_uuid, vstat_p->s_chan_uuid, msg_resp.um_chan_uuid_len);
				msg_resp.um_is_running = vstat_p->s_stat;
				msg_resp.um_req_num = vstat_p->s_flush_num;
				msg_resp.um_req_len = vstat_p->s_write_size;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc)
					goto out;
				write(sfd, msg_buf, cmd_len);
			}
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_IOINFO_BFE_CHECK_ALL:
		all_bwc = bwcg_p->wg_op->wg_get_all_bwc(bwcg_p, &num_bwc);
		for (i = 0; i < num_bwc; i++) {
			bwc_p = all_bwc[i];
			bwc_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
			all_vstat = (struct io_vec_stat *)bwc_p->bw_op->bw_get_all_vec_stat(bwc_p, &num_bfe);
			for (j = 0; j < num_bfe; j++) {
				msghdr = (struct umessage_bwc_hdr *)&msg_resp;
				msghdr->um_req = UMSG_BWC_CMD_IOINFO;
				msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK_ALL;

				vstat_p = &all_vstat[j];
				msg_resp.um_bwc_uuid_len = strlen(bwc_uuid);
				memcpy(msg_resp.um_bwc_uuid, bwc_uuid, msg_resp.um_bwc_uuid_len);
				msg_resp.um_chan_uuid_len = strlen(vstat_p->s_chan_uuid);
				memcpy(msg_resp.um_chan_uuid, vstat_p->s_chan_uuid, msg_resp.um_chan_uuid_len);
				msg_resp.um_is_running = vstat_p->s_stat;
				msg_resp.um_req_num = vstat_p->s_flush_num;
				msg_resp.um_req_len = vstat_p->s_write_size;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc)
					goto out;
				write(sfd, msg_buf, cmd_len);
			}
		}
		memset(&msg_resp, 0, sizeof(msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&msg_resp;
		msghdr->um_req = UMSG_BWC_CMD_IOINFO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_IOINFO_END;

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
	free(all_vstat);
	free(all_bwc);
	close(sfd);
}

static void
__bwcc_update_delay(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_delay * ud_msg)
{
	char *log_header = "__bwcc_update_delay";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	char nothing_back;
	struct umessage_bwc_hdr *msghdr;
	struct umessage_bwc_delay_resp wm_resp;
	struct bwc_delay_stat delay_stat;
	int ctype = NID_CTYPE_BWC, rc, rc1 = 0;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)ud_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_UPDATE_DELAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, ud_msg->um_bwc_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_UPDATE_DELAY:
		nid_log_warning("%s: got UMSG_BWC_CODE_UPDATE_DELAY, uuid:%s",
			log_header, ud_msg->um_bwc_uuid);

		delay_stat.write_delay_first_level = ud_msg->um_write_delay_first_level;
		delay_stat.write_delay_second_level = ud_msg->um_write_delay_second_level;
		delay_stat.write_delay_first_level_max_us = ud_msg->um_write_delay_first_level_max;
		delay_stat.write_delay_second_level_max_us = ud_msg->um_write_delay_second_level_max;

		rc1 = bwcg_p->wg_op->wg_update_delay_level(bwcg_p, ud_msg->um_bwc_uuid, &delay_stat);

		msghdr = (struct umessage_bwc_hdr *)&wm_resp;
		msghdr->um_req = UMSG_BWC_CMD_UPDATE_DELAY;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_UPDATE_DELAY;
		memcpy(wm_resp.um_bwc_uuid, ud_msg->um_bwc_uuid, ud_msg->um_bwc_uuid_len);
		wm_resp.um_bwc_uuid_len = ud_msg->um_bwc_uuid_len;

		wm_resp.um_resp_code = rc1,

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
__bwcc_flush_empty(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_flush_empty * fe_msg)
{
	char *log_header = "__bwcc_flush_empty";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	char nothing_back;
	struct umessage_bwc_hdr *msghdr;
	struct umessage_bwc_flush_empty_resp fe_resp;
	int ctype = NID_CTYPE_BWC, rc;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)fe_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_FLUSH_EMPTY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);
	if (cmd_len > UMSG_BWC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, c_uuid:%s",
		log_header, cmd_len, msghdr->um_req_code, fe_msg->um_bwc_uuid);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_FLUSH_EMPTY_START:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, fe_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, fe_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_p->bw_op->bw_sync_rec_seq(bwc_p);

		msghdr = (struct umessage_bwc_hdr *)&fe_resp;
		msghdr->um_req = UMSG_BWC_CMD_FLUSH_EMPTY;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_FLUSH_EMPTY_START;
		memcpy(fe_resp.um_bwc_uuid, fe_msg->um_bwc_uuid, fe_msg->um_bwc_uuid_len);
		fe_resp.um_bwc_uuid_len = fe_msg->um_bwc_uuid_len;
		fe_resp.um_resp_code = 0;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc)
			goto out;
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);
		break;

	case UMSG_BWC_CODE_FLUSH_EMPTY_STOP:
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, fe_msg->um_bwc_uuid);
		if (!wc_p) {
			nid_log_warning("%s: did not find bwc %s", log_header, fe_msg->um_bwc_uuid);
			goto out;
		}
		bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
		bwc_p->bw_op->bw_stop_sync_rec_seq(bwc_p);

		msghdr = (struct umessage_bwc_hdr *)&fe_resp;
		msghdr->um_req = UMSG_BWC_CMD_FLUSH_EMPTY;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_FLUSH_EMPTY_STOP;
		memcpy(fe_resp.um_bwc_uuid, fe_msg->um_bwc_uuid, fe_msg->um_bwc_uuid_len);
		fe_resp.um_bwc_uuid_len = fe_msg->um_bwc_uuid_len;
		fe_resp.um_resp_code = 0;

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
__bwcc_display(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_display *dis_msg)
{
	char *log_header = "__bwcc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr;
	int ctype = NID_CTYPE_BWC, rc;
	char nothing_back;
	struct umessage_bwc_display_resp dis_resp;
	struct wc_interface *bwc_p;
	struct bwc_setup *bwc_setup_p;
	int *working_bwc = NULL, cur_index = 0;
	int num_bwc = 0, num_working_bwc = 0, get_it = 0;
	int i;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);

	if (cmd_len > UMSG_BWC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, bwc_uuid: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_bwc_uuid);

	switch (msghdr->um_req_code){
	case UMSG_BWC_CODE_S_DISP:
		if (dis_msg->um_bwc_uuid == NULL)
			goto out;
		bwc_setup_p = bwcg_p->wg_op->wg_get_all_bwc_setup(bwcg_p, &num_bwc);
		if (bwc_setup_p == NULL)
			goto out;
		for (i = 0; i < num_bwc; i++, bwc_setup_p++) {
			if (!strcmp(bwc_setup_p->uuid ,dis_msg->um_bwc_uuid)) {
				get_it = 1;
				break;
			}
		}
		if (get_it) {
			dis_resp.um_bwc_uuid_len = strlen(bwc_setup_p->uuid);
			memcpy(dis_resp.um_bwc_uuid, bwc_setup_p->uuid, dis_resp.um_bwc_uuid_len);
			dis_resp.um_bufdev_len = strlen(bwc_setup_p->bufdevice);
			memcpy(dis_resp.um_bufdev, bwc_setup_p->bufdevice, dis_resp.um_bufdev_len);
			dis_resp.um_tp_name_len = strlen(bwc_setup_p->tp_name);
			memcpy(dis_resp.um_tp_name, bwc_setup_p->tp_name, dis_resp.um_tp_name_len);
			dis_resp.um_bufdev_sz = bwc_setup_p->bufdevicesz;
			dis_resp.um_rw_sync = bwc_setup_p->rw_sync;
			dis_resp.um_two_step_read = bwc_setup_p->two_step_read;
			dis_resp.um_do_fp = bwc_setup_p->do_fp;
			dis_resp.um_bfp_type = bwc_setup_p->bfp_type;
			dis_resp.um_coalesce_ratio = bwc_setup_p->coalesce_ratio;
			dis_resp.um_load_ratio_max = bwc_setup_p->load_ratio_max;
			dis_resp.um_load_ratio_min = bwc_setup_p->load_ratio_min;
			dis_resp.um_load_ctrl_level = bwc_setup_p->load_ctrl_level;
			dis_resp.um_flush_delay_ctl = bwc_setup_p->flush_delay_ctl;
			dis_resp.um_throttle_ratio = bwc_setup_p->throttle_ratio;
			dis_resp.um_ssd_mode = bwc_setup_p->ssd_mode;
			dis_resp.um_max_flush_size = bwc_setup_p->max_flush_size;
			dis_resp.um_write_delay_first_level = bwc_setup_p->write_delay_first_level;
			dis_resp.um_write_delay_second_level = bwc_setup_p->write_delay_second_level;
			dis_resp.um_write_delay_first_level_max = bwc_setup_p->write_delay_first_level_max_us;
			dis_resp.um_write_delay_second_level_max = bwc_setup_p->write_delay_second_level_max_us;
			dis_resp.um_high_water_mark = bwc_setup_p->high_water_mark;
			dis_resp.um_low_water_mark = bwc_setup_p->low_water_mark;

			msghdr = (struct umessage_bwc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_BWC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_BWC_CODE_S_RESP_DISP;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;

	case UMSG_BWC_CODE_S_DISP_ALL:
		bwc_setup_p = bwcg_p->wg_op->wg_get_all_bwc_setup(bwcg_p, &num_bwc);
		for (i = 0; i < num_bwc; i++, bwc_setup_p++) {
			memset(&dis_resp, 0, sizeof(dis_resp));
			dis_resp.um_bwc_uuid_len = strlen(bwc_setup_p->uuid);
			memcpy(dis_resp.um_bwc_uuid, bwc_setup_p->uuid, dis_resp.um_bwc_uuid_len);
			dis_resp.um_bufdev_len = strlen(bwc_setup_p->bufdevice);
			memcpy(dis_resp.um_bufdev, bwc_setup_p->bufdevice, dis_resp.um_bufdev_len);
			dis_resp.um_tp_name_len = strlen(bwc_setup_p->tp_name);
			memcpy(dis_resp.um_tp_name, bwc_setup_p->tp_name, dis_resp.um_tp_name_len);
			dis_resp.um_bufdev_sz = bwc_setup_p->bufdevicesz;
			dis_resp.um_rw_sync = bwc_setup_p->rw_sync;
			dis_resp.um_two_step_read = bwc_setup_p->two_step_read;
			dis_resp.um_do_fp = bwc_setup_p->do_fp;
			dis_resp.um_bfp_type = bwc_setup_p->bfp_type;
			dis_resp.um_coalesce_ratio = bwc_setup_p->coalesce_ratio;
			dis_resp.um_load_ratio_max = bwc_setup_p->load_ratio_max;
			dis_resp.um_load_ratio_min = bwc_setup_p->load_ratio_min;
			dis_resp.um_load_ctrl_level = bwc_setup_p->load_ctrl_level;
			dis_resp.um_flush_delay_ctl = bwc_setup_p->flush_delay_ctl;
			dis_resp.um_throttle_ratio = bwc_setup_p->throttle_ratio;
			dis_resp.um_ssd_mode = bwc_setup_p->ssd_mode;
			dis_resp.um_max_flush_size = bwc_setup_p->max_flush_size;
			dis_resp.um_write_delay_first_level = bwc_setup_p->write_delay_first_level;
			dis_resp.um_write_delay_second_level = bwc_setup_p->write_delay_second_level;
			dis_resp.um_write_delay_first_level_max = bwc_setup_p->write_delay_first_level_max_us;
			dis_resp.um_write_delay_second_level_max = bwc_setup_p->write_delay_second_level_max_us;
			dis_resp.um_high_water_mark = bwc_setup_p->high_water_mark;
			dis_resp.um_low_water_mark = bwc_setup_p->low_water_mark;

			msghdr = (struct umessage_bwc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_BWC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_BWC_CODE_S_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_bwc_hdr *)&dis_resp;
		msghdr->um_req = UMSG_BWC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_BWC_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_BWC_CODE_W_DISP:
		if (dis_msg->um_bwc_uuid == NULL)
			goto out;
		bwc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, dis_msg->um_bwc_uuid);

		if (bwc_p == NULL)
			goto out;

		bwc_setup_p = bwcg_p->wg_op->wg_get_all_bwc_setup(bwcg_p, &num_bwc);
		for (i = 0; i <num_bwc; i++, bwc_setup_p++) {
			if (!strcmp(bwc_setup_p->uuid, dis_msg->um_bwc_uuid)) {
				dis_resp.um_bwc_uuid_len = strlen(bwc_setup_p->uuid);
				memcpy(dis_resp.um_bwc_uuid, bwc_setup_p->uuid, dis_resp.um_bwc_uuid_len);
				dis_resp.um_bufdev_len = strlen(bwc_setup_p->bufdevice);
				memcpy(dis_resp.um_bufdev, bwc_setup_p->bufdevice, dis_resp.um_bufdev_len);
				dis_resp.um_tp_name_len = strlen(bwc_setup_p->tp_name);
				memcpy(dis_resp.um_tp_name, bwc_setup_p->tp_name, dis_resp.um_tp_name_len);
				dis_resp.um_bufdev_sz = bwc_setup_p->bufdevicesz;
				dis_resp.um_rw_sync = bwc_setup_p->rw_sync;
				dis_resp.um_two_step_read = bwc_setup_p->two_step_read;
				dis_resp.um_do_fp = bwc_setup_p->do_fp;
				dis_resp.um_bfp_type = bwc_setup_p->bfp_type;
				dis_resp.um_coalesce_ratio = bwc_setup_p->coalesce_ratio;
				dis_resp.um_load_ratio_max = bwc_setup_p->load_ratio_max;
				dis_resp.um_load_ratio_min = bwc_setup_p->load_ratio_min;
				dis_resp.um_load_ctrl_level = bwc_setup_p->load_ctrl_level;
				dis_resp.um_flush_delay_ctl = bwc_setup_p->flush_delay_ctl;
				dis_resp.um_throttle_ratio = bwc_setup_p->throttle_ratio;
				dis_resp.um_ssd_mode = bwc_setup_p->ssd_mode;
				dis_resp.um_max_flush_size = bwc_setup_p->max_flush_size;
				dis_resp.um_write_delay_first_level = bwc_setup_p->write_delay_first_level;
				dis_resp.um_write_delay_second_level = bwc_setup_p->write_delay_second_level;
				dis_resp.um_write_delay_first_level_max = bwc_setup_p->write_delay_first_level_max_us;
				dis_resp.um_write_delay_second_level_max = bwc_setup_p->write_delay_second_level_max_us;
				dis_resp.um_high_water_mark = bwc_setup_p->high_water_mark;
				dis_resp.um_low_water_mark = bwc_setup_p->low_water_mark;

				msghdr = (struct umessage_bwc_hdr *)&dis_resp;
				msghdr->um_req = UMSG_BWC_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_BWC_CODE_W_RESP_DISP;

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

	case UMSG_BWC_CODE_W_DISP_ALL:
		bwc_setup_p = bwcg_p->wg_op->wg_get_all_bwc_setup(bwcg_p, &num_bwc);
		working_bwc = bwcg_p->wg_op->wg_get_working_bwc_index(bwcg_p, &num_working_bwc);

		for (i = 0; i < num_working_bwc; i++) {
			memset(&dis_resp, 0, sizeof(dis_resp));
			cur_index = working_bwc[i];
			dis_resp.um_bwc_uuid_len = strlen(bwc_setup_p[cur_index].uuid);
			memcpy(dis_resp.um_bwc_uuid, bwc_setup_p[cur_index].uuid, dis_resp.um_bwc_uuid_len);
			dis_resp.um_bufdev_len = strlen(bwc_setup_p[cur_index].bufdevice);
			memcpy(dis_resp.um_bufdev, bwc_setup_p[cur_index].bufdevice, dis_resp.um_bufdev_len);
			dis_resp.um_tp_name_len = strlen(bwc_setup_p[cur_index].tp_name);
			memcpy(dis_resp.um_tp_name, bwc_setup_p[cur_index].tp_name, dis_resp.um_tp_name_len);
			dis_resp.um_bufdev_sz = bwc_setup_p[cur_index].bufdevicesz;
			dis_resp.um_rw_sync = bwc_setup_p[cur_index].rw_sync;
			dis_resp.um_two_step_read = bwc_setup_p[cur_index].two_step_read;
			dis_resp.um_do_fp = bwc_setup_p[cur_index].do_fp;
			dis_resp.um_bfp_type = bwc_setup_p[cur_index].bfp_type;
			dis_resp.um_coalesce_ratio = bwc_setup_p[cur_index].coalesce_ratio;
			dis_resp.um_load_ratio_max = bwc_setup_p[cur_index].load_ratio_max;
			dis_resp.um_load_ratio_min = bwc_setup_p[cur_index].load_ratio_min;
			dis_resp.um_load_ctrl_level = bwc_setup_p[cur_index].load_ctrl_level;
			dis_resp.um_flush_delay_ctl = bwc_setup_p[cur_index].flush_delay_ctl;
			dis_resp.um_throttle_ratio = bwc_setup_p[cur_index].throttle_ratio;
			dis_resp.um_ssd_mode = bwc_setup_p[cur_index].ssd_mode;
			dis_resp.um_max_flush_size = bwc_setup_p[cur_index].max_flush_size;
			dis_resp.um_write_delay_first_level = bwc_setup_p[cur_index].write_delay_first_level;
			dis_resp.um_write_delay_second_level = bwc_setup_p[cur_index].write_delay_second_level;
			dis_resp.um_write_delay_first_level_max = bwc_setup_p[cur_index].write_delay_first_level_max_us;
			dis_resp.um_write_delay_second_level_max = bwc_setup_p[cur_index].write_delay_second_level_max_us;
			dis_resp.um_high_water_mark = bwc_setup_p[cur_index].high_water_mark;
			dis_resp.um_low_water_mark = bwc_setup_p[cur_index].low_water_mark;

			msghdr = (struct umessage_bwc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_BWC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_BWC_CODE_W_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_bwc_hdr *)&dis_resp;
		msghdr->um_req = UMSG_BWC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_BWC_CODE_DISP_END;

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
	free(working_bwc);
	close(sfd);
}

static void
__bwcc_hello(struct bwcc_private *priv_p, char *msg_buf, struct umessage_bwc_hello *hello_msg)
{
	char *log_header = "__bwcc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_bwc_hdr *msghdr;
	int ctype = NID_CTYPE_BWC, rc;
	char nothing_back;
	struct umessage_bwc_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_bwc_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_BWC_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_BWC_HEADER_LEN);

	if (cmd_len > UMSG_BWC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_BWC_HEADER_LEN, cmd_len - UMSG_BWC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_BWC_CODE_HELLO:
		msghdr = (struct umessage_bwc_hdr *)&hello_resp;
		msghdr->um_req = UMSG_BWC_CMD_HELLO;
		msghdr->um_req_code = UMSG_BWC_CODE_RESP_HELLO;

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
bwcc_do_channel(struct bwcc_interface *bwcc_p, struct scg_interface *scg_p)
{
	char *log_header = "bwcc_do_channel";
	struct bwcc_private *priv_p = (struct bwcc_private *)bwcc_p->w_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_bwc bwc_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&bwc_msg;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_BWC_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	nid_log_error("um_req: %u", msghdr->um_req);

	if (!priv_p->p_bwcg) {
		nid_log_warning("%s support_bwc should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr->um_req) {
	case UMSG_BWC_CMD_DROPCACHE:
		__bwcc_dropcache(priv_p, msg_buf, (struct umessage_bwc_dropcache *)msghdr);
		break;

	case UMSG_BWC_CMD_INFORMATION:
		__bwcc_information(priv_p, msg_buf, (struct umessage_bwc_information *)msghdr);
		break;

	case UMSG_BWC_CMD_THROUGHPUT:
		__bwcc_throughput(priv_p, msg_buf, (struct umessage_bwc_throughput *)msghdr);
		break;

	case UMSG_BWC_CMD_ADD:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__bwcc_add(priv_p, msg_buf, (struct umessage_bwc_add *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_BWC_CMD_REMOVE:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__bwcc_remove(priv_p, msg_buf, (struct umessage_bwc_remove *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_BWC_CMD_FFLUSH:
		__bwcc_fflush(priv_p, msg_buf, (struct umessage_bwc_fflush *)msghdr);
		break;

	case UMSG_BWC_CMD_RECOVER:
		__bwcc_recover(priv_p, msg_buf, (struct umessage_bwc_recover *)msghdr);
		break;

	case UMSG_BWC_CMD_SNAPSHOT:
		__bwcc_admin_snapshot(priv_p, msg_buf, (struct umessage_bwc_snapshot *)msghdr);
		break;

	case UMSG_BWC_CMD_IOINFO:
		__bwcc_ioinfo(priv_p, msg_buf, (struct umessage_bwc_ioinfo *)msghdr);
		break;

	case UMSG_BWC_CMD_UPDATE_WATER_MARK:
		__bwcc_update_water_mark(priv_p, msg_buf, (struct umessage_bwc_water_mark *)msghdr);
		break;
	case UMSG_BWC_CMD_UPDATE_DELAY:
		__bwcc_update_delay(priv_p, msg_buf, (struct umessage_bwc_delay *)msghdr);
		break;

	case UMSG_BWC_CMD_FLUSH_EMPTY:
		__bwcc_flush_empty(priv_p, msg_buf, (struct umessage_bwc_flush_empty *)msghdr);
		break;

	case UMSG_BWC_CMD_DISPLAY:
		__bwcc_display(priv_p, msg_buf, (struct umessage_bwc_display *)msghdr);
		break;

	case UMSG_BWC_CMD_HELLO:
		__bwcc_hello(priv_p, msg_buf, (struct umessage_bwc_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
	}
}

static void
bwcc_cleanup(struct bwcc_interface *bwcc_p)
{
	nid_log_debug("bwcc_cleanup start, bwcc_p:%p", bwcc_p);
	if (bwcc_p->w_private != NULL) {
		free(bwcc_p->w_private);
		bwcc_p->w_private = NULL;
	}
}

struct bwcc_operations bwcc_op = {
	.w_accept_new_channel = bwcc_accept_new_channel,
	.w_do_channel = bwcc_do_channel,
	.w_cleanup = bwcc_cleanup,
};

int
bwcc_initialization(struct bwcc_interface *bwcc_p, struct bwcc_setup *setup)
{
	char *log_header = "bwcc_initialization";
	struct bwcc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	bwcc_p->w_private = priv_p;
	bwcc_p->w_op = &bwcc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_bwcg = setup->bwcg;
	return 0;
}
