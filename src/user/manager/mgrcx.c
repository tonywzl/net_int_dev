/*
 * mgrcx.c
 * 	Implementation of Manager to CX Channel Module
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
#include "umpk_cxa_if.h"
#include "mgrcx_if.h"
#include "nid_shared.h"

struct mgrcx_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_CXA;

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
mgrcx_display(struct mgrcx_interface *mgrcx_p, char *cxa_name, uint8_t is_setup)
{
	char *log_header = "mgrcx_display";
	struct mgrcx_private *priv_p = mgrcx_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_cxa_display nid_msg;
	struct umessage_cxa_display_resp nid_msg_resp;
	struct umessage_tx_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_CXA;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tx_hdr *)&nid_msg;
	msghdr->um_req = UMSG_CXA_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_CXA_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_CXA_CODE_W_DISP;
	strcpy(nid_msg.um_chan_cxa_uuid, cxa_name);
	nid_msg.um_chan_cxa_uuid_len = strlen(cxa_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	util_nw_write_n(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_tx_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TX_HEADER_LEN);
	if (nread != UMSG_TX_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	len = nread;
	p = decode_tx_hdr(p , &len, msghdr);
	nid_log_debug("%s: req:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_len);

	assert(msghdr->um_req == UMSG_CXA_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_CXA_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_CXA_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TX_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_CXA, msghdr);

	printf("cxa %s:\n\tpeer_uuid: %s\n\tip: %s\n\tcxt_name: %s\n", nid_msg_resp.um_chan_cxa_uuid, nid_msg_resp.um_peer_uuid, nid_msg_resp.um_ip, nid_msg_resp.um_cxt_name);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcx_display_all(struct mgrcx_interface *mgrcx_p, uint8_t is_setup)
{
	char *log_header = "mgrcx_display_all";
	struct mgrcx_private *priv_p = mgrcx_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_cxa_display nid_msg;
	struct umessage_cxa_display_resp nid_msg_resp;
	struct umessage_tx_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_CXA;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tx_hdr *)&nid_msg;
	msghdr->um_req = UMSG_CXA_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_CXA_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_CXA_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	util_nw_write_n(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_tx_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_TX_HEADER_LEN);
		if (nread != UMSG_TX_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		len = nread;
		p = decode_tx_hdr(p , &len, msghdr);
		nid_log_debug("%s: req:%d, resp_len:%d, done\n",
			log_header, msghdr->um_req, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_CXA_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_CXA_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_CXA_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_CXA_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TX_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_CXA, msghdr);

		printf("cxa %s:\n\tpeer_uuid: %s\n\tip: %s\n\tcxt_name: %s\n", nid_msg_resp.um_chan_cxa_uuid, nid_msg_resp.um_peer_uuid, nid_msg_resp.um_ip, nid_msg_resp.um_cxt_name);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcx_hello(struct mgrcx_interface *mgrcx_p)
{
	char *log_header = "mgrcx_hello";
	struct mgrcx_private *priv_p = mgrcx_p->cx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_cxa_hello nid_msg, nid_msg_resp;
	struct umessage_tx_hdr *msghdr;
	char  *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int  rc = 0, ctype = NID_CTYPE_CXA;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tx_hdr *)&nid_msg;
	msghdr->um_req = UMSG_CXA_CMD_HELLO;
	msghdr->um_req_code = UMSG_CXA_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	util_nw_write_n(sfd, buf, len);

	msghdr = (struct umessage_tx_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TX_HEADER_LEN);
	if (nread != UMSG_TX_HEADER_LEN) {
		printf("module cx is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	len = nread;
	p = decode_tx_hdr(p , &len, msghdr);
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_CXA_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_CXA_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TX_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_TX_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_CXA, msghdr);
	printf("module cx is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrcx_operations mgrcx_op = {
	.cx_display = mgrcx_display,
	.cx_display_all = mgrcx_display_all,
	.cx_hello = mgrcx_hello,
};

int
mgrcx_initialization(struct mgrcx_interface *mgrcx_p, struct mgrcx_setup *setup)
{
	struct mgrcx_private *priv_p;

	nid_log_debug("mgrcx_initialization start ...");
	priv_p = calloc(1, sizeof(*priv_p));
	mgrcx_p->cx_private = priv_p;
	mgrcx_p->cx_op = &mgrcx_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
