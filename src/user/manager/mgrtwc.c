/*
 * mgrwc.c
 * 	Implementation of Manager to TWC Channel Module
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
#include "umpk_twc_if.h"
#include "mgrtwc_if.h"
#include "nid_shared.h"
#include "manager.h"

struct mgrtwc_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_TWC;

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
mgrtwc_throughput_stat_reset(struct mgrtwc_interface *mgrtwc_p, char *chan_uuid)
{
	char *log_header = "mgrtwc_throughput_stat_reset";
	struct mgrtwc_private *priv_p = mgrtwc_p->tw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_twc_throughput nid_msg;
	struct umessage_twc_hdr *msghdr = (struct umessage_twc_hdr *)&nid_msg;
	char buf[1024];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_TWC;

	nid_log_warning("%s: start (uuid:%s)...", log_header, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, chan_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_TWC_CMD_THROUGHPUT;

	msghdr->um_req_code = UMSG_TWC_CODE_THROUGHPUT_RESET;

	strcpy(nid_msg.um_twc_uuid, chan_uuid);
	nid_msg.um_twc_uuid_len = strlen(chan_uuid);

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
mgrtwc_information_flushing(struct mgrtwc_interface *mgrtwc_p, char *c_uuid)
{
	char *log_header = "mgrtwc_information_flushing";
	struct mgrtwc_private *priv_p = mgrtwc_p->tw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_twc_information nid_msg;
	struct umessage_twc_hdr *msghdr = (struct umessage_twc_hdr *)&nid_msg;
	char buf[4096];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_TWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_TWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_TWC_CODE_FLUSHING;
	strcpy(nid_msg.um_twc_uuid, c_uuid);
	nid_msg.um_twc_uuid_len = strlen(c_uuid);
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
mgrtwc_information_stat(struct mgrtwc_interface *mgrtwc_p, char *c_uuid)
{
	char *log_header = "mgrtwc_information_stat";
	struct mgrtwc_private *priv_p = mgrtwc_p->tw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_twc_information nid_msg;
	struct umessage_twc_information_resp_stat nid_msg_resp;
	struct umessage_twc_hdr *msghdr; 
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_twc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_TWC_CODE_STAT;
	strcpy(nid_msg.um_twc_uuid, c_uuid);
	nid_msg.um_twc_uuid_len = strlen(c_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_twc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TWC_HEADER_LEN);
	if (nread != UMSG_TWC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	printf("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_TWC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_TWC_CODE_RESP_STAT);
	assert(msghdr->um_len >= UMSG_TWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TWC, msghdr);
	printf(	"%s stat:\n"
		first_intend"seq_assigned:%lu\n"
		first_intend"seq_flushed:%lu\n",
	        nid_msg_resp.um_twc_uuid,
		nid_msg_resp.um_seq_assigned,
		nid_msg_resp.um_seq_flushed);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}


static int
mgrtwc_information_throughput_stat(struct mgrtwc_interface *mgrtwc_p, char *c_uuid)
{
	char *log_header = "mgrtwc_information_throughput_stat";
	struct mgrtwc_private *priv_p = mgrtwc_p->tw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_twc_information nid_msg;
	struct umessage_twc_information_throughput_stat nid_msg_resp;
	struct umessage_twc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_twc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_TWC_CODE_THROUGHPUT_STAT;
	strcpy(nid_msg.um_twc_uuid, c_uuid);
	nid_msg.um_twc_uuid_len = strlen(c_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_twc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TWC_HEADER_LEN);
	if (nread != UMSG_TWC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_TWC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_TWC_CODE_THROUGHPUT_STAT_RSLT);
	assert(msghdr->um_len >= UMSG_TWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TWC, msghdr);
	printf(	"%s twc throughput stat:\n"
		first_intend"data flushed:%lu\n"
		first_intend"data package flushed:%lu\n"
		first_intend"nondata package flushed:%lu\n",

        nid_msg_resp.um_twc_uuid,
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
mgrtwc_information_rw_stat(struct mgrtwc_interface *mgrtwc_p, char *c_uuid, char *ch_uuid)
{
	char *log_header = "mgrtwc_information_rw_stat";
	struct mgrtwc_private *priv_p = mgrtwc_p->tw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_twc_information nid_msg;
	struct umessage_twc_information_rw_stat nid_msg_resp;
	struct umessage_twc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_twc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TWC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_TWC_CODE_RW_STAT;
	strcpy(nid_msg.um_twc_uuid, c_uuid);
	nid_msg.um_twc_uuid_len = strlen(c_uuid);
	strcpy(nid_msg.um_chan_uuid, ch_uuid);
	nid_msg.um_chan_uuid_len = strlen(ch_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_twc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TWC_HEADER_LEN);
	if (nread != UMSG_TWC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_TWC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_TWC_CODE_RW_STAT_RSLT);
	assert(msghdr->um_len >= UMSG_TWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TWC, msghdr);
	if (nid_msg_resp.res){
		printf(	"%s twc rw stat:\n"
			first_intend"data write:%lu\n"
			first_intend"data read:%lu\n",
			nid_msg_resp.um_twc_uuid,
			nid_msg_resp.um_chan_data_write,
			nid_msg_resp.um_chan_data_read);
	} else {
		printf("wrong twc uuid or channel uuid\n");
	}

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrtwc_display(struct mgrtwc_interface *mgrtwc_p, char *twc_uuid, uint8_t is_setup)
{
	char *log_header = "mgrtwc_display";
	struct mgrtwc_private *priv_p = mgrtwc_p->tw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_twc_display nid_msg;
	struct umessage_twc_display_resp nid_msg_resp;
	struct umessage_twc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_twc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TWC_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_TWC_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_TWC_CODE_W_DISP;
	strcpy(nid_msg.um_twc_uuid, twc_uuid);
	nid_msg.um_twc_uuid_len = strlen(twc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_twc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TWC_HEADER_LEN);
	if (nread != UMSG_TWC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_TWC_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_TWC_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_TWC_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_TWC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TWC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TWC, msghdr);

	printf("twc %s:\n\ttp_name: %s\n\tdo_fp: %d\n", nid_msg_resp.um_twc_uuid, nid_msg_resp.um_tp_name, nid_msg_resp.um_do_fp);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrtwc_display_all(struct mgrtwc_interface *mgrtwc_p, uint8_t is_setup)
{
	char *log_header = "mgrtwc_display_all";
	struct mgrtwc_private *priv_p = mgrtwc_p->tw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_twc_display nid_msg;
	struct umessage_twc_display_resp nid_msg_resp;
	struct umessage_twc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_twc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TWC_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_TWC_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_TWC_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_twc_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_TWC_HEADER_LEN);
		if (nread != UMSG_TWC_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_TWC_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_TWC_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_TWC_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_TWC_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_TWC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TWC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TWC, msghdr);

		printf("twc %s:\n\ttp_name: %s\n\tdo_fp: %d\n", nid_msg_resp.um_twc_uuid, nid_msg_resp.um_tp_name, nid_msg_resp.um_do_fp);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrtwc_hello(struct mgrtwc_interface *mgrtwc_p)
{
	char *log_header = "mgrtwc_hello";
	struct mgrtwc_private *priv_p = mgrtwc_p->tw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_twc_hello nid_msg, nid_msg_resp;
	struct umessage_twc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_TWC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_twc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TWC_CMD_HELLO;
	msghdr->um_req_code = UMSG_TWC_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_twc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TWC_HEADER_LEN);
	if (nread != UMSG_TWC_HEADER_LEN) {
		printf("module twc is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_TWC_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_TWC_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_TWC_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TWC_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_TWC_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TWC, msghdr);
	printf("module twc is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrtwc_operations mgrtwc_op = {
	.tw_information_flushing = mgrtwc_information_flushing,
	.tw_information_stat = mgrtwc_information_stat,
	.tw_throughput_stat_reset= mgrtwc_throughput_stat_reset,
	.tw_information_throughput_stat = mgrtwc_information_throughput_stat,
	.tw_information_rw_stat = mgrtwc_information_rw_stat,
	.tw_display = mgrtwc_display,
	.tw_display_all = mgrtwc_display_all,
	.tw_hello = mgrtwc_hello,
};

int
mgrtwc_initialization(struct mgrtwc_interface *mgrtwc_p, struct mgrtwc_setup *setup)
{
	struct mgrtwc_private *priv_p;

	nid_log_debug("mgrtwc_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrtwc_p->tw_private = priv_p;
	mgrtwc_p->tw_op = &mgrtwc_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
