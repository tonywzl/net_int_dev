/*
 * mgrsac.c
 * 	Implementation of Manager to SAC Channel Module
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
#include "umpk_sac_if.h"
#include "mgrsac_if.h"
#include "nid_shared.h"

struct mgrsac_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_SAC;

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
mgrsac_information(struct mgrsac_interface *mgrsac_p, char *chan_uuid)
{
	char *log_header = "mgrsac_information";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_information nid_msg;
	struct umessage_sac_information_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096], *m_state;
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_SAC_CODE_STAT;
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_STAT);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);

	printf("%s:\n", nid_msg_resp.um_chan_uuid);
	if (nid_msg_resp.um_state == NID_STAT_ACTIVE)
		m_state = "active";
	else if (nid_msg_resp.um_state == NID_STAT_INACTIVE)
		m_state = "inactive";
	else
		m_state = "unknown";
	printf("\tstate:%s(%d)\n", m_state, nid_msg_resp.um_state);
	printf("\tip:%s\n"
		"\talevel:%s\n"
		"\trcounter:%lu\n"
		"\trreadycounter:%lu\n"
		"\trrcounter:%lu\n"
		"\twcounter:%lu\n"
		"\twreadycounter:%lu\n"
		"\trwcounter:%lu\n"
		"\tkcounter:%lu\n"
		"\trkcounter:%lu\n"
		"\trecv_sequence:%lu\n"
		"\twait_sequence:%lu\n"
		"\trtree_segsz:%u\n"
		"\trtree_nseg:%u\n"
		"\trtree_nfree:%u\n"
		"\trtree_nused:%u\n"
		"\tbtn_segsz:%u\n"
		"\tbtn_nseg:%u\n"
		"\tbtn_nfree:%u\n"
		"\tbtn_nused:%u\n"
		"\tbfe_page_counter: %d\n"
		"bre_page_counter: %d\n"
		"\tlive_time:%dsec\n",
		nid_msg_resp.um_ip,
		nid_alevel_to_str(nid_msg_resp.um_alevel),
		nid_msg_resp.um_rcounter,
		nid_msg_resp.um_rreadycounter,
		nid_msg_resp.um_r_rcounter,
		nid_msg_resp.um_wcounter,
		nid_msg_resp.um_wreadycounter,
		nid_msg_resp.um_r_wcounter,
		nid_msg_resp.um_kcounter,
		nid_msg_resp.um_r_kcounter,
		nid_msg_resp.um_recv_sequence,
		nid_msg_resp.um_wait_sequence,
		nid_msg_resp.um_rtree_segsz,
		nid_msg_resp.um_rtree_nseg,
		nid_msg_resp.um_rtree_nfree,
		nid_msg_resp.um_rtree_nused,
		nid_msg_resp.um_btn_segsz,
		nid_msg_resp.um_btn_nseg,
		nid_msg_resp.um_btn_nfree,
		nid_msg_resp.um_btn_nused,
		nid_msg_resp.um_bfe_page_counter,
		nid_msg_resp.um_bre_page_counter,
		nid_msg_resp.um_live_time);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_information_all(struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "mgrsac_information_all";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_information nid_msg;
	struct umessage_sac_information_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096], *m_state;
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_SAC_CODE_STAT_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
		if (nread != UMSG_SAC_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_SAC_CODE_STAT_END)
			goto out;

		assert(msghdr->um_req == UMSG_SAC_CMD_INFORMATION);
		assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_STAT_ALL);
		assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
		printf("%s:\n", nid_msg_resp.um_chan_uuid);
		if (nid_msg_resp.um_state == NID_STAT_ACTIVE)
			m_state = "active";
		else if (nid_msg_resp.um_state == NID_STAT_INACTIVE)
			m_state = "inactive";
		else
			m_state = "unknown";
		printf("\tstate:%s(%d)\n", m_state, nid_msg_resp.um_state);
		printf("\tip:%s\n"
			"\talevel:%s\n"
			"\trcounter:%lu\n"
			"\trreadycounter:%lu\n"
			"\trrcounter:%lu\n"
			"\twcounter:%lu\n"
			"\twreadycounter:%lu\n"
			"\trwcounter:%lu\n"
			"\tkcounter:%lu\n"
			"\trkcounter:%lu\n"
			"\trecv_sequence:%lu\n"
			"\twait_sequence:%lu\n"
			"\trtree_segsz:%u\n"
			"\trtree_nseg:%u\n"
			"\trtree_nfree:%u\n"
			"\trtree_nused:%u\n"
			"\tbtn_segsz:%u\n"
			"\tbtn_nseg:%u\n"
			"\tbtn_nfree:%u\n"
			"\tbtn_nused:%u\n"
			"\tbfe_page_counter: %d\n"
			"\tbre_page_counter: %d\n"
			"\tlive_time:%dsec\n",
			nid_msg_resp.um_ip,
			nid_alevel_to_str(nid_msg_resp.um_alevel),
			nid_msg_resp.um_rcounter,
			nid_msg_resp.um_rreadycounter,
			nid_msg_resp.um_r_rcounter,
			nid_msg_resp.um_wcounter,
			nid_msg_resp.um_wreadycounter,
			nid_msg_resp.um_r_wcounter,
			nid_msg_resp.um_kcounter,
			nid_msg_resp.um_r_kcounter,
			nid_msg_resp.um_recv_sequence,
			nid_msg_resp.um_wait_sequence,
			nid_msg_resp.um_rtree_segsz,
			nid_msg_resp.um_rtree_nseg,
			nid_msg_resp.um_rtree_nfree,
			nid_msg_resp.um_rtree_nused,
			nid_msg_resp.um_btn_segsz,
			nid_msg_resp.um_btn_nseg,
			nid_msg_resp.um_btn_nfree,
			nid_msg_resp.um_btn_nused,
			nid_msg_resp.um_bfe_page_counter,
			nid_msg_resp.um_bre_page_counter,
			nid_msg_resp.um_live_time);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_list_stat(struct mgrsac_interface *mgrsac_p, char *chan_uuid)
{
	char *log_header = "mgrsac_list_stat";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_information nid_msg;
	struct umessage_sac_list_stat_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_SAC_CODE_LIST_STAT;
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_LIST_STAT);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);

	printf("%s:\n", nid_msg_resp.um_chan_uuid);
	printf("\treq_counter: %d\n"
		"\tto_req_counter: %d\n"
		"\tread_counter: %d\n"
		"\tto_read_counter: %d\n"
		"\twrite_counter: %d\n"
		"\tto_write_counter: %d\n"
		"\tresp_counter: %d\n"
		"\tto_resp_counter: %d\n"
		"\tdresp_counter: %d\n"
		"\tto_dresp_counter: %d\n",
		nid_msg_resp.um_req_counter,
		nid_msg_resp.um_to_req_counter,
		nid_msg_resp.um_rcounter,
		nid_msg_resp.um_to_rcounter,
		nid_msg_resp.um_wcounter,
		nid_msg_resp.um_to_wcounter,
		nid_msg_resp.um_resp_counter,
		nid_msg_resp.um_to_resp_counter,
		nid_msg_resp.um_dresp_counter,
		nid_msg_resp.um_to_dresp_counter);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_add(struct mgrsac_interface *mgrsac_p, char *chan_uuid, uint8_t do_sync, uint8_t direct_io, uint8_t enable_kill_myself,
		uint32_t alignment, char *ds_name, char *dev_name, char *exportname, uint32_t dev_size, char *tp_name)
{
	char *log_header = "mgrsac_add";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_add nid_msg;
	struct umessage_sac_add_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_ADD;
	msghdr->um_req_code = UMSG_SAC_CODE_ADD;
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	nid_msg.um_sync = do_sync;
	nid_msg.um_direct_io = direct_io;
	nid_msg.um_enable_kill_myself = enable_kill_myself;
	nid_msg.um_alignment = alignment;
	strcpy(nid_msg.um_ds_name, ds_name);
	nid_msg.um_ds_name_len = strlen(ds_name);
	strcpy(nid_msg.um_dev_name, dev_name);
	nid_msg.um_dev_name_len = strlen(dev_name);
	strcpy(nid_msg.um_exportname, exportname);
	nid_msg.um_exportname_len = strlen(exportname);
	nid_msg.um_dev_size = dev_size;
	strcpy(nid_msg.um_tp_name, tp_name);
	nid_msg.um_tp_name_len = strlen(tp_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_ADD);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_ADD);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: add sac successfully\n",log_header);
	else
		printf("%s: add sac failed\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_delete(struct mgrsac_interface *mgrsac_p, char *chan_uuid)
{
	char *log_header = "mgrsac_del";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_delete nid_msg;
	struct umessage_sac_delete_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_DELETE;
	msghdr->um_req_code = UMSG_SAC_CODE_DELETE;
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_DELETE);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_DELETE);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: delete sac successfully\n",log_header);
	else
		printf("%s: delete sac failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_switch(struct mgrsac_interface *mgrsac_p, char *chan_uuid, char *bwc_uuid)
{
	char *log_header = "mgrsac_switch";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_switch nid_msg;
	struct umessage_sac_switch_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_SWITCH;
	msghdr->um_req_code = UMSG_SAC_CODE_SWITCH;
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	strcpy(nid_msg.um_bwc_uuid, bwc_uuid);
	nid_msg.um_bwc_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_SWITCH);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_SWITCH);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: switch sac successfully\n",log_header);
	else
		printf("%s: switch sac failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_set_keepalive(struct mgrsac_interface *mgrsac_p, char *chan_uuid, uint16_t enable_keepalive, uint16_t max_keepalive)
{
	char *log_header = "mgrsac_set_keepalive";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_set_keepalive nid_msg;
	struct umessage_sac_set_keepalive_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096], cmdstr[32];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_SET_KEEPALIVE;
	msghdr->um_req_code = UMSG_SAC_CODE_SET_KEEPALIVE;
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);

	nid_msg.um_enable_keepalive = enable_keepalive;
	nid_msg.um_max_keepalive = max_keepalive;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_SET_KEEPALIVE);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_SET_KEEPALIVE);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);

	if(enable_keepalive) {
		strcpy(cmdstr, "enable");
	} else {
		strcpy(cmdstr, "disable");
	}

	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: %s keepalive sac successfully\n",log_header, cmdstr);
	else
		printf("%s: %s keepalive sac failed\n", log_header, cmdstr);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_fast_release(struct mgrsac_interface *mgrsac_p, char *chan_uuid, uint8_t is_start)
{
	char *log_header = "mgrsac_fast_release";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_fast_release nid_msg;
	struct umessage_sac_fast_release_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_FAST_RELEASE;
	if (is_start)
		msghdr->um_req_code = UMSG_SAC_CODE_START;
	else
		msghdr->um_req_code = UMSG_SAC_CODE_STOP;
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_FAST_RELEASE);
	if (is_start)
		assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_START);
	else
		assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_STOP);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
	if (nid_msg_resp.um_resp_code == 0) {
		if (is_start)
			printf("start fast release successfully\n");
		else
			printf("stop fast release successfully\n");
	}
	else {
		if (is_start)
			printf("fail to start fast release\n");
		else
			printf("fail to stop fast release\n");
	}

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_ioinfo_start(struct mgrsac_interface *mgrsac_p, char *sac_uuid)
{
	char *log_header = "mgrsac_ioinfo_start";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_ioinfo nid_msg;
	struct umessage_sac_hdr *msghdr = (struct umessage_sac_hdr *)&nid_msg;
	struct umessage_sac_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_warning("%s: start (sac_uuid:%s)...", log_header, sac_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_SAC_CMD_IOINFO;
	msghdr->um_req_code = UMSG_SAC_CODE_IOINFO_START;
	strcpy(nid_msg.um_chan_uuid, sac_uuid);
	nid_msg.um_chan_uuid_len = strlen(sac_uuid);

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: sac_uuid:%s, len:%d",
			log_header, sac_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_IOINFO);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_START);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
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
mgrsac_ioinfo_start_all(struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "mgrsac_ioinfo_start_all";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_ioinfo nid_msg;
	struct umessage_sac_hdr *msghdr = (struct umessage_sac_hdr *)&nid_msg;
	struct umessage_sac_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_warning("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_SAC_CMD_IOINFO;
	msghdr->um_req_code = UMSG_SAC_CODE_IOINFO_START_ALL;

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: len:%d",
			log_header, msghdr->um_len);
	write(sfd, buf, len);

	while (1) {
		msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
		if (nread != UMSG_SAC_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_END)
			goto out;

		assert(msghdr->um_req == UMSG_SAC_CMD_IOINFO);
		assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_START_ALL);
		assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
		if (nid_msg_resp.um_is_running) 
			printf("%s:start count successfully!\n", nid_msg_resp.um_chan_uuid);
		else 
			printf("%s:fail to start count!\n", nid_msg_resp.um_chan_uuid);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_ioinfo_stop(struct mgrsac_interface *mgrsac_p, char *sac_uuid)
{
	char *log_header = "mgrsac_ioinfo_stop";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_ioinfo nid_msg;
	struct umessage_sac_hdr *msghdr = (struct umessage_sac_hdr *)&nid_msg;
	struct umessage_sac_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_warning("%s: start (sac_uuid:%s)...", log_header, sac_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_SAC_CMD_IOINFO;
	msghdr->um_req_code = UMSG_SAC_CODE_IOINFO_STOP;
	strcpy(nid_msg.um_chan_uuid, sac_uuid);
	nid_msg.um_chan_uuid_len = strlen(sac_uuid);

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: sac_uuid:%s, len:%d",
			log_header, sac_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_IOINFO);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_STOP);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
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
mgrsac_ioinfo_stop_all(struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "mgrsac_ioinfo_stop_all";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_ioinfo nid_msg;
	struct umessage_sac_hdr *msghdr = (struct umessage_sac_hdr *)&nid_msg;
	struct umessage_sac_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_warning("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_SAC_CMD_IOINFO;
	msghdr->um_req_code = UMSG_SAC_CODE_IOINFO_STOP_ALL;

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: len:%d",
			log_header, msghdr->um_len);
	write(sfd, buf, len);

	while (1) {
		msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
		if (nread != UMSG_SAC_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_END)
			goto out;

		assert(msghdr->um_req == UMSG_SAC_CMD_IOINFO);
		assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_STOP_ALL);
		assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
		if (nid_msg_resp.um_is_running) {
			printf("%s:fail to stop count!\n", nid_msg_resp.um_chan_uuid);
		}
		else {
			printf("chan_uuid:%s\nstatus: stopped\n", nid_msg_resp.um_chan_uuid);
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
mgrsac_ioinfo_check(struct mgrsac_interface *mgrsac_p, char *sac_uuid)
{
	char *log_header = "mgrsac_ioinfo_check";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_ioinfo nid_msg;
	struct umessage_sac_hdr *msghdr = (struct umessage_sac_hdr *)&nid_msg;
	struct umessage_sac_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_warning("%s: start (sac_uuid:%s)...", log_header, sac_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_SAC_CMD_IOINFO;
	msghdr->um_req_code = UMSG_SAC_CODE_IOINFO_CHECK;
	strcpy(nid_msg.um_chan_uuid, sac_uuid);
	nid_msg.um_chan_uuid_len = strlen(sac_uuid);

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: sac_uuid:%s, len:%d",
			log_header, sac_uuid, msghdr->um_len);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_IOINFO);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_CHECK);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
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
mgrsac_ioinfo_check_all(struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "mgrsac_ioinfo_check_all";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_ioinfo nid_msg;
	struct umessage_sac_hdr *msghdr = (struct umessage_sac_hdr *)&nid_msg;
	struct umessage_sac_ioinfo_resp nid_msg_resp;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_warning("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_SAC_CMD_IOINFO;
	msghdr->um_req_code = UMSG_SAC_CODE_IOINFO_CHECK_ALL;

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: len:%d",
			log_header, msghdr->um_len);
	write(sfd, buf, len);

	while (1) {
		msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
		if (nread != UMSG_SAC_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_END)
			goto out;
		assert(msghdr->um_req == UMSG_SAC_CMD_IOINFO);
		assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_IOINFO_CHECK_ALL);
		assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);

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
mgrsac_display(struct mgrsac_interface *mgrsac_p, char *chan_uuid, uint8_t is_setup)
{
	char *log_header = "mgrsac_display";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_display nid_msg;
	struct umessage_sac_display_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_SAC_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_SAC_CODE_W_DISP;
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SAC_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_SAC_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_SAC_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
	printf("sac %s:\n\tsync: %d\n\tdirect_io: %d\n\tenable_kill_myself: %d\n\talignment: %d\n",
		nid_msg_resp.um_chan_uuid, nid_msg_resp.um_sync, nid_msg_resp.um_direct_io, nid_msg_resp.um_enable_kill_myself, nid_msg_resp.um_alignment);
	printf("\ttp_name: %s\n\tds_name: %s\n\tdev_name: %s\n\texport_name: %s\n", nid_msg_resp.um_tp_name, nid_msg_resp.um_ds_name, nid_msg_resp.um_dev_name, nid_msg_resp.um_export_name);
	printf("\tdev_size: %d\n\twc_uuid: %s\n\trc_uuid: %s\n\tio_type: %d\n", nid_msg_resp.um_dev_size, nid_msg_resp.um_wc_uuid, nid_msg_resp.um_rc_uuid, nid_msg_resp.um_io_type);
	printf("\tds_type: %d\n\twc_type: %d\n\trc_type: %d\n\tready: %d\n", nid_msg_resp.um_ds_type, nid_msg_resp.um_wc_type, nid_msg_resp.um_rc_type, nid_msg_resp.um_ready);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_display_all(struct mgrsac_interface *mgrsac_p, uint8_t is_setup)
{
	char *log_header = "mgrsac_display_all";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_display nid_msg;
	struct umessage_sac_display_resp nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_SAC_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_SAC_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
		if (nread != UMSG_SAC_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_SAC_CODE_RESP_END)
			goto out;

		assert(msghdr->um_req == UMSG_SAC_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_SAC_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_SAC_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);

		printf("sac %s:\n\tsync: %d\n\tdirect_io: %d\n\tenable_kill_myself: %d\n\talignment: %d\n",
			nid_msg_resp.um_chan_uuid, nid_msg_resp.um_sync, nid_msg_resp.um_direct_io, nid_msg_resp.um_enable_kill_myself, nid_msg_resp.um_alignment);
		printf("\ttp_name: %s\n\tds_name: %s\n\tdev_name: %s\n\texport_name: %s\n", nid_msg_resp.um_tp_name, nid_msg_resp.um_ds_name, nid_msg_resp.um_dev_name, nid_msg_resp.um_export_name);
		printf("\tdev_size: %d\n\twc_uuid: %s\n\trc_uuid: %s\n\tio_type: %d\n", nid_msg_resp.um_dev_size, nid_msg_resp.um_wc_uuid, nid_msg_resp.um_rc_uuid, nid_msg_resp.um_io_type);
		printf("\tds_type: %d\n\twc_type: %d\n\trc_type: %d\n\tready: %d\n", nid_msg_resp.um_ds_type, nid_msg_resp.um_wc_type, nid_msg_resp.um_rc_type, nid_msg_resp.um_ready);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsac_hello(struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "mgrsac_hello";
	struct mgrsac_private *priv_p = mgrsac_p->sa_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sac_hello nid_msg, nid_msg_resp;
	struct umessage_sac_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_SAC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sac_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SAC_CMD_HELLO;
	msghdr->um_req_code = UMSG_SAC_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sac_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SAC_HEADER_LEN);
	if (nread != UMSG_SAC_HEADER_LEN) {
		printf("module sac is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_SAC_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_SAC_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_SAC_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SAC_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_SAC_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SAC, msghdr);
	printf("module sac is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrsac_operations mgrsac_op = {
	.sa_information = mgrsac_information,
	.sa_information_all = mgrsac_information_all,
	.sa_list_stat = mgrsac_list_stat,
	.sa_add = mgrsac_add,
	.sa_delete = mgrsac_delete,
	.sa_switch_bwc = mgrsac_switch,
	.sa_set_keepalive = mgrsac_set_keepalive,
	.sa_fast_release = mgrsac_fast_release,
	.sa_ioinfo_start = mgrsac_ioinfo_start,
	.sa_ioinfo_start_all = mgrsac_ioinfo_start_all,
	.sa_ioinfo_stop = mgrsac_ioinfo_stop,
	.sa_ioinfo_stop_all = mgrsac_ioinfo_stop_all,
	.sa_ioinfo_check = mgrsac_ioinfo_check,
	.sa_ioinfo_check_all = mgrsac_ioinfo_check_all,
	.sa_display = mgrsac_display,
	.sa_display_all = mgrsac_display_all,
	.sa_hello = mgrsac_hello,
};

int
mgrsac_initialization(struct mgrsac_interface *mgrsac_p, struct mgrsac_setup *setup)
{
	struct mgrsac_private *priv_p;

	nid_log_debug("mgrsac_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrsac_p->sa_private = priv_p;
	mgrsac_p->sa_op = &mgrsac_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
