/*
 * umpk_ini.c
 *	umpk for ini
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_ini_if.h"

static int
__encode_ini_display(char *out_buf, uint32_t *out_len, struct umessage_ini_hdr *msghdr_p)
{
	char *log_header = "__encode_ini_display";
	struct umessage_ini_display *msg_p = (struct umessage_ini_display *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:key_len:%d, req:%d, req_code:%d", log_header, msg_p->um_key_len, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_INI_ITEM_KEY;
	len++;
	sub_len = msg_p->um_key_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_key, sub_len);
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
__decode_ini_display(char *p, int len, struct umessage_ini_hdr *msghdr_p)
{
	char *log_header = "__decode_ini_display";
	struct umessage_ini_display *res_p = (struct umessage_ini_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_INI_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_INI_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_INI_ITEM_KEY:
			res_p->um_key_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_key, p, res_p->um_key_len);
			res_p->um_key[res_p->um_key_len] = 0;
			p += res_p->um_key_len;
			len -= res_p->um_key_len;
			nid_log_warning("%s: un_key:%s",log_header, res_p->um_key); 
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

static int
__encode_ini_resp_display(char *out_buf, uint32_t *out_len, struct umessage_ini_hdr *msghdr_p)
{
	char *log_header = "__encode_ini_display";
	struct umessage_ini_resp_display *msg_p = (struct umessage_ini_resp_display *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:key_len:%d, req:%d, req_code:%d", log_header, msg_p->um_key_len, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_INI_ITEM_KEY;
	len++;
	sub_len = msg_p->um_key_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_key, sub_len);
	p += sub_len;
	len += sub_len;
	nid_log_warning("%s: um_key: %s", log_header, msg_p->um_key);

	*p++ = UMSG_INI_ITEM_DESC;
	len++;
	sub_len = msg_p->um_desc_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_desc, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_INI_ITEM_VAL;
	len++;
	sub_len = msg_p->um_value_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_value, sub_len);
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
__decode_ini_resp_display(char *p, int len, struct umessage_ini_hdr *msghdr_p)
{
	char *log_header = "__decode_ini_display";
	struct umessage_ini_resp_display *res_p = (struct umessage_ini_resp_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_INI_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_INI_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_INI_ITEM_KEY:
			res_p->um_key_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_key, p, res_p->um_key_len);
			res_p->um_key[res_p->um_key_len] = 0;
			p += res_p->um_key_len;
			len -= res_p->um_key_len;
			break;

		case UMSG_INI_ITEM_DESC:
			res_p->um_desc_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_desc, p, res_p->um_desc_len);
			res_p->um_desc[res_p->um_desc_len] = 0;
			p += res_p->um_desc_len;
			len -= res_p->um_desc_len;
			break;

		case UMSG_INI_ITEM_VAL:
			res_p->um_value_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_value, p, res_p->um_value_len);
			res_p->um_value[res_p->um_value_len] = 0;
			p += res_p->um_value_len;
			len -= res_p->um_value_len;
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

static int
__encode_ini_hello(char *out_buf, uint32_t *out_len, struct umessage_ini_hdr *msghdr_p)
{
	char *log_header = "__encode_ini_hello";
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
__decode_ini_hello(char *p, int len, struct umessage_ini_hdr *msghdr_p)
{
	char *log_header = "__decode_ini_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_INI_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_INI_HEADER_LEN;

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

int
umpk_encode_ini(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_ini";
	struct umessage_ini_hdr *msghdr_p = (struct umessage_ini_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_INI_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_INI_CODE_T_SECTION:
		case UMSG_INI_CODE_T_SECTION_DETAIL:
		case UMSG_INI_CODE_T_ALL:
		case UMSG_INI_CODE_C_SECTION:
		case UMSG_INI_CODE_C_SECTION_DETAIL:
		case UMSG_INI_CODE_C_ALL:

			rc = __encode_ini_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_INI_CODE_T_RESP_SECTION:
		case UMSG_INI_CODE_T_RESP_SECTION_DETAIL:
		case UMSG_INI_CODE_T_RESP_ALL:
		case UMSG_INI_CODE_C_RESP_SECTION:
		case UMSG_INI_CODE_C_RESP_SECTION_DETAIL:
		case UMSG_INI_CODE_C_RESP_ALL:
		case UMSG_INI_CODE_RESP_END:

			rc = __encode_ini_resp_display(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_INI_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_INI_CODE_HELLO:
		case UMSG_INI_CODE_RESP_HELLO:
			rc = __encode_ini_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_ini(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_ini";
	struct umessage_ini_hdr *msghdr_p = (struct umessage_ini_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_INI_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_INI_CODE_T_SECTION:
		case UMSG_INI_CODE_T_SECTION_DETAIL:
		case UMSG_INI_CODE_T_ALL:
		case UMSG_INI_CODE_C_SECTION:
		case UMSG_INI_CODE_C_SECTION_DETAIL:
		case UMSG_INI_CODE_C_ALL:

			rc = __decode_ini_display(p, len, msghdr_p);
			break;

		case UMSG_INI_CODE_T_RESP_SECTION:
		case UMSG_INI_CODE_T_RESP_SECTION_DETAIL:
		case UMSG_INI_CODE_T_RESP_ALL:
		case UMSG_INI_CODE_C_RESP_SECTION:
		case UMSG_INI_CODE_C_RESP_SECTION_DETAIL:
		case UMSG_INI_CODE_C_RESP_ALL:
		case UMSG_INI_CODE_RESP_END:

			rc = __decode_ini_resp_display(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_INI_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_INI_CODE_HELLO:
		case UMSG_INI_CODE_RESP_HELLO:
			rc = __decode_ini_hello(p, len, msghdr_p);
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
