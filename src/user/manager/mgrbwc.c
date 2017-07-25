/*
 * mgrbwc.c
 * 	Implementation of Manager to BWC Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "util_nw.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_bwc_if.h"
#include "mgrbwc_if.h"
#include "nid_shared.h"
#include "manager.h"

struct mgrbwc_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_BWC;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);
	sfd = util_nw_make_connection(ipstr, port);

	if (sfd < 0) {
		nid_log_error("cannot connect to the client (%s:%d), errno:%d",
			ipstr, port, errno);
	} else {
		if (util_nw_write_two_byte(sfd, chan_type) != NID_SIZE_CTYPE_MSG) {
			nid_log_error("%s: failed to send chan_type, errno:%d", log_header, errno);
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

static int
mgrbwc_throughput_stat_reset(struct mgrbwc_interface *mgrbwc_p, char *chan_uuid)
{
	char *log_header = "mgrbwc_throughput_stat_reset";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_throughput nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	char buf[1024];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (uuid:%s)...", log_header, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, chan_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_THROUGHPUT;

	msghdr->um_req_code = UMSG_BWC_CODE_THROUGHPUT_RESET;

	strcpy(nid_msg.um_bwc_uuid, chan_uuid);
	nid_msg.um_bwc_uuid_len = strlen(chan_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, chan_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_add(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *bufdev, uint32_t bufdev_sz,
		uint8_t rw_sync, uint8_t two_step_read, uint8_t do_fp, uint8_t bfp_type, double coalesce_ratio,
		double load_ratio_max, double load_ratio_min, double load_ctrl_level, double flush_delay_ctl,
		double throttle_ratio, char *tp_name, uint8_t ssd_mode, uint8_t max_flush_size
		, uint16_t write_delay_first_level, uint16_t write_delay_second_level, uint16_t high_water_mark,
		uint16_t low_water_mark)
{
	char *log_header = "mgrbwc_add";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_add nid_msg;
	struct umessage_bwc_add_resp nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_ADD;
	msghdr->um_req_code = UMSG_BWC_CODE_ADD;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	strcpy(nid_msg.um_bufdev, bufdev);
	nid_msg.um_bufdev_len = strlen(bufdev);
	nid_msg.um_bufdev_sz = bufdev_sz;
	nid_msg.um_rw_sync= rw_sync;
	nid_msg.um_two_step_read = two_step_read;
	nid_msg.um_do_fp = do_fp;
	nid_msg.um_bfp_type = bfp_type;
	nid_msg.um_coalesce_ratio = coalesce_ratio;
	nid_msg.um_load_ratio_max = load_ratio_max;
	nid_msg.um_load_ratio_min = load_ratio_min;
	nid_msg.um_load_ctrl_level = load_ctrl_level;
	nid_msg.um_flush_delay_ctl = flush_delay_ctl;
	nid_msg.um_throttle_ratio = throttle_ratio;
	nid_msg.um_ssd_mode = ssd_mode;
	nid_msg.um_max_flush_size = max_flush_size;
	nid_msg.um_write_delay_first_level = write_delay_first_level;
	nid_msg.um_write_delay_second_level = write_delay_second_level;
	nid_msg.um_high_water_mark = high_water_mark;
	nid_msg.um_low_water_mark = low_water_mark;
	strcpy(nid_msg.um_tp_name, tp_name);
	nid_msg.um_tp_name_len = strlen(tp_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_ADD);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_ADD);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: add bwc successfully\n",log_header);
	else
		printf("%s: add bwc failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_remove(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid)
{
	char *log_header = "mgrbwc_remove";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_remove nid_msg;
	struct umessage_bwc_remove_resp nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_REMOVE;
	msghdr->um_req_code = UMSG_BWC_CODE_REMOVE;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_REMOVE);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_REMOVE);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: remove bwc successfully\n",log_header);
	else
		printf("%s: remove bwc failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_dropcache_start(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chan_uuid, int do_sync)
{
	char *log_header = "mgrbwc_dropcache_start";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_dropcache nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	char buf[1024];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (uuid:%s)...", log_header, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, chan_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_DROPCACHE;
	if (!do_sync)
		msghdr->um_req_code = UMSG_BWC_CODE_START;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_START_SYNC;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, chan_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}


static int
mgrbwc_dropcache_stop(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chan_uuid)
{
	char *log_header = "mgrbwc_dropcache_stop";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_dropcache nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	char buf[4096];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_DROPCACHE;
	msghdr->um_req_code = UMSG_BWC_CODE_STOP;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_information_flushing(struct mgrbwc_interface *mgrbwc_p, char *c_uuid)
{
	char *log_header = "mgrbwc_information_flushing";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_information nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	char buf[4096];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_BWC_CODE_FLUSHING;
	strcpy(nid_msg.um_bwc_uuid, c_uuid);
	nid_msg.um_bwc_uuid_len = strlen(c_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_snapshot_admin(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chain_uuid, int snapshot_op)
{
	char *log_header = "mgrbwc_snapshot_admin";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_snapshot nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	char buf[4096];
	int sfd = -1;
	uint32_t len;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	// send resource freeze/unfreeze command to nidserver
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_SNAPSHOT;
	msghdr->um_req_code = snapshot_op;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	strcpy(nid_msg.um_chan_uuid, chain_uuid);
	nid_msg.um_chan_uuid_len = strlen(chain_uuid);

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	struct umessage_bwc_snapshot_resp nid_msg_resp;

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	int nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	char *p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_SNAPSHOT);
	if(msghdr->um_req_code != UMSG_BWC_CODE_SNAPSHOT_FREEZE_RESP &&
	   msghdr->um_req_code != UMSG_BWC_CODE_SNAPSHOT_UNFREEZE_RESP) {
		nid_log_debug("%s: invalid command, req:%d, code:%d\n",
			log_header, msghdr->um_req, msghdr->um_req_code);
		assert(0);
	}
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);

	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	if(nread != (int)(msghdr->um_len - UMSG_BWC_HEADER_LEN)) {
		nid_log_debug("%s: failed to read response message, ret: %d\n",
			log_header, nread);
		assert(0);
	}
	rc = umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if(rc) {
		nid_log_debug("%s: failed to decode response message, ret: %d\n",
			log_header, rc);
		assert(0);
	}
	struct umessage_bwc_snapshot_resp *snapshot_resp = (struct umessage_bwc_snapshot_resp *)msghdr;
	rc = snapshot_resp->um_resp_code;

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}


static int
mgrbwc_information_stat(struct mgrbwc_interface *mgrbwc_p, char *c_uuid)
{
	char *log_header = "mgrbwc_information_stat";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_information nid_msg;
	struct umessage_bwc_information_resp_stat nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096], *m_stat;
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_BWC_CODE_STAT;
	strcpy(nid_msg.um_bwc_uuid, c_uuid);
	nid_msg.um_bwc_uuid_len = strlen(c_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_STAT);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	printf("%s stat:\n", nid_msg_resp.um_bwc_uuid);
	if (nid_msg_resp.um_state == NID_STAT_ACTIVE)
		m_stat = "active";
	else if (nid_msg_resp.um_state == NID_STAT_INACTIVE)
		m_stat = "inactive";
	else
		m_stat = "unknown";
	printf(	first_intend"state:%s(%d)\n"
		first_intend"block_occupied:%u/%u\n"
		first_intend"cur_write_block:%u\n"
		first_intend"wc_cur_flush_block:%u\n"
		first_intend"seq_assigned:%lu\n"
		first_intend"seq_flushed:%lu\n"
		first_intend"rec_seq_flushed:%lu\n"
		first_intend"resp_counter:%lu\n",
		m_stat,
		nid_msg_resp.um_state,
		nid_msg_resp.um_block_occupied,
		nid_msg_resp.um_flush_nblocks,
		nid_msg_resp.um_cur_write_block,
		nid_msg_resp.um_cur_flush_block,
		nid_msg_resp.um_seq_assigned,
		nid_msg_resp.um_seq_flushed,
		nid_msg_resp.um_rec_seq_flushed,
		nid_msg_resp.um_resp_counter);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}


static int
mgrbwc_information_throughput_stat(struct mgrbwc_interface *mgrbwc_p, char *c_uuid)
{
	char *log_header = "mgrbwc_information_throughput_stat";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_information nid_msg;
	struct umessage_bwc_information_resp_throughput_stat nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_BWC_CODE_THROUGHPUT_STAT;
	strcpy(nid_msg.um_bwc_uuid, c_uuid);
	nid_msg.um_bwc_uuid_len = strlen(c_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_THROUGHPUT_STAT_RSLT);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	printf(	"%s bwc throughput stat:\n"
		first_intend"data flushed:%lu\n"
		first_intend"data package flushed:%lu\n"
		first_intend"nondata package flushed:%lu\n",

        nid_msg_resp.um_bwc_uuid,
		nid_msg_resp.um_seq_data_flushed,
		nid_msg_resp.um_seq_data_pkg_flushed,
		nid_msg_resp.um_seq_nondata_pkg_flushed);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_information_rw_stat(struct mgrbwc_interface *mgrbwc_p, char *c_uuid, char *ch_uuid)
{
	char *log_header = "mgrbwc_information_rw_stat";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_information nid_msg;
	struct umessage_bwc_information_resp_rw_stat nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_BWC_CODE_RW_STAT;
	strcpy(nid_msg.um_bwc_uuid, c_uuid);
	nid_msg.um_bwc_uuid_len = strlen(c_uuid);
	strcpy(nid_msg.um_chan_uuid, ch_uuid);
	nid_msg.um_chan_uuid_len = strlen(ch_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RW_STAT_RSLT);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (nid_msg_resp.res){
		printf(	"%s bwc rw stat:\n"
			first_intend"overwritten_num:%lu\n"
			first_intend"overwritten_back_num:%lu\n"
			first_intend"coalesce_flush_num:%lu\n"
			first_intend"coalesce_flush_back_num:%lu\n"
			first_intend"flush_num:%lu\n"
			first_intend"flush_back_num:%lu\n"
			first_intend"flush_page_num:%lu\n"
			first_intend"flush_not_ready_num:%lu\n",
			nid_msg_resp.um_bwc_uuid,
			nid_msg_resp.um_seq_overwritten_counter,
			nid_msg_resp.um_seq_overwritten_back_counter,
			nid_msg_resp.um_seq_coalesce_flush_num,
			nid_msg_resp.um_seq_coalesce_flush_back_num,
			nid_msg_resp.um_seq_flush_num,
			nid_msg_resp.um_seq_flush_back_num,
			nid_msg_resp.um_seq_flush_page_num,
			nid_msg_resp.um_seq_not_ready_num);
	} else {
		printf("wrong bwc uuid or channel uuid\n");
	}

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_information_list_stat(struct mgrbwc_interface *mgrbwc_p, char *c_uuid, char *ch_uuid)
{
	char *log_header = "mgrbwc_information_list_stat";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_information nid_msg;
	struct umessage_bwc_information_list_resp_stat nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_BWC_CODE_LIST_STAT;
	strcpy(nid_msg.um_bwc_uuid, c_uuid);
	nid_msg.um_bwc_uuid_len = strlen(c_uuid);
	strcpy(nid_msg.um_chan_uuid, ch_uuid);
	nid_msg.um_chan_uuid_len = strlen(ch_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_LIST_RESP_STAT);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (nid_msg_resp.um_found_it){
		printf(	"%s bwc request list stat:\n"
			first_intend"to_write_counter:%u\n"
			first_intend"to_read_counter:%u\n"
			first_intend"write_counter:%u\n"
			first_intend"write_vec_counter:%u\n"
			first_intend"write_busy:%u\n"
			first_intend"ready_busy:%u\n",
			nid_msg_resp.um_bwc_uuid,
			nid_msg_resp.um_to_write_list_counter,
			nid_msg_resp.um_to_read_list_counter,
			nid_msg_resp.um_write_list_counter,
			nid_msg_resp.um_write_vec_list_counter,
			nid_msg_resp.um_write_busy,
			nid_msg_resp.um_read_busy);
	} else {
		printf("wrong channel uuid\n");
	}

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_information_delay_stat(struct mgrbwc_interface *mgrbwc_p, char *c_uuid)
{
	char *log_header = "mgrbwc_information_delay_stat";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_information nid_msg;
	struct umessage_bwc_information_resp_delay_stat nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_BWC_CODE_DELAY_STAT;
	strcpy(nid_msg.um_bwc_uuid, c_uuid);
	nid_msg.um_bwc_uuid_len = strlen(c_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_DELAY_STAT);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	printf("%s bwc delay stat:\n", nid_msg_resp.um_bwc_uuid);
	printf(	first_intend"write_delay_first_level:%u(blocks)\n"
		first_intend"write_delay_first_level_max:%u\n"
		first_intend"write_delay_first_level_increase_interval:%u\n"
		first_intend"write_delay_second_level:%u(blocks)\n"
		first_intend"write_delay_second_level_max:%u\n"
		first_intend"write_delay_second_level_increase_interval:%u\n"
		first_intend"write_delay_second_level_start:%u\n"
		first_intend"write_delay_time:%u\n",
		nid_msg_resp.um_write_delay_first_level,
		nid_msg_resp.um_write_delay_first_level_max,
		nid_msg_resp.um_write_delay_first_level_increase_interval,
		nid_msg_resp.um_write_delay_second_level,
		nid_msg_resp.um_write_delay_second_level_max,
		nid_msg_resp.um_write_delay_second_level_increase_interval,
		nid_msg_resp.um_write_delay_second_level_start,
		nid_msg_resp.um_write_delay_time);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_fflush_start(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chan_uuid)
{
	char *log_header = "mgrbwc_fflush_start";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_fflush nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	char buf[1024];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_FFLUSH;
	msghdr->um_req_code = UMSG_BWC_CODE_FF_START;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);

	nid_log_warning("%s: step 2 (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, chan_uuid:%s, len:%d, plen:%d",
			log_header, bwc_uuid, chan_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static char *mgrbwc_fflush_states[] = {
		"none",
		"doing",
		"done",
};

static int
mgrbwc_fflush_get(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chan_uuid)
{
	char *log_header = "mgrbwc_fflush_get";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_fflush nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_fflush_resp_get nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_FFLUSH;
	msghdr->um_req_code = UMSG_BWC_CODE_FF_GET;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);

	nid_log_warning("%s: step 2 (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, chan_uuid:%s, len:%d, plen:%d",
			log_header, bwc_uuid, chan_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_FFLUSH);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_FF_GET);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	assert(nid_msg_resp.um_ff_state < 3);
	printf(	"%s fast flush state:\n"
		first_intend"%s\n",
		nid_msg_resp.um_chan_uuid,
		mgrbwc_fflush_states[nid_msg_resp.um_ff_state]);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_fflush_stop(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chan_uuid)
{
	char *log_header = "mgrbwc_fflush_stop";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_fflush nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	char buf[1024];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_FFLUSH;
	msghdr->um_req_code = UMSG_BWC_CODE_FF_STOP;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);

	nid_log_warning("%s: step 2 (bwc_uuid:%s, chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, chan_uuid:%s, len:%d, plen:%d",
			log_header, bwc_uuid, chan_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_recover_start(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid)
{
	char *log_header = "mgrbwc_recover_start";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_recover nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	char buf[1024];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (bwc_uuid:%s)...", log_header, bwc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_RECOVER;
	msghdr->um_req_code = UMSG_BWC_CODE_RCV_START;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);

	nid_log_warning("%s: step 2 (bwc_uuid:%s)...", log_header, bwc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d, plen:%d",
			log_header, bwc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static char *mgrbwc_recover_states[] = {
		"none",
		"doing",
		"done",
};

static int
mgrbwc_recover_get(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid)
{
	char *log_header = "mgrbwc_recover_get";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_recover nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_recover_resp_get nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (bwc_uuid:%s)...", log_header, bwc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_RECOVER;
	msghdr->um_req_code = UMSG_BWC_CODE_RCV_GET;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);

	nid_log_warning("%s: step 2 (bwc_uuid:%s)...", log_header, bwc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d, plen:%d",
			log_header, bwc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_RECOVER);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_RCV_GET);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	assert(nid_msg_resp.um_rcv_state < 3);
	printf(	"%s recovery state:\n"
		first_intend"%s\n",
		nid_msg_resp.um_bwc_uuid,
		mgrbwc_recover_states[nid_msg_resp.um_rcv_state]);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_water_mark(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, uint16_t high_water_mark, uint16_t low_water_mark)
{
	char *log_header = "mgrbwc_water_mark";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_water_mark nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_water_mark_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_UPDATE_WATER_MARK;
	msghdr->um_req_code = UMSG_BWC_CODE_UPDATE_WATER_MARK;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	nid_msg.um_high_water_mark = high_water_mark;
	nid_msg.um_low_water_mark = low_water_mark;

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_UPDATE_WATER_MARK);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_UPDATE_WATER_MARK);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (nid_msg_resp.um_resp_code)
		printf("%s: set water_mark successfully!\n", nid_msg_resp.um_bwc_uuid);
	else
		printf("%s: fail to set water_mark!\n", nid_msg_resp.um_bwc_uuid);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_ioinfo_start(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chan_uuid, uint8_t is_bfe)
{
	char *log_header = "mgrbwc_ioinfo_start";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_ioinfo nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)(chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_IOINFO;
	if (is_bfe)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_START;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_START;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	if (is_bfe) {
		strcpy(nid_msg.um_chan_uuid, chan_uuid);
		nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	}

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_IOINFO);
	if (is_bfe)
		assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_START);
	else
		assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_START);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);

	if (nid_msg_resp.um_is_running)
		printf("start count successfully!\n");
	else
		printf("fail to start count!\n");

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_ioinfo_stop(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chan_uuid, uint8_t is_bfe)
{
	char *log_header = "mgrbwc_ioinfo_stop";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_ioinfo nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)(chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_IOINFO;
	if (is_bfe)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_STOP;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_STOP;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	if (is_bfe) {
		strcpy(nid_msg.um_chan_uuid, chan_uuid);
		nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	}

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_IOINFO);
	if (is_bfe)
		assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP);
	else
		assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_STOP);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (nid_msg_resp.um_is_running) {
		printf("fail to stop count!\n");
	}
	else {
		printf("status: stopped\n");
		printf("req_num: %ld\n", nid_msg_resp.um_req_num);
		if (nid_msg_resp.um_req_num == 0)
			printf("average_len: 0\n");
		else
			printf("average_len: %ld\n", nid_msg_resp.um_req_len/nid_msg_resp.um_req_num);
	}

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_ioinfo_check(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, char *chan_uuid, uint8_t is_bfe)
{
	char *log_header = "mgrbwc_ioinfo_check";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_ioinfo nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)(chan_uuid:%s)...", log_header, bwc_uuid, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_IOINFO;
	if (is_bfe)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_CHECK;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_CHECK;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	if (is_bfe) {
		strcpy(nid_msg.um_chan_uuid, chan_uuid);
		nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	}

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_IOINFO);
	if (is_bfe)
		assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK);
	else
		assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_CHECK);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (nid_msg_resp.um_is_running)
		printf("status: running\n");
	else
		printf("status: not running\n");

	printf("req_num: %ld\n", nid_msg_resp.um_req_num);
	if (nid_msg_resp.um_req_num == 0)
		printf("average_len: 0\n");
	else
		printf("average_len: %ld\n", nid_msg_resp.um_req_len/nid_msg_resp.um_req_num);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_ioinfo_start_all(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, uint8_t is_bfe)
{
	char *log_header = "mgrbwc_ioinfo_start_all";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_ioinfo nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_IOINFO;
	if (is_bfe && bwc_uuid)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_CHAIN_START_ALL;
	else if (is_bfe && !bwc_uuid)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_START_ALL;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_START_ALL;

	if (is_bfe && bwc_uuid) {
		strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
		nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	}

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	while (1) {
		msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
		if (nread != UMSG_BWC_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
			log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_END)
			goto out;

		assert(msghdr->um_req == UMSG_BWC_CMD_IOINFO);
		if (is_bfe && bwc_uuid)
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_START_ALL);
		else if (is_bfe && !bwc_uuid)
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_START_ALL);
		else
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_START_ALL);
		assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);

		if (nid_msg_resp.um_is_running) {
			if (is_bfe)
				printf("%s:%s:start count successfully!\n",
					nid_msg_resp.um_bwc_uuid, nid_msg_resp.um_chan_uuid);
			else
				printf("%s:start count successfully!\n",
					nid_msg_resp.um_bwc_uuid);


		} else {
			if (is_bfe)
				printf("%s:%s:fail to start count!\n",
					nid_msg_resp.um_bwc_uuid, nid_msg_resp.um_chan_uuid);
			else
				printf("%sfail to start count!\n",
					nid_msg_resp.um_bwc_uuid);
		}
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_ioinfo_stop_all(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, uint8_t is_bfe)
{
	char *log_header = "mgrbwc_ioinfo_stop_all";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_ioinfo nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_IOINFO;
	if (is_bfe && bwc_uuid)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_CHAIN_STOP_ALL;
	else if (is_bfe && !bwc_uuid)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_STOP_ALL;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_STOP_ALL;

	if (is_bfe && bwc_uuid) {
		strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
		nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	}

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	while (1) {
		msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
		if (nread != UMSG_BWC_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
			log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_END)
			goto out;

		assert(msghdr->um_req == UMSG_BWC_CMD_IOINFO);
		if (is_bfe && bwc_uuid)
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_STOP_ALL);
		else if (is_bfe && !bwc_uuid)
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_STOP_ALL);
		assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
		if (nid_msg_resp.um_is_running) {
			printf("%s:fail to stop count!\n", nid_msg_resp.um_bwc_uuid);
		}
		else {
			printf("bwc_uuid:%s\n", nid_msg_resp.um_bwc_uuid);
			if (is_bfe)
				printf("chan_uuid:%s\n", nid_msg_resp.um_chan_uuid);
			printf("status: stopped\n");
			printf("req_num: %ld\n", nid_msg_resp.um_req_num);
			if (nid_msg_resp.um_req_num == 0)
				printf("average_len: 0\n\n");
			else
				printf("average_len: %ld\n\n", nid_msg_resp.um_req_len/nid_msg_resp.um_req_num);
		}
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_ioinfo_check_all(struct mgrbwc_interface *mgrbwc_p, char * bwc_uuid, uint8_t is_bfe)
{
	char *log_header = "mgrbwc_ioinfo_check_all";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_ioinfo nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_IOINFO;
	if (is_bfe && bwc_uuid)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_CHAIN_CHECK_ALL;
	else if (is_bfe && !bwc_uuid)
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_BFE_CHECK_ALL;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_IOINFO_CHECK_ALL;

	if (is_bfe && bwc_uuid) {
		strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
		nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	}

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	while(1) {
		msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
		if (nread != UMSG_BWC_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
			log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_END)
			goto out;

		assert(msghdr->um_req == UMSG_BWC_CMD_IOINFO);
		if (is_bfe && bwc_uuid)
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_CHECK_ALL);
		else if (is_bfe && !bwc_uuid)
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK_ALL);
		else
			assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_IOINFO_CHECK_ALL);
		assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
		printf("bwc_uuid:%s\n", nid_msg_resp.um_bwc_uuid);
		if (is_bfe)
			printf("chan_uuid:%s\n", nid_msg_resp.um_chan_uuid);
		if (nid_msg_resp.um_is_running)
			printf("status: running\n");
		else
			printf("status: not running\n");

		printf("req_num: %ld\n", nid_msg_resp.um_req_num);
		if (nid_msg_resp.um_req_num == 0)
			printf("average_len: 0\n\n");
		else
			printf("average_len: %ld\n\n", nid_msg_resp.um_req_len/nid_msg_resp.um_req_num);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_delay(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, uint16_t write_delay_first_level, uint16_t write_delay_second_level, uint32_t write_delay_first_level_max, uint32_t write_delay_second_level_max)
{
	char *log_header = "mgrbwc_delay";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_delay nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_delay_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_UPDATE_DELAY;
	msghdr->um_req_code = UMSG_BWC_CODE_UPDATE_DELAY;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	nid_msg.um_write_delay_first_level = write_delay_first_level;
	nid_msg.um_write_delay_second_level = write_delay_second_level;
	nid_msg.um_write_delay_first_level_max = write_delay_first_level_max;
	nid_msg.um_write_delay_second_level_max = write_delay_second_level_max;

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_UPDATE_DELAY);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_UPDATE_DELAY);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: set write delay successfully!\n", nid_msg_resp.um_bwc_uuid);
	else
		printf("%s: fail to set write delay!\n", nid_msg_resp.um_bwc_uuid);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_flush_empty_start(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid)
{
	char *log_header = "mgrbwc_flush_empty";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_flush_empty nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_flush_empty_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_FLUSH_EMPTY;
	msghdr->um_req_code = UMSG_BWC_CODE_FLUSH_EMPTY_START;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_FLUSH_EMPTY);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_FLUSH_EMPTY_START);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: start flush_empty successfully!\n", nid_msg_resp.um_bwc_uuid);
	else
		printf("%s: fail to start flush_empty!\n", nid_msg_resp.um_bwc_uuid);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_flush_empty_stop(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid)
{
	char *log_header = "mgrbwc_flush_empty";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_flush_empty nid_msg;
	struct umessage_bwc_hdr *msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	struct umessage_bwc_flush_empty_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_warning("%s: start (bwc_uuid:%s)...", log_header, bwc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_BWC_CMD_FLUSH_EMPTY;
	msghdr->um_req_code = UMSG_BWC_CODE_FLUSH_EMPTY_STOP;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: bwc_uuid:%s, len:%d",
			log_header, bwc_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_FLUSH_EMPTY);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_FLUSH_EMPTY_STOP);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: stop flush_empty successfully!\n", nid_msg_resp.um_bwc_uuid);
	else
		printf("%s: fail to stop flush_empty!\n", nid_msg_resp.um_bwc_uuid);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_display(struct mgrbwc_interface *mgrbwc_p, char *bwc_uuid, uint8_t is_setup)
{
	char *log_header = "mgrbwc_display";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_display nid_msg;
	struct umessage_bwc_display_resp nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_BWC_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_W_DISP;
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(bwc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_BWC_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_BWC_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);

	printf("bwc %s:\n\ttype = bwc\n\tcache_device = %s\n\tcache_size = %d\n\trw_sync = %d\n\ttwo_step_read = %d\n\t"
		"do_fp = %d\n\ttp_name = %s\n\tssd_mode = %d\n\tmax_flush_size = %d\n\twrite_delay_first_level = %d\n\t"
		"write_delay_second_level = %d\n\twrite_delay_first_level_max_us = %d\n\twrite_delay_second_level_max_us = %d\n",
		nid_msg_resp.um_bwc_uuid, nid_msg_resp.um_bufdev, nid_msg_resp.um_bufdev_sz, nid_msg_resp.um_rw_sync,
		nid_msg_resp.um_two_step_read, nid_msg_resp.um_do_fp, nid_msg_resp.um_tp_name, nid_msg_resp.um_ssd_mode,
		nid_msg_resp.um_max_flush_size, nid_msg_resp.um_write_delay_first_level, nid_msg_resp.um_write_delay_second_level,
		nid_msg_resp.um_write_delay_first_level_max, nid_msg_resp.um_write_delay_second_level_max);

	if (nid_msg_resp.um_bfp_type == 1)
		printf("\thigh_water_mark: %d\n\tlow_water_mark: %d\n\tflush_policy = bfp1\n",
			nid_msg_resp.um_high_water_mark, nid_msg_resp.um_low_water_mark);
	else
		printf("\tload_ratio_min = %f\n\tload_ratio_max = %f\n\tload_ctrl_level = %f\n"
			"\tflush_delay_ctl = %f\n",
			nid_msg_resp.um_load_ratio_min, nid_msg_resp.um_load_ratio_max, nid_msg_resp.um_load_ctrl_level,
			nid_msg_resp.um_flush_delay_ctl);

	if (nid_msg_resp.um_bfp_type == 2)
		printf("\tflush_policy = bfp2\n");

	if (nid_msg_resp.um_bfp_type == 3)
		printf("\tcoalesce_ratio = %f\n\tflush_policy = bfp3\n", nid_msg_resp.um_coalesce_ratio);

	if (nid_msg_resp.um_bfp_type == 4)
		printf("\tthrottle_ratio = %f\n\tflush_policy = bfp4\n", nid_msg_resp.um_throttle_ratio);

	if (nid_msg_resp.um_bfp_type == 5)
		printf("\tthrottle_ratio = %f\n\tcoalesce_ratio = %f\n\tflush_policy = bfp5\n",
			nid_msg_resp.um_throttle_ratio, nid_msg_resp.um_coalesce_ratio);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_display_all(struct mgrbwc_interface *mgrbwc_p, uint8_t is_setup)
{
	char *log_header = "mgrbwc_display_all";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_display nid_msg;
	struct umessage_bwc_display_resp nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_BWC_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_BWC_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
		if (nread != UMSG_BWC_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, resp_len:%d, done\n",
			log_header, msghdr->um_req, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_BWC_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_BWC_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_BWC_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_BWC_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);

		printf("bwc %s:\n\ttype = bwc\n\tcache_device = %s\n\tcache_size = %d\n\trw_sync = %d\n\ttwo_step_read = %d\n\t"
			"do_fp = %d\n\ttp_name = %s\n\tssd_mode = %d\n\tmax_flush_size = %d\n\twrite_delay_first_level = %d\n\t"
			"write_delay_second_level = %d\n\twrite_delay_first_level_max_us = %d\n\twrite_delay_second_level_max_us = %d\n",
			nid_msg_resp.um_bwc_uuid, nid_msg_resp.um_bufdev, nid_msg_resp.um_bufdev_sz, nid_msg_resp.um_rw_sync,
			nid_msg_resp.um_two_step_read, nid_msg_resp.um_do_fp, nid_msg_resp.um_tp_name, nid_msg_resp.um_ssd_mode,
			nid_msg_resp.um_max_flush_size, nid_msg_resp.um_write_delay_first_level, nid_msg_resp.um_write_delay_second_level,
			nid_msg_resp.um_write_delay_first_level_max, nid_msg_resp.um_write_delay_second_level_max);

		if (nid_msg_resp.um_bfp_type == 1)
			printf("\thigh_water_mark: %d\n\tlow_water_mark: %d\n\tflush_policy = bfp1\n",
				nid_msg_resp.um_high_water_mark, nid_msg_resp.um_low_water_mark);
		else
			printf("\tload_ratio_min = %f\n\tload_ratio_max = %f\n\tload_ctrl_level = %f\n"
				"\tflush_delay_ctl = %f\n",
				nid_msg_resp.um_load_ratio_min, nid_msg_resp.um_load_ratio_max, nid_msg_resp.um_load_ctrl_level,
				nid_msg_resp.um_flush_delay_ctl);

		if (nid_msg_resp.um_bfp_type == 2)
			printf("\tflush_policy = bfp2\n");

		if (nid_msg_resp.um_bfp_type == 3)
			printf("\tcoalesce_ratio = %f\n\tflush_policy = bfp3\n", nid_msg_resp.um_coalesce_ratio);

		if (nid_msg_resp.um_bfp_type == 4)
			printf("\tthrottle_ratio = %f\n\tflush_policy = bfp4\n", nid_msg_resp.um_throttle_ratio);

		if (nid_msg_resp.um_bfp_type == 5)
			printf("\tthrottle_ratio = %f\n\tcoalesce_ratio = %f\n\tflush_policy = bfp5\n",
				nid_msg_resp.um_throttle_ratio, nid_msg_resp.um_coalesce_ratio);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrbwc_hello(struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "mgrbwc_hello";
	struct mgrbwc_private *priv_p = mgrbwc_p->bw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_bwc_hello nid_msg, nid_msg_resp;
	struct umessage_bwc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_BWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_bwc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_BWC_CMD_HELLO;
	msghdr->um_req_code = UMSG_BWC_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_bwc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_BWC_HEADER_LEN);
	if (nread != UMSG_BWC_HEADER_LEN) {
		printf("module bwc is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_BWC_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_BWC_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_BWC_HEADER_LEN);

	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_BWC_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_BWC_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_BWC, msghdr);
	printf("module bwc is supported.\n");

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrbwc_operations mgrbwc_op = {
	.bw_dropcache_start = mgrbwc_dropcache_start,
	.bw_dropcache_stop = mgrbwc_dropcache_stop,
	.bw_information_flushing = mgrbwc_information_flushing,
	.bw_information_stat = mgrbwc_information_stat,
	.bw_throughput_stat_reset= mgrbwc_throughput_stat_reset,
	.bw_information_throughput_stat = mgrbwc_information_throughput_stat,
	.bw_information_rw_stat = mgrbwc_information_rw_stat,
	.bw_information_delay_stat = mgrbwc_information_delay_stat,
	.bw_add = mgrbwc_add,
	.bw_remove = mgrbwc_remove,
	.bw_fflush_start = mgrbwc_fflush_start,
	.bw_fflush_get = mgrbwc_fflush_get,
	.bw_fflush_stop = mgrbwc_fflush_stop,
	.bw_recover_start = mgrbwc_recover_start,
	.bw_recover_get = mgrbwc_recover_get,
	.bw_snapshot_admin = mgrbwc_snapshot_admin,
	.bw_water_mark = mgrbwc_water_mark,
	.bw_ioinfo_start = mgrbwc_ioinfo_start,
	.bw_ioinfo_stop = mgrbwc_ioinfo_stop,
	.bw_ioinfo_check = mgrbwc_ioinfo_check,
	.bw_ioinfo_start_all = mgrbwc_ioinfo_start_all,
	.bw_ioinfo_stop_all = mgrbwc_ioinfo_stop_all,
	.bw_ioinfo_check_all = mgrbwc_ioinfo_check_all,
	.bw_delay = mgrbwc_delay,
	.bw_flush_empty_start = mgrbwc_flush_empty_start,
	.bw_flush_empty_stop = mgrbwc_flush_empty_stop,
	.bw_display = mgrbwc_display,
	.bw_display_all = mgrbwc_display_all,
	.bw_hello = mgrbwc_hello,
	.bw_information_list_stat = mgrbwc_information_list_stat,
};

int
mgrbwc_initialization(struct mgrbwc_interface *mgrbwc_p, struct mgrbwc_setup *setup)
{
	struct mgrbwc_private *priv_p;

	nid_log_debug("mgrbwc_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrbwc_p->bw_private = priv_p;
	mgrbwc_p->bw_op = &mgrbwc_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
