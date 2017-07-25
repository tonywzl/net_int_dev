/*
 * mgrdx.c
 * 	Implementation of Manager to DX Channel Module
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
#include "umpk_dxa_if.h"
#include "mgrdx_if.h"
#include "nid_shared.h"
#include "tx_shared.h"

struct mgrdx_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_DXA;

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
mgrdx_display(struct mgrdx_interface *mgrdx_p, char *dxa_name, uint8_t is_setup)
{
	char *log_header = "mgrdx_display";
	struct mgrdx_private *priv_p = mgrdx_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_dxa_display nid_msg;
	struct umessage_dxa_display_resp nid_msg_resp;
	struct umessage_tx_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_DXA;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tx_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DXA_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_DXA_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_DXA_CODE_W_DISP;
	strcpy(nid_msg.um_chan_dxa_uuid, dxa_name);
	nid_msg.um_chan_dxa_uuid_len = strlen(dxa_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

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

	assert(msghdr->um_req == UMSG_DXA_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_DXA_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_DXA_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TX_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DXA, msghdr);

	printf("dxa %s:\n\tpeer_uuid: %s\n\tip: %s\n\tdxt_name: %s\n", nid_msg_resp.um_chan_dxa_uuid, nid_msg_resp.um_peer_uuid, nid_msg_resp.um_ip, nid_msg_resp.um_dxt_name);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrdx_display_all(struct mgrdx_interface *mgrdx_p, uint8_t is_setup)
{
	char *log_header = "mgrdx_display_all";
	struct mgrdx_private *priv_p = mgrdx_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_dxa_display nid_msg;
	struct umessage_dxa_display_resp nid_msg_resp;
	struct umessage_tx_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_DXA;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tx_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DXA_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_DXA_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_DXA_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

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

		if (msghdr->um_req_code == UMSG_DXA_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_DXA_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_DXA_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_DXA_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TX_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DXA, msghdr);

		printf("dxa %s:\n\tpeer_uuid: %s\n\tip: %s\n\tdxt_name: %s\n", nid_msg_resp.um_chan_dxa_uuid, nid_msg_resp.um_peer_uuid, nid_msg_resp.um_ip, nid_msg_resp.um_dxt_name);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrdx_hello(struct mgrdx_interface *mgrdx_p)
{
	char *log_header = "mgrdx_hello";
	struct mgrdx_private *priv_p = mgrdx_p->dx_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_dxa_hello nid_msg, nid_msg_resp;
	struct umessage_tx_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_DXA;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tx_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DXA_CMD_HELLO;
	msghdr->um_req_code = UMSG_DXA_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_tx_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TX_HEADER_LEN);
	if (nread != UMSG_TX_HEADER_LEN) {
		printf("module dx is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	p = decode_tx_hdr(p , &len, msghdr);	
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_DXA_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_DXA_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_TX_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TX_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_TX_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DXA, msghdr);
	printf("module dx is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrdx_operations mgrdx_op = {
	.dx_display = mgrdx_display,
	.dx_display_all = mgrdx_display_all,
	.dx_hello = mgrdx_hello,
};

int
mgrdx_initialization(struct mgrdx_interface *mgrdx_p, struct mgrdx_setup *setup)
{
	struct mgrdx_private *priv_p;

	nid_log_debug("mgrdx_initialization start ...");
	priv_p = calloc(1, sizeof(*priv_p));
	mgrdx_p->dx_private = priv_p;
	mgrdx_p->dx_op = &mgrdx_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
