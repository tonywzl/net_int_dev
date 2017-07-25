/*
 * mgrini.c
 *	Implementation of Manager to ini Channel Module
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
#include "umpk_ini_if.h"
#include "mgrini_if.h"
#include "nid_shared.h"
#include "manager.h"

struct mgrini_private {
	char			p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_INI;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);
	sfd = util_nw_make_connection(ipstr, port);

	if (sfd < 0) {
		nid_log_error("cannot connect to the client (%s:%d), errno:%d", ipstr, port, errno);
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
mgrini_display_section_detail(struct mgrini_interface *mgrini_p, char *s_name, uint8_t is_template)
{
	char *log_header = "mgrini_display_section_detail";
	struct mgrini_private *priv_p = mgrini_p->i_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_ini_display nid_msg;
	struct umessage_ini_resp_display nid_msg_resp;
	struct umessage_ini_hdr *msghdr;

	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_INI, found = 0;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_ini_hdr *)&nid_msg;
	msghdr->um_req = UMSG_INI_CMD_DISPLAY;
	if (is_template == 1) {
		msghdr->um_req_code = UMSG_INI_CODE_T_SECTION_DETAIL;
	}
	else if (is_template == 0) {
		msghdr->um_req_code = UMSG_INI_CODE_C_SECTION_DETAIL;
	}
	else {
		nid_log_error("%s: cannot determine template command or configuration command. %d", log_header, is_template);
		goto out;
	}
	strcpy(nid_msg.um_key, s_name);
	nid_msg.um_key_len = strlen(s_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while (1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_ini_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_INI_HEADER_LEN);
		if (nread != UMSG_INI_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_INI_CODE_RESP_END) {
			goto out;
		}

		assert(msghdr->um_req == UMSG_INI_CMD_DISPLAY);
		if (is_template == 1) {
			assert(msghdr->um_req_code == UMSG_INI_CODE_T_RESP_SECTION_DETAIL);
		}
		else if (is_template == 0) {
			assert(msghdr->um_req_code == UMSG_INI_CODE_C_RESP_SECTION_DETAIL);
		}
		else {
			assert(0);
		}
		assert(msghdr->um_len >= UMSG_INI_HEADER_LEN);
	
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_INI_HEADER_LEN);
		if (nread < (msghdr->um_len - UMSG_INI_HEADER_LEN)) {
			rc = -1;
			goto out;
		}
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_INI, msghdr);
		if (!strcmp("type", nid_msg_resp.um_key) && is_template == 1) {
			printf(first_intend"%-35s%-25s%s\n", "key name", "default value", "description");
			printf("%s:\n", nid_msg_resp.um_value);
		}
		else if (!strcmp("type", nid_msg_resp.um_key) && is_template == 0) {
			printf(first_intend"%-35s%-25s\n", "key name", "value");
			printf("%s: %s\n", nid_msg_resp.um_desc, nid_msg_resp.um_value);
		}
		else {
			printf(first_intend"%-35s%-25s%s\n", nid_msg_resp.um_key, nid_msg_resp.um_value, nid_msg_resp.um_desc);
		}
		found ++;
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	if (found == 0) {
		printf("module %s not found.\n", s_name);
	} 
	return rc;

}

static int
mgrini_display_section(struct mgrini_interface *mgrini_p, uint8_t is_template)
{
	char *log_header = "mgrini_display_section";
	struct mgrini_private *priv_p = mgrini_p->i_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_ini_display nid_msg;
	struct umessage_ini_resp_display nid_msg_resp;
	struct umessage_ini_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1, is_first = 1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_INI;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_ini_hdr *)&nid_msg;
	msghdr->um_req = UMSG_INI_CMD_DISPLAY;
	if (is_template == 1) {
		msghdr->um_req_code = UMSG_INI_CODE_T_SECTION;
	}
	else if (is_template == 0) {
		msghdr->um_req_code = UMSG_INI_CODE_C_SECTION;
	}
	else {
		nid_log_error("%s: cannot determine template command or configuration command.", log_header);
		goto out;
	}
	strcpy(nid_msg.um_key, "NULL");
	nid_msg.um_key_len = strlen(nid_msg.um_key);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while (1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_ini_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_INI_HEADER_LEN);
		if (nread != UMSG_INI_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_INI_CODE_RESP_END) {
			goto out;
		}

		assert(msghdr->um_req == UMSG_INI_CMD_DISPLAY);
		if (is_template == 1) {
			assert(msghdr->um_req_code == UMSG_INI_CODE_T_RESP_SECTION);
		}
		else if (is_template == 0) {
			assert(msghdr->um_req_code == UMSG_INI_CODE_C_RESP_SECTION);
		}
		else {
			assert(0);
		}
		assert(msghdr->um_len >= UMSG_INI_HEADER_LEN);
	
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_INI_HEADER_LEN);
		if (nread < (msghdr->um_len - UMSG_INI_HEADER_LEN)) {
			rc = -1;
			goto out;
		}
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_INI, msghdr);
		if (is_first == 1 && is_template == 1) {
			printf("template modules:\n");
			is_first = 0;
		}
		else if (is_first == 1 && is_template ==0) {
			printf("config sections:\n");
			is_first = 0;
		}
		printf(first_intend"%s:"second_intend"%s\n", nid_msg_resp.um_value, nid_msg_resp.um_desc);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

static int
mgrini_display_all(struct mgrini_interface *mgrini_p, uint8_t is_template)
{
	char *log_header = "mgrini_display_all";
	struct mgrini_private *priv_p = mgrini_p->i_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_ini_display nid_msg;
	struct umessage_ini_resp_display nid_msg_resp;
	struct umessage_ini_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_INI, is_first = 1;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_ini_hdr *)&nid_msg;
	msghdr->um_req = UMSG_INI_CMD_DISPLAY;
	if (is_template == 1) {
		msghdr->um_req_code = UMSG_INI_CODE_T_ALL;
	}
	else if (is_template == 0) {
		msghdr->um_req_code = UMSG_INI_CODE_C_ALL;
	}
	else {
		nid_log_error("%s cannot determine template command or configuration command.", log_header);
		goto out;
	}
	strcpy(nid_msg.um_key, "NULL");
	nid_msg.um_key_len = strlen(nid_msg.um_key);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while (1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_ini_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_INI_HEADER_LEN);
		if (nread != UMSG_INI_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_INI_CODE_RESP_END) {
			goto out;
		}

		assert(msghdr->um_req == UMSG_INI_CMD_DISPLAY);
		if (is_template == 1) {
			assert(msghdr->um_req_code == UMSG_INI_CODE_T_RESP_ALL);
		}
		else if (is_template == 0) {
			assert(msghdr->um_req_code == UMSG_INI_CODE_C_RESP_ALL);
		}
		else {
			assert(0);
		}
		assert(msghdr->um_len >= UMSG_INI_HEADER_LEN);
	
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_INI_HEADER_LEN);
		if (nread < (msghdr->um_len - UMSG_INI_HEADER_LEN)) {
			rc = -1;
			goto out;
		}
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_INI, msghdr);
		if (!strcmp(nid_msg_resp.um_key, "type")) {
			if (is_first == 0) {
				printf("\n");
			}
			else if (is_template == 1) {
				printf(first_intend"%-35s%-25s%s\n", "key name", "default value", "description");
			}
			else {
				printf(first_intend"%-35s%-25s\n", "key name", "value");
			}
			is_first = 0;
			printf("%s: %s\n", nid_msg_resp.um_value, nid_msg_resp.um_desc);
		}
		else {
			printf(first_intend"%-35s%-25s%s\n", nid_msg_resp.um_key, nid_msg_resp.um_value, nid_msg_resp.um_desc);
		}
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

static int
mgrini_hello(struct mgrini_interface *mgrini_p)
{
	char *log_header = "mgrini_hello";
	struct mgrini_private *priv_p = mgrini_p->i_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_ini_hello nid_msg, nid_msg_resp;
	struct umessage_ini_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_INI;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_ini_hdr *)&nid_msg;
	msghdr->um_req = UMSG_INI_CMD_HELLO;
	msghdr->um_req_code = UMSG_INI_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_ini_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_INI_HEADER_LEN);
	if (nread != UMSG_INI_HEADER_LEN) {
		printf("module ini is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_INI_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_INI_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_INI_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_INI_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_INI_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_INI, msghdr);
	printf("module ini is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrini_operations mgrini_op = {
	.ini_display_section_detail = mgrini_display_section_detail,
	.ini_display_section = mgrini_display_section,
	.ini_display_all = mgrini_display_all,
	.ini_hello = mgrini_hello,
};

int
mgrini_initialization(struct mgrini_interface *mgrini_p, struct mgrini_setup *setup)
{
	char *log_header = "mgrini_initialization";
	struct mgrini_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrini_p->i_private = priv_p;
	mgrini_p->i_op = &mgrini_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
