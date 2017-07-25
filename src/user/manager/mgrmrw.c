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
#include "umpk_mrw_if.h"
#include "mgrmrw_if.h"
#include "nid_shared.h"
#include "manager.h"

struct mgrmrw_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_MRW;

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
mgrmrw_information_stat(struct mgrmrw_interface *mgrmrw_p, char *mrw_name)
{
	char *log_header = "mgrmrw_information_stat";
	struct mgrmrw_private *priv_p = mgrmrw_p->mr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_mrw_information nid_msg;
	struct umessage_mrw_information_stat_resp nid_msg_resp;
	struct umessage_mrw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_MRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_mrw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_MRW_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_MRW_CODE_STAT;
	strcpy(nid_msg.um_mrw_name, mrw_name);
	nid_msg.um_mrw_name_len = strlen(mrw_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_mrw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_MRW_HEADER_LEN);
	if (nread != UMSG_MRW_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_warning("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_MRW_CMD_INFORMATION);
	assert(msghdr->um_req_code == UMSG_MRW_CODE_RESP_STAT);
	assert(msghdr->um_len >= UMSG_MRW_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_MRW_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_MRW, msghdr);
	if (nid_msg_resp.um_seq_mrw == 1) {
		printf(	"MRW stat:\n"
			first_intend"write operation num:%lu\n"
			first_intend"write fingerprint num:%lu\n",
			nid_msg_resp.um_seq_wop_num,
			nid_msg_resp.um_seq_wfp_num);
	} else {
		printf( " nidserver do not in mrw type mode \n");
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrmrw_add(struct mgrmrw_interface *mgrmrw_p,char *mrw_name)
{
	char *log_header = "mgrmrw_add";
	struct mgrmrw_private *priv_p = mgrmrw_p->mr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_mrw_add nid_msg;
	struct umessage_mrw_add_resp nid_msg_resp;
	struct umessage_mrw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_MRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_mrw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_MRW_CMD_ADD;
	msghdr->um_req_code = UMSG_MRW_CODE_ADD;
	strcpy(nid_msg.um_mrw_name, mrw_name);
	nid_msg.um_mrw_name_len = strlen(mrw_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_mrw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_MRW_HEADER_LEN);
	if (nread != UMSG_MRW_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_MRW_CMD_ADD);
	assert(msghdr->um_req_code == UMSG_MRW_CODE_RESP_ADD);
	assert(msghdr->um_len >= UMSG_MRW_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_MRW_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_MRW, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: add mrw successfully\n",log_header);
	else
		printf("%s: add mrw failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrmrw_display(struct mgrmrw_interface *mgrmrw_p, char *mrw_name, uint8_t is_setup)
{
	char *log_header = "mgrmrw_display";
	struct mgrmrw_private *priv_p = mgrmrw_p->mr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_mrw_display nid_msg;
	struct umessage_mrw_display nid_msg_resp;
	struct umessage_mrw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_MRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_mrw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_MRW_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_MRW_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_MRW_CODE_W_DISP;
	strcpy(nid_msg.um_mrw_name, mrw_name);
	nid_msg.um_mrw_name_len = strlen(mrw_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_mrw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_MRW_HEADER_LEN);
	if (nread != UMSG_MRW_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_MRW_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_MRW_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_MRW_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_MRW_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_MRW_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_MRW, msghdr);
	printf("mrw: %s\n", nid_msg_resp.um_mrw_name);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrmrw_display_all(struct mgrmrw_interface *mgrmrw_p, uint8_t is_setup)
{
	char *log_header = "mgrmrw_display_all";
	struct mgrmrw_private *priv_p = mgrmrw_p->mr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_mrw_display nid_msg;
	struct umessage_mrw_display nid_msg_resp;
	struct umessage_mrw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_MRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_mrw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_MRW_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_MRW_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_MRW_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_mrw_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_MRW_HEADER_LEN);
		if (nread != UMSG_MRW_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_MRW_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_MRW_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_MRW_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_MRW_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_MRW_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_MRW_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_MRW, msghdr);

		printf("mrw: %s\n", nid_msg_resp.um_mrw_name);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrmrw_hello(struct mgrmrw_interface *mgrmrw_p)
{
	char *log_header = "mgrmrw_hello";
	struct mgrmrw_private *priv_p = mgrmrw_p->mr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_mrw_hello nid_msg, nid_msg_resp;
	struct umessage_mrw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_MRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_mrw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_MRW_CMD_HELLO;
	msghdr->um_req_code = UMSG_MRW_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_mrw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_MRW_HEADER_LEN);
	if (nread != UMSG_MRW_HEADER_LEN) {
		printf("module mrw is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_MRW_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_MRW_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_MRW_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_MRW_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_MRW_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_MRW, msghdr);
	printf("module mrw is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrmrw_operations mgrmrw_op = {
	.mr_information_stat = mgrmrw_information_stat,
	.mr_add = mgrmrw_add,
	.mr_display = mgrmrw_display,
	.mr_display_all = mgrmrw_display_all,
	.mr_hello = mgrmrw_hello,
};

int
mgrmrw_initialization(struct mgrmrw_interface *mgrmrw_p, struct mgrmrw_setup *setup)
{
	struct mgrmrw_private *priv_p;

	nid_log_debug("mgrmrw_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrmrw_p->mr_private = priv_p;
	mgrmrw_p->mr_op = &mgrmrw_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
