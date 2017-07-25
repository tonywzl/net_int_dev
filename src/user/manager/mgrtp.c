/*
 * mgrtp.c
 *	Implementation of Manager to TP Channel Module
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
#include "umpk_tp_if.h"
#include "mgrtp_if.h"
#include "nid_shared.h"
#include "manager.h"

struct mgrtp_private {
	char			p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_TP;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);
	sfd = util_nw_make_connection(ipstr, port);

	if (sfd < 0) {
		nid_log_error("cannot connect to the client (%s:%d), errno:%d", ipstr, port, errno);
	} else {
		if (util_nw_write_two_byte(sfd, chan_type) != NID_SIZE_CTYPE_MSG){
			nid_log_error("%s: failed to send chan_type, errno:%d", log_header, errno);
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

static int
mgrtp_information_stat(struct mgrtp_interface *mgrtp_p, char *t_uuid)
{
	char *log_header = "mgrtp_information_stat";
	struct mgrtp_private *priv_p = mgrtp_p->t_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_tp_information nid_msg;
	struct umessage_tp_information_resp_stat nid_msg_resp;
	struct umessage_tp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_TP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TP_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_TP_CODE_STAT;
	strcpy(nid_msg.um_uuid, t_uuid);
	nid_msg.um_uuid_len = strlen(t_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_tp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TP_HEADER_LEN);
	if (nread != UMSG_TP_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);
	if (msghdr->um_req_code == UMSG_TP_CODE_RESP_STAT) {
		assert(msghdr->um_req == UMSG_TP_CMD_INFORMATION);
		assert(msghdr->um_req_code == UMSG_TP_CODE_RESP_STAT);
		assert(msghdr->um_len >= UMSG_TP_HEADER_LEN);
	
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TP_HEADER_LEN);
		if (nread < (msghdr->um_len - UMSG_TP_HEADER_LEN)) {
			rc = -1;
			goto out;
		}
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TP, msghdr);
		printf("tp %s stat:\n"first_intend"nused: %d\n"first_intend"nfree: %d\n"first_intend"workers: %d\n"first_intend"max_wokers: %d\n"first_intend"no_free: %d\n",
			nid_msg_resp.um_uuid, nid_msg_resp.um_nused, nid_msg_resp.um_nfree, nid_msg_resp.um_workers, nid_msg_resp.um_max_workers, nid_msg_resp.um_no_free);
	} else if (msghdr->um_req_code == UMSG_TP_CODE_RESP_NOT_FOUND) {
		printf("tp %s does not exist.\n", t_uuid);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrtp_information_all(struct mgrtp_interface *mgrtp_p)
{
	char *log_header = "mgrtp_information_all";
	struct mgrtp_private *priv_p = mgrtp_p->t_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_tp_information nid_msg;
	struct umessage_tp_information_resp_stat nid_msg_resp;
	struct umessage_tp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_TP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TP_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_TP_CODE_STAT_ALL;
	strcpy(nid_msg.um_uuid, "NULL");
	nid_msg.um_uuid_len = sizeof("NULL");
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while (1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_tp_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_TP_HEADER_LEN);
		if (nread != UMSG_TP_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);
		
		if (msghdr->um_req_code == UMSG_TP_CODE_RESP_END) {
			goto out;
		}
	
		assert(msghdr->um_req == UMSG_TP_CMD_INFORMATION);
		assert(msghdr->um_req_code == UMSG_TP_CODE_RESP_STAT_ALL);
		assert(msghdr->um_len >= UMSG_TP_HEADER_LEN);
	
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TP_HEADER_LEN);
		if (nread < (msghdr->um_len - UMSG_TP_HEADER_LEN)) {
			rc = -1;
			goto out;
		}
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TP, msghdr);
		printf("tp %s stat:\n"first_intend"nused: %d\n"first_intend"nfree: %d\n"first_intend"workers: %d\n"first_intend"max_wokers: %d\n"first_intend"no_free: %d\n",
			nid_msg_resp.um_uuid, nid_msg_resp.um_nused, nid_msg_resp.um_nfree, nid_msg_resp.um_workers, nid_msg_resp.um_max_workers, nid_msg_resp.um_no_free);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

static int
mgrtp_add(struct mgrtp_interface *mgrtp_p, char *uuid, uint16_t num_workers)
{
	char *log_header = "mgrtp_add";
	struct mgrtp_private *priv_p = mgrtp_p->t_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_tp_add nid_msg;
	struct umessage_tp_add_resp nid_msg_resp;
	struct umessage_tp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TP_CMD_ADD;
	msghdr->um_req_code = UMSG_TP_CODE_ADD;

	strcpy(nid_msg.um_uuid, uuid);
	nid_msg.um_uuid_len = strlen(uuid);
	nid_msg.um_workers = num_workers;

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_tp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TP_HEADER_LEN);
	if (nread != UMSG_TP_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_TP_CMD_ADD);
	assert(msghdr->um_req_code == UMSG_TP_CODE_RESP_ADD);
	assert(msghdr->um_len >= UMSG_TP_HEADER_LEN);

	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TP_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TP, msghdr);

	if (!nid_msg_resp.um_resp_code)
		printf("mgrtp_add: add tp successfully (%s)\n", nid_msg_resp.um_uuid);
	else
		printf("fail to create %s\n", nid_msg_resp.um_uuid);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrtp_delete(struct mgrtp_interface *mgrtp_p, char *tp_uuid)
{
	char *log_header = "mgrtp_del";
	struct mgrtp_private *priv_p = mgrtp_p->t_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_tp_delete nid_msg;
	struct umessage_tp_delete_resp nid_msg_resp;
	struct umessage_tp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TP_CMD_DELETE;
	msghdr->um_req_code = UMSG_TP_CODE_DELETE;
	strcpy(nid_msg.um_uuid, tp_uuid);
	nid_msg.um_uuid_len = strlen(tp_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_tp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TP_HEADER_LEN);
	if (nread != UMSG_TP_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_TP_CMD_DELETE);
	assert(msghdr->um_req_code == UMSG_TP_CODE_RESP_DELETE);
	assert(msghdr->um_len >= UMSG_TP_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TP_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TP, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: delete tp successfully\n",log_header);
	else
		printf("%s: delete tp failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrtp_display(struct mgrtp_interface *mgrtp_p, char *uuid, uint8_t is_setup)
{
	char *log_header = "mgrtp_display";
	struct mgrtp_private *priv_p = mgrtp_p->t_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_tp_display nid_msg;
	struct umessage_tp_display_resp nid_msg_resp;
	struct umessage_tp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TP_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_TP_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_TP_CODE_W_DISP;
	strcpy(nid_msg.um_uuid, uuid);
	nid_msg.um_uuid_len = strlen(uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_tp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TP_HEADER_LEN);
	if (nread != UMSG_TP_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_TP_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_TP_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_TP_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_TP_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TP_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TP, msghdr);

	printf("tp %s:\n\tmax_workers: %d\n\tmin_workers: %d\n\textend: %d\n\tdelay: %d\n", nid_msg_resp.um_uuid, nid_msg_resp.um_max_workers, nid_msg_resp.um_min_workers, nid_msg_resp.um_extend, nid_msg_resp.um_delay);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrtp_display_all(struct mgrtp_interface *mgrtp_p, uint8_t is_setup)
{
	char *log_header = "mgrtp_display_all";
	struct mgrtp_private *priv_p = mgrtp_p->t_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_tp_display nid_msg;
	struct umessage_tp_display_resp nid_msg_resp;
	struct umessage_tp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_TP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TP_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_TP_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_TP_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_tp_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_TP_HEADER_LEN);
		if (nread != UMSG_TP_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_TP_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_TP_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_TP_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_TP_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_TP_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TP_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TP, msghdr);

		printf("tp %s:\n\tmax_workers: %d\n\tmin_workers: %d\n\textend: %d\n\tdelay: %d\n", nid_msg_resp.um_uuid, nid_msg_resp.um_max_workers, nid_msg_resp.um_min_workers, nid_msg_resp.um_extend, nid_msg_resp.um_delay);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrtp_hello(struct mgrtp_interface *mgrtp_p)
{
	char *log_header = "mgrtp_hello";
	struct mgrtp_private *priv_p = mgrtp_p->t_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_tp_hello nid_msg, nid_msg_resp;
	struct umessage_tp_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_TP;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_tp_hdr *)&nid_msg;
	msghdr->um_req = UMSG_TP_CMD_HELLO;
	msghdr->um_req_code = UMSG_TP_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_tp_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_TP_HEADER_LEN);
	if (nread != UMSG_TP_HEADER_LEN) {
		printf("module tp is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_TP_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_TP_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_TP_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_TP_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_TP_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_TP, msghdr);
	printf("module tp is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrtp_operations mgrtp_op = {
	.tp_information_stat = mgrtp_information_stat,
	.tp_information_all = mgrtp_information_all,
	.tp_add = mgrtp_add,
	.tp_delete = mgrtp_delete,
	.tp_display = mgrtp_display,
	.tp_display_all = mgrtp_display_all,
	.tp_hello = mgrtp_hello,
};

int 
mgrtp_initialization(struct mgrtp_interface *mgrtp_p, struct mgrtp_setup *setup)
{
	char *log_header = "mgrtp_initialization";
	struct mgrtp_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrtp_p->t_private = priv_p;
	mgrtp_p->t_op = &mgrtp_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
