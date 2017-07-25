/*
 * mgrdrw.c
 * 	Implementation of Manager to DRW Channel Module
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
#include "umpk_drw_if.h"
#include "mgrdrw_if.h"
#include "nid_shared.h"

struct mgrdrw_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_DRW;

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
mgrdrw_add(struct mgrdrw_interface *mgrdrw_p,char *drw_name, char *exportname, uint8_t device_provision)
{
	char *log_header = "mgrdrw_add";
	struct mgrdrw_private *priv_p = mgrdrw_p->drw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_drw_add nid_msg;
	struct umessage_drw_add_resp nid_msg_resp;
	struct umessage_drw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_DRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_drw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DRW_CMD_ADD;
	msghdr->um_req_code = UMSG_DRW_CODE_ADD;
	strcpy(nid_msg.um_drw_name, drw_name);
	nid_msg.um_drw_name_len = strlen(drw_name);
	strcpy(nid_msg.um_exportname, exportname);
	nid_msg.um_exportname_len = strlen(exportname);
	nid_msg.um_device_provision = device_provision;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_drw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_DRW_HEADER_LEN);
	if (nread != UMSG_DRW_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_DRW_CMD_ADD);
	assert(msghdr->um_req_code == UMSG_DRW_CODE_RESP_ADD);
	assert(msghdr->um_len >= UMSG_DRW_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_DRW_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DRW, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: add drw successfully\n",log_header);
	else
		printf("%s: add drw failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrdrw_delete(struct mgrdrw_interface *mgrdrw_p, char *drw_name)
{
	char *log_header = "mgrdrw_del";
	struct mgrdrw_private *priv_p = mgrdrw_p->drw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_drw_delete nid_msg;
	struct umessage_drw_delete_resp nid_msg_resp;
	struct umessage_drw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_DRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_drw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DRW_CMD_DELETE;
	msghdr->um_req_code = UMSG_DRW_CODE_DELETE;
	strcpy(nid_msg.um_drw_name, drw_name);
	nid_msg.um_drw_name_len = strlen(drw_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_drw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_DRW_HEADER_LEN);
	if (nread != UMSG_DRW_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_DRW_CMD_DELETE);
	assert(msghdr->um_req_code == UMSG_DRW_CODE_RESP_DELETE);
	assert(msghdr->um_len >= UMSG_DRW_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_DRW_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DRW, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: delete drw successfully\n",log_header);
	else
		printf("%s: delete drw failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrdrw_ready(struct mgrdrw_interface *mgrdrw_p,char *drw_name)
{
	char *log_header = "mgrdrw_ready";
	struct mgrdrw_private *priv_p = mgrdrw_p->drw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_drw_ready nid_msg;
	struct umessage_drw_ready_resp nid_msg_resp;
	struct umessage_drw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_DRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_drw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DRW_CMD_READY;
	msghdr->um_req_code = UMSG_DRW_CODE_READY;
	strcpy(nid_msg.um_drw_name, drw_name);
	nid_msg.um_drw_name_len = strlen(drw_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_drw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_DRW_HEADER_LEN);
	if (nread != UMSG_DRW_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_DRW_CMD_READY);
	assert(msghdr->um_req_code == UMSG_DRW_CODE_RESP_READY);
	assert(msghdr->um_len >= UMSG_DRW_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_DRW_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DRW, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: set drw ready successfully\n",log_header);
	else
		printf("%s: set drw ready failed\n", log_header);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrdrw_display(struct mgrdrw_interface *mgrdrw_p, char *drw_name, uint8_t is_setup)
{
	char *log_header = "mgrdrw_display";
	struct mgrdrw_private *priv_p = mgrdrw_p->drw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_drw_display nid_msg;
	struct umessage_drw_display_resp nid_msg_resp;
	struct umessage_drw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_DRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_drw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DRW_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_DRW_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_DRW_CODE_W_DISP;
	strcpy(nid_msg.um_drw_name, drw_name);
	nid_msg.um_drw_name_len = strlen(drw_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_drw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_DRW_HEADER_LEN);
	if (nread != UMSG_DRW_HEADER_LEN) {
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

	assert(msghdr->um_req == UMSG_DRW_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_DRW_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_DRW_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_DRW_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_DRW_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DRW, msghdr);
	printf("drw %s:\n\texportname: %s\n\tdevice_provision: %d\n\tsimulate_async: %d\n\tsimulate_delay: %d\n", nid_msg_resp.um_drw_name, nid_msg_resp.um_exportname, nid_msg_resp.um_device_provision, nid_msg_resp.um_simulate_async, nid_msg_resp.um_simulate_delay);
	printf("\tsimulate_delay_min_gap: %d\n\tsimulate_delay_max_gap: %d\n\tsimulate_delay_time_us: %d\n", nid_msg_resp.um_simulate_delay_min_gap, nid_msg_resp.um_simulate_delay_max_gap, nid_msg_resp.um_simulate_delay_time_us);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrdrw_display_all(struct mgrdrw_interface *mgrdrw_p, uint8_t is_setup)
{
	char *log_header = "mgrdrw_display_all";
	struct mgrdrw_private *priv_p = mgrdrw_p->drw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_drw_display nid_msg;
	struct umessage_drw_display_resp nid_msg_resp;
	struct umessage_drw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_DRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_drw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DRW_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_DRW_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_DRW_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_drw_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_DRW_HEADER_LEN);
		if (nread != UMSG_DRW_HEADER_LEN) {
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

		if (msghdr->um_req_code == UMSG_DRW_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_DRW_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_DRW_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_DRW_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_DRW_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_DRW_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DRW, msghdr);

		printf("drw %s:\n\texportname: %s\n\tdevice_provision: %d\n\tsimulate_async: %d\n\tsimulate_delay: %d\n", nid_msg_resp.um_drw_name, nid_msg_resp.um_exportname, nid_msg_resp.um_device_provision, nid_msg_resp.um_simulate_async, nid_msg_resp.um_simulate_delay);
		printf("\tsimulate_delay_min_gap: %d\n\tsimulate_delay_max_gap: %d\n\tsimulate_delay_time_us: %d\n", nid_msg_resp.um_simulate_delay_min_gap, nid_msg_resp.um_simulate_delay_max_gap, nid_msg_resp.um_simulate_delay_time_us);

	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrdrw_hello(struct mgrdrw_interface *mgrdrw_p)
{
	char *log_header = "mgrdrw_hello";
	struct mgrdrw_private *priv_p = mgrdrw_p->drw_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_drw_hello nid_msg, nid_msg_resp;
	struct umessage_drw_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_DRW;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_drw_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DRW_CMD_HELLO;
	msghdr->um_req_code = UMSG_DRW_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_drw_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_DRW_HEADER_LEN);
	if (nread != UMSG_DRW_HEADER_LEN) {
		printf("module drw is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_DRW_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_DRW_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_DRW_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_DRW_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_DRW_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DRW, msghdr);
	printf("module drw is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrdrw_operations mgrdrw_op = {
	.drw_add = mgrdrw_add,
	.drw_delete = mgrdrw_delete,
	.drw_ready = mgrdrw_ready,
	.drw_display = mgrdrw_display,
	.drw_display_all = mgrdrw_display_all,
	.drw_hello = mgrdrw_hello,
};

int
mgrdrw_initialization(struct mgrdrw_interface *mgrdrw_p, struct mgrdrw_setup *setup)
{
	struct mgrdrw_private *priv_p;

	nid_log_debug("mgrdrw_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrdrw_p->drw_private = priv_p;
	mgrdrw_p->drw_op = &mgrdrw_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
