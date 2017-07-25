/*
 * mgrpp.c
 * 	Implementation of Manager to PP Channel Module
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
#include "umpk_pp_if.h"
#include "mgrpp_if.h"
#include "nid_shared.h"
#include "manager.h"

struct mgrpp_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_PP;

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
mgrpp_stat(struct mgrpp_interface *mgrpp_p, char *pp_name)
{
	char *log_header = "mgrpp_stat";
	struct mgrpp_private *priv_p = mgrpp_p->pp_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_pp_stat nid_msg;
	struct umessage_pp_stat_resp nid_msg_resp;
	struct umessage_pp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_PP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_pp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_PP_CMD_STAT;
	msghdr->um_req_code = UMSG_PP_CODE_STAT;
	strcpy(nid_msg.um_pp_name, pp_name);
	nid_msg.um_pp_name_len = strlen(pp_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_pp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_PP_HEADER_LEN);
	if (nread != UMSG_PP_HEADER_LEN) {
		rc = 1;	//pp not found
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_PP_CMD_STAT);
	assert(msghdr->um_req_code == UMSG_PP_CODE_RESP_STAT);
	assert(msghdr->um_len >= UMSG_PP_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_PP_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_PP, msghdr);
	printf("pp %s stat:\n"first_intend"pool size: %d\n"first_intend"page size: %d\n"first_intend"pool len: %d\n"first_intend"nfree: %d\n",
                        nid_msg_resp.um_pp_name, nid_msg_resp.um_pp_poolsz, nid_msg_resp.um_pp_pagesz, nid_msg_resp.um_pp_poollen, nid_msg_resp.um_pp_nfree);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrpp_stat_all(struct mgrpp_interface *mgrpp_p)
{
	char *log_header = "mgrpp_stat_all";
	struct mgrpp_private *priv_p = mgrpp_p->pp_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_pp_stat nid_msg;
	struct umessage_pp_stat_resp nid_msg_resp;
	struct umessage_pp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_PP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_pp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_PP_CMD_STAT;
	msghdr->um_req_code = UMSG_PP_CODE_STAT_ALL;
	strcpy(nid_msg.um_pp_name, "NULL");
	nid_msg.um_pp_name_len = strlen(nid_msg.um_pp_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while (1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_pp_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_PP_HEADER_LEN);
		if (nread != UMSG_PP_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_PP_CODE_RESP_END) {
			goto out;
		}

		assert(msghdr->um_req == UMSG_PP_CMD_STAT);
		assert(msghdr->um_req_code == UMSG_PP_CODE_RESP_STAT_ALL);
		assert(msghdr->um_len >= UMSG_PP_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_PP_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_PP, msghdr);
		printf("pp %s stat:\n"first_intend"pool size: %d\n"first_intend"page size: %d\n"first_intend"pool len: %d\n"first_intend"nfree: %d\n",
       	                 nid_msg_resp.um_pp_name, nid_msg_resp.um_pp_poolsz, nid_msg_resp.um_pp_pagesz, nid_msg_resp.um_pp_poollen, nid_msg_resp.um_pp_nfree);
	}

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

static int
mgrpp_add(struct mgrpp_interface *mgrpp_p, char *pp_name, uint8_t set_id, uint32_t pool_size, uint32_t page_size)
{
	char *log_header = "mgrpp_add";
	struct mgrpp_private *priv_p = mgrpp_p->pp_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_pp_add nid_msg;
	struct umessage_pp_add_resp nid_msg_resp;
	struct umessage_pp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_PP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_pp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_PP_CMD_ADD;
	msghdr->um_req_code = UMSG_PP_CODE_ADD;
	strcpy(nid_msg.um_pp_name, pp_name);
	nid_msg.um_pp_name_len = strlen(pp_name);
	strcpy(nid_msg.um_pp_name, pp_name);
	nid_msg.um_pp_name_len = strlen(pp_name);
	nid_msg.um_set_id = set_id;
	nid_msg.um_pool_size = pool_size;
	nid_msg.um_page_size = page_size;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_pp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_PP_HEADER_LEN);
	if (nread != UMSG_PP_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_PP_CMD_ADD);
	assert(msghdr->um_req_code == UMSG_PP_CODE_RESP_ADD);
	assert(msghdr->um_len >= UMSG_PP_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_PP_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_PP, msghdr);
	nid_log_warning("%s: the resp (%d)", log_header, nid_msg_resp.um_resp_code);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: add pp successfully\n",log_header);
	else
		printf("%s: add pp failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrpp_delete(struct mgrpp_interface *mgrpp_p, char *pp_name)
{
	char *log_header = "mgrpp_del";
	struct mgrpp_private *priv_p = mgrpp_p->pp_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_pp_delete nid_msg;
	struct umessage_pp_delete_resp nid_msg_resp;
	struct umessage_pp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_PP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_pp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_PP_CMD_DELETE;
	msghdr->um_req_code = UMSG_PP_CODE_DELETE;
	strcpy(nid_msg.um_pp_name, pp_name);
	nid_msg.um_pp_name_len = strlen(pp_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_pp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_PP_HEADER_LEN);
	if (nread != UMSG_PP_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_PP_CMD_DELETE);
	assert(msghdr->um_req_code == UMSG_PP_CODE_RESP_DELETE);
	assert(msghdr->um_len >= UMSG_PP_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_PP_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_PP, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: delete pp successfully\n",log_header);
	else
		printf("%s: delete pp failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrpp_display(struct mgrpp_interface *mgrpp_p, char *pp_name, uint8_t is_setup)
{
	char *log_header = "mgrpp_display";
	struct mgrpp_private *priv_p = mgrpp_p->pp_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_pp_display nid_msg;
	struct umessage_pp_display_resp nid_msg_resp;
	struct umessage_pp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_PP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_pp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_PP_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_PP_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_PP_CODE_W_DISP;
	strcpy(nid_msg.um_pp_name, pp_name);
	nid_msg.um_pp_name_len = strlen(pp_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_pp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_PP_HEADER_LEN);
	if (nread != UMSG_PP_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_PP_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_PP_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_PP_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_PP_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_PP_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_PP, msghdr);

	printf("pp %s:\n\tpool_size: %d\n\tpage_size: %d\n\tset_id: %d\n", nid_msg_resp.um_pp_name, nid_msg_resp.um_pool_size, nid_msg_resp.um_page_size, nid_msg_resp.um_set_id);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrpp_display_all(struct mgrpp_interface *mgrpp_p, uint8_t is_setup)
{
	char *log_header = "mgrpp_display_all";
	struct mgrpp_private *priv_p = mgrpp_p->pp_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_pp_display nid_msg;
	struct umessage_pp_display_resp nid_msg_resp;
	struct umessage_pp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_PP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_pp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_PP_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_PP_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_PP_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_pp_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_PP_HEADER_LEN);
		if (nread != UMSG_PP_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_PP_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_PP_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_PP_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_PP_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_PP_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_PP_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_PP, msghdr);

		printf("pp %s:\n\tpool_size: %d\n\tpage_size: %d\n\tset_id: %d\n", nid_msg_resp.um_pp_name, nid_msg_resp.um_pool_size, nid_msg_resp.um_page_size, nid_msg_resp.um_set_id);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrpp_hello(struct mgrpp_interface *mgrpp_p)
{
	char *log_header = "mgrpp_hello";
	struct mgrpp_private *priv_p = mgrpp_p->pp_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_pp_hello nid_msg, nid_msg_resp;
	struct umessage_pp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_PP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_pp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_PP_CMD_HELLO;
	msghdr->um_req_code = UMSG_PP_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_pp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_PP_HEADER_LEN);
	if (nread != UMSG_PP_HEADER_LEN) {
		printf("module pp is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_PP_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_PP_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_PP_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_PP_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_PP_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_PP, msghdr);
	printf("module pp is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrpp_operations mgrpp_op = {
	.pp_stat = mgrpp_stat,
	.pp_stat_all = mgrpp_stat_all,
	.pp_add = mgrpp_add,
	.pp_delete = mgrpp_delete,
	.pp_display = mgrpp_display,
	.pp_display_all = mgrpp_display_all,
	.pp_hello = mgrpp_hello,
};

int
mgrpp_initialization(struct mgrpp_interface *mgrpp_p, struct mgrpp_setup *setup)
{
	struct mgrpp_private *priv_p;

	nid_log_debug("mgrpp_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrpp_p->pp_private = priv_p;
	mgrpp_p->pp_op = &mgrpp_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
