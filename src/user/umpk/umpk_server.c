/*
 * umpk_server.c
 *	umpk for Server
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_server_if.h"

static int
__encode_server_version(char *out_buf, uint32_t *out_len, struct umessage_server_hdr *msghdr_p)
{
	char *log_header = "__encode_server_version";
	char *p = out_buf;
	uint32_t len = 0;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;
	nid_log_warning("%s:um_len:%d", log_header,(*(uint32_t *)p));

	return 0;
}


static int
__decode_server_version(char *p, int len, struct umessage_server_hdr *msghdr_p)
{
	char *log_header = "__decode_server_version";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SERVER_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_SERVER_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		default:
			nid_log_error("%s: got wrong item (%d)", log_header, item);
			rc = -1;
			goto out;
		}
		
	}

out:
	return rc;
}

static int
__encode_server_version_resp(char *out_buf, uint32_t *out_len, struct umessage_server_hdr *msghdr_p)
{
	char *log_header = "__encode_server_version_resp";
	struct umessage_server_version_resp *msg_p = (struct umessage_server_version_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_SERVER_ITEM_VERSION;
	len++;
	sub_len = msg_p->um_version_len;
	*(uint8_t *)p = (uint8_t)sub_len;
	p += 1;
	len += 1;
	memcpy(p, msg_p->um_version, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;
	nid_log_warning("%s:um_len:%d", log_header,(*(uint32_t *)p));

	return 0;
}

static int
__decode_server_version_resp(char *p, int len, struct umessage_server_hdr *msghdr_p)
{
	char *log_header = "__decode_server_version_resp";
	struct umessage_server_version_resp *res_p = (struct umessage_server_version_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SERVER_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_SERVER_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_SERVER_ITEM_VERSION:
			res_p->um_version_len = *(int8_t *)p;
			p += 1;
			len -= 1;
			memcpy(res_p->um_version, p, res_p->um_version_len);
			res_p->um_version[res_p->um_version_len] = 0;
			p += res_p->um_version_len;
			len -= res_p->um_version_len;
			break;

		default:
			nid_log_error("%s: got wrong item (%d)", log_header, item);
			rc = -1;
			goto out;
		}
		
	}

out:
	return rc;
}

int
umpk_encode_server(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_server";
	struct umessage_server_hdr *msghdr_p = (struct umessage_server_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_SERVER_CMD_VERSION:
		switch (msghdr_p->um_req_code) {
		case UMSG_SERVER_CODE_VERSION:
			rc = __encode_server_version(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SERVER_CODE_RESP_VERSION:
			rc = __encode_server_version_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
	}
	return rc;
}

int
umpk_decode_server(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_server";
	struct umessage_server_hdr *msghdr_p = (struct umessage_server_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_SERVER_CMD_VERSION:
		switch(msghdr_p->um_req_code) {
		case UMSG_SERVER_CODE_VERSION:
			rc = __decode_server_version(p, len, msghdr_p);
			break;

		case UMSG_SERVER_CODE_RESP_VERSION:
			rc = __decode_server_version_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
	}
	return rc;
}
