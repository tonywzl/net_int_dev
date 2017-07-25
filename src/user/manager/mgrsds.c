/*
 * mgrsds.c
 * 	Implementation of Manager to SDS Channel Module
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
#include "umpk_sds_if.h"
#include "mgrsds_if.h"
#include "nid_shared.h"

struct mgrsds_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_SDS;

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
mgrsds_add(struct mgrsds_interface *mgrsds_p,char *sds_name, char *pp_name, char *wc_uuid, char *rc_uuid)
{
	char *log_header = "mgrsds_add";
	struct mgrsds_private *priv_p = mgrsds_p->sds_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sds_add nid_msg;
	struct umessage_sds_add_resp nid_msg_resp;
	struct umessage_sds_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SDS;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sds_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SDS_CMD_ADD;
	msghdr->um_req_code = UMSG_SDS_CODE_ADD;
	strcpy(nid_msg.um_sds_name, sds_name);
	nid_msg.um_sds_name_len = strlen(sds_name);
	strcpy(nid_msg.um_pp_name, pp_name);
	nid_msg.um_pp_name_len = strlen(pp_name);
	if (wc_uuid) {
		strcpy(nid_msg.um_wc_uuid, wc_uuid);
		nid_msg.um_wc_uuid_len = strlen(wc_uuid);

	}
	if (rc_uuid) {
		strcpy(nid_msg.um_rc_uuid, rc_uuid);
		nid_msg.um_rc_uuid_len = strlen(rc_uuid);
	}
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sds_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SDS_HEADER_LEN);
	if (nread != UMSG_SDS_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SDS_CMD_ADD);
	assert(msghdr->um_req_code == UMSG_SDS_CODE_RESP_ADD);
	assert(msghdr->um_len >= UMSG_SDS_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SDS_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SDS, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: add sds successfully\n",log_header);
	else
		printf("%s: add sds failed\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsds_delete(struct mgrsds_interface *mgrsds_p, char *sds_name)
{
	char *log_header = "mgrsds_del";
	struct mgrsds_private *priv_p = mgrsds_p->sds_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sds_delete nid_msg;
	struct umessage_sds_delete_resp nid_msg_resp;
	struct umessage_sds_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SDS;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sds_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SDS_CMD_DELETE;
	msghdr->um_req_code = UMSG_SDS_CODE_DELETE;
	strcpy(nid_msg.um_sds_name, sds_name);
	nid_msg.um_sds_name_len = strlen(sds_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sds_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SDS_HEADER_LEN);
	if (nread != UMSG_SDS_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SDS_CMD_DELETE);
	assert(msghdr->um_req_code == UMSG_SDS_CODE_RESP_DELETE);
	assert(msghdr->um_len >= UMSG_SDS_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SDS_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SDS, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: delete sds successfully\n",log_header);
	else
		printf("%s: delete sds failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsds_display(struct mgrsds_interface *mgrsds_p, char *sds_name, uint8_t is_setup)
{
	char *log_header = "mgrsds_display";
	struct mgrsds_private *priv_p = mgrsds_p->sds_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sds_display nid_msg;
	struct umessage_sds_display_resp nid_msg_resp;
	struct umessage_sds_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SDS;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sds_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SDS_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_SDS_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_SDS_CODE_W_DISP;
	strcpy(nid_msg.um_sds_name, sds_name);
	nid_msg.um_sds_name_len = strlen(sds_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_sds_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SDS_HEADER_LEN);
	if (nread != UMSG_SDS_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_SDS_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_SDS_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_SDS_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_SDS_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SDS_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SDS, msghdr);
	printf("sds %s:\n\tpp_name: %s\n\twc_uuid: %s\n\trc_uuid: %s\n", nid_msg_resp.um_sds_name, nid_msg_resp.um_pp_name, nid_msg_resp.um_wc_uuid, nid_msg_resp.um_rc_uuid);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsds_display_all(struct mgrsds_interface *mgrsds_p, uint8_t is_setup)
{
	char *log_header = "mgrsds_display_all";
	struct mgrsds_private *priv_p = mgrsds_p->sds_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sds_display nid_msg;
	struct umessage_sds_display_resp nid_msg_resp;
	struct umessage_sds_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_SDS;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sds_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SDS_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_SDS_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_SDS_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_sds_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_SDS_HEADER_LEN);
		if (nread != UMSG_SDS_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_SDS_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_SDS_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_SDS_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_SDS_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_SDS_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SDS_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SDS, msghdr);
	printf("sds %s:\n\tpp_name: %s\n\twc_uuid: %s\n\trc_uuid: %s\n", nid_msg_resp.um_sds_name, nid_msg_resp.um_pp_name, nid_msg_resp.um_wc_uuid, nid_msg_resp.um_rc_uuid);

	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrsds_hello(struct mgrsds_interface *mgrsds_p)
{
	char *log_header = "mgrsds_hello";
	struct mgrsds_private *priv_p = mgrsds_p->sds_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_sds_hello nid_msg, nid_msg_resp;
	struct umessage_sds_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_SDS;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_sds_hdr *)&nid_msg;
	msghdr->um_req = UMSG_SDS_CMD_HELLO;
	msghdr->um_req_code = UMSG_SDS_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_sds_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_SDS_HEADER_LEN);
	if (nread != UMSG_SDS_HEADER_LEN) {
		printf("module sds is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_SDS_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_SDS_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_SDS_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_SDS_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_SDS_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_SDS, msghdr);
	printf("module sds is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrsds_operations mgrsds_op = {
	.sds_add = mgrsds_add,
	.sds_delete = mgrsds_delete,
	.sds_display = mgrsds_display,
	.sds_display_all = mgrsds_display_all,
	.sds_hello = mgrsds_hello,
};

int
mgrsds_initialization(struct mgrsds_interface *mgrsds_p, struct mgrsds_setup *setup)
{
	struct mgrsds_private *priv_p;

	nid_log_debug("mgrsds_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrsds_p->sds_private = priv_p;
	mgrsds_p->sds_op = &mgrsds_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
