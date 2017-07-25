/*
 * mgrwc.c
 * 	Implementation of Manager to WC Channel Module
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
#include "umpk_wc_if.h"
#include "mgrwc_if.h"
#include "nid_shared.h"
#include "manager.h"

struct mgrwc_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_WC;

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
mgrwc_information_flushing(struct mgrwc_interface *mgrwc_p, char *c_uuid)
{
	char *log_header = "mgrwc_information_flushing";
	struct mgrwc_private *priv_p = mgrwc_p->wc_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_wc_information nid_msg;
	struct umessage_wc_hdr *msghdr = (struct umessage_wc_hdr *)&nid_msg;
	char buf[4096];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_WC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_WC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_WC_CODE_FLUSHING;
	strcpy(nid_msg.um_uuid, c_uuid);
	nid_msg.um_uuid_len = strlen(c_uuid);
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
mgrwc_information_stat(struct mgrwc_interface *mgrwc_p, char *c_uuid)
{
	char *log_header = "mgrwc_information_stat";
	struct mgrwc_private *priv_p = mgrwc_p->wc_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_wc_information nid_msg;
	struct umessage_wc_information_resp_stat nid_msg_resp;
	struct umessage_wc_hdr *msghdr; 
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_WC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_wc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_WC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_WC_CODE_STAT;
	strcpy(nid_msg.um_uuid, c_uuid);
	nid_msg.um_uuid_len = strlen(c_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_wc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_WC_HEADER_LEN);
	if (nread != UMSG_WC_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_WC_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_WC_CODE_RESP_STAT);
	assert(msghdr->um_len >= UMSG_WC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_WC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_WC, msghdr);
	printf(	"%s stat:\n"
		first_intend"block_occupied:%u/%u\n"
		first_intend"cur_write_block:%u\n"
		first_intend"wc_cur_flush_block:%u\n"
		first_intend"seq_assigned:%lu\n"
		first_intend"seq_flushed:%lu\n",
	        nid_msg_resp.wc_uuid,
		nid_msg_resp.wc_block_occupied,
		nid_msg_resp.wc_flush_nblocks,
		nid_msg_resp.wc_cur_write_block,
		nid_msg_resp.wc_cur_flush_block,
		nid_msg_resp.wc_seq_assigned,
		nid_msg_resp.wc_seq_flushed);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrwc_hello(struct mgrwc_interface *mgrwc_p)
{
	char *log_header = "mgrwc_hello";
	struct mgrwc_private *priv_p = mgrwc_p->wc_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_wc_hello nid_msg, nid_msg_resp;
	struct umessage_wc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_WC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_wc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_WC_CMD_HELLO;
	msghdr->um_req_code = UMSG_WC_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_wc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_WC_HEADER_LEN);
	if (nread != UMSG_WC_HEADER_LEN) {
		printf("module wc is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_WC_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_WC_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_WC_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_WC_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_WC_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_WC, msghdr);
	printf("module wc is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrwc_operations mgrwc_op = {
	.wc_information_flushing = mgrwc_information_flushing,
	.wc_information_stat = mgrwc_information_stat,
	.wc_hello = mgrwc_hello,
};

int
mgrwc_initialization(struct mgrwc_interface *mgrwc_p, struct mgrwc_setup *setup)
{
	struct mgrwc_private *priv_p;

	nid_log_debug("mgrwc_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrwc_p->wc_private = priv_p;
	mgrwc_p->wc_op = &mgrwc_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
