/*
 * umpk_tp.c
 *	umpk for Thread Pool
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_tp_if.h"

static int
__encode_tp_information(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_information";
	struct umessage_tp_information *msg_p = (struct umessage_tp_information *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:uuid_len:%d, req:%d, req_code:%d", log_header, msg_p->um_uuid_len, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TP_ITEM_TP;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
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
__encode_tp_information_resp_stat(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_information_resp_stat";
	struct umessage_tp_information_resp_stat *msg_p = (struct umessage_tp_information_resp_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:uuid_len:%d", log_header, msg_p->um_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TP_ITEM_TP;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TP_ITEM_STAT;
	len++;
	*(uint16_t *)p = msg_p->um_nused;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_nfree;
	p += 2;
	len += 2;

 	*(uint16_t *)p = msg_p->um_workers;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_max_workers;
	p += 2;
	len += 2;

	*(uint32_t *)p = msg_p->um_no_free;
	p += 4;
	len += 4;

	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;

	return 0;

}

static int
__encode_tp_add(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_add";
	struct umessage_tp_add *msg_p = (struct umessage_tp_add *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:uuid_len:%d, req:%d, req_code:%d", log_header, msg_p->um_uuid_len, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TP_ITEM_TP;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TP_ITEM_WORKERS;
	len++;
	*(uint16_t *)p = msg_p->um_workers;
	p += 2;
	len += 2;

	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;
	nid_log_warning("%s:um_len:%d", log_header,(*(uint32_t *)p));

	return 0;
}

static int
__encode_tp_add_resp(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_add_resp";
	struct umessage_tp_add_resp *msg_p = (struct umessage_tp_add_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:uuid_len:%d, req:%d, req_code:%d", log_header, msg_p->um_uuid_len, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TP_ITEM_TP;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TP_ITEM_RESP_CODE;
	len++;
	*(uint8_t *)p = msg_p->um_resp_code;
	p += 1;
	len += 1;

	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;
	nid_log_warning("%s:um_len:%d", log_header,(*(uint32_t *)p));

	return 0;
}

static int
__encode_tp_delete(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_delete";
	struct umessage_tp_delete *msg_p = (struct umessage_tp_delete *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:uuid_len:%d, req:%d, req_code:%d", log_header, msg_p->um_uuid_len, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TP_ITEM_TP;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
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
__encode_tp_delete_resp(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_delete_resp";
	struct umessage_tp_delete_resp *msg_p = (struct umessage_tp_delete_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:uuid_len:%d, req:%d, req_code:%d", log_header, msg_p->um_uuid_len, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TP_ITEM_TP;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TP_ITEM_RESP_CODE;
	len++;
	*(uint8_t *)p = msg_p->um_resp_code;
	p += 1;
	len += 1;

	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;
	nid_log_warning("%s:um_len:%d", log_header,(*(uint32_t *)p));

	return 0;
}

static int
__encode_tp_display(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_display";
	struct umessage_tp_display *msg_p = (struct umessage_tp_display *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:uuid_len:%d, req:%d, req_code:%d", log_header, msg_p->um_uuid_len, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TP_ITEM_TP;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
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
__encode_tp_display_resp(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_display_resp";
	struct umessage_tp_display_resp *msg_p = (struct umessage_tp_display_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:uuid_len:%d", log_header, msg_p->um_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TP_ITEM_TP;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TP_ITEM_SETUP;
	len++;
	*(uint16_t *)p = msg_p->um_min_workers;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_max_workers;
	p += 2;
	len += 2;

 	*(uint16_t *)p = msg_p->um_extend;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_delay;
	p += 2;
	len += 2;

	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;

	return 0;

}

static int
__decode_tp_information(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_information";
	struct umessage_tp_information *res_p = (struct umessage_tp_information *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TP_ITEM_TP:
			res_p->um_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_uuid, p, res_p->um_uuid_len);
			res_p->um_uuid[res_p->um_uuid_len] = 0;
			p += res_p->um_uuid_len;
			len -= res_p->um_uuid_len;
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
__decode_tp_information_resp_stat(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_information_resp_stat";
	struct umessage_tp_information_resp_stat *res_p = (struct umessage_tp_information_resp_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++){
		case UMSG_TP_ITEM_TP:
			res_p->um_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_uuid, p, res_p->um_uuid_len);
			res_p->um_uuid[res_p->um_uuid_len] = 0;
			p += res_p->um_uuid_len;
			len -= res_p->um_uuid_len;
			break;

		case UMSG_TP_ITEM_STAT:
			res_p->um_nused = *(uint16_t *)p;
			p += 2;
			len -= 2;

			res_p->um_nfree = *(uint16_t *)p;
			p += 2;
			len -= 2;

			res_p->um_workers = *(uint16_t *)p;
			p += 2;
			len -= 2;

			res_p->um_max_workers = *(uint16_t *)p;
			p += 2;
			len -= 2;

			res_p->um_no_free = *(uint32_t *)p;
			p += 4;
			len -= 4;
			break;

		default:
			nid_log_error("%s: got wrong item (%d) len: %d", log_header, item, len);
			rc = -1;
			goto out;
		}
	}
out:
	return rc;
}

static int
__decode_tp_add(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_add";
	struct umessage_tp_add *res_p = (struct umessage_tp_add *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TP_ITEM_TP:
			res_p->um_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_uuid, p, res_p->um_uuid_len);
			res_p->um_uuid[res_p->um_uuid_len] = 0;
			p += res_p->um_uuid_len;
			len -= res_p->um_uuid_len;
			break;

		case UMSG_TP_ITEM_WORKERS:
			res_p->um_workers = *(int16_t *)p;
			p += 2;
			len -= 2;
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
__decode_tp_add_resp(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_add_resp";
	struct umessage_tp_add_resp *res_p = (struct umessage_tp_add_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TP_ITEM_TP:
			res_p->um_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_uuid, p, res_p->um_uuid_len);
			res_p->um_uuid[res_p->um_uuid_len] = 0;
			p += res_p->um_uuid_len;
			len -= res_p->um_uuid_len;
			break;

		case UMSG_TP_ITEM_RESP_CODE:
			res_p->um_resp_code = *(int8_t *)p;
			p += 1;
			len -= 1;
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
__decode_tp_delete(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_delete";
	struct umessage_tp_delete *res_p = (struct umessage_tp_delete *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TP_ITEM_TP:
			res_p->um_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_uuid, p, res_p->um_uuid_len);
			res_p->um_uuid[res_p->um_uuid_len] = 0;
			p += res_p->um_uuid_len;
			len -= res_p->um_uuid_len;
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
__decode_tp_delete_resp(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_delete_resp";
	struct umessage_tp_delete_resp *res_p = (struct umessage_tp_delete_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TP_ITEM_TP:
			res_p->um_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_uuid, p, res_p->um_uuid_len);
			res_p->um_uuid[res_p->um_uuid_len] = 0;
			p += res_p->um_uuid_len;
			len -= res_p->um_uuid_len;
			break;

		case UMSG_TP_ITEM_RESP_CODE:
			res_p->um_resp_code = *(int8_t *)p;
			p += 1;
			len -= 1;
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
__decode_tp_display(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_display";
	struct umessage_tp_display *res_p = (struct umessage_tp_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TP_ITEM_TP:
			res_p->um_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_uuid, p, res_p->um_uuid_len);
			res_p->um_uuid[res_p->um_uuid_len] = 0;
			p += res_p->um_uuid_len;
			len -= res_p->um_uuid_len;
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
__decode_tp_display_resp(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_display_resp";
	struct umessage_tp_display_resp *res_p = (struct umessage_tp_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++){
		case UMSG_TP_ITEM_TP:
			res_p->um_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_uuid, p, res_p->um_uuid_len);
			res_p->um_uuid[res_p->um_uuid_len] = 0;
			p += res_p->um_uuid_len;
			len -= res_p->um_uuid_len;
			break;

		case UMSG_TP_ITEM_SETUP:
			res_p->um_min_workers = *(uint16_t *)p;
			p += 2;
			len -= 2;

			res_p->um_max_workers = *(uint16_t *)p;
			p += 2;
			len -= 2;

			res_p->um_extend = *(uint16_t *)p;
			p += 2;
			len -= 2;

			res_p->um_delay = *(uint16_t *)p;
			p += 2;
			len -= 2;

			break;

		default:
			nid_log_error("%s: got wrong item (%d) len: %d", log_header, item, len);
			rc = -1;
			goto out;
		}
	}
out:
	return rc;
}

static int
__encode_tp_hello(char *out_buf, uint32_t *out_len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__encode_tp_hello";
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
__decode_tp_hello(char *p, int len, struct umessage_tp_hdr *msghdr_p)
{
	char *log_header = "__decode_tp_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TP_HEADER_LEN;

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
umpk_encode_tp(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_tp";
	struct umessage_tp_hdr *msghdr_p = (struct umessage_tp_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_TP_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_STAT:
		case UMSG_TP_CODE_STAT_ALL:
			rc = __encode_tp_information(out_buf, out_len, msghdr_p);
			break;

		case UMSG_TP_CODE_RESP_STAT:
		case UMSG_TP_CODE_RESP_STAT_ALL:
		case UMSG_TP_CODE_RESP_END:
		case UMSG_TP_CODE_RESP_NOT_FOUND:
			rc = __encode_tp_information_resp_stat(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TP_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_ADD:
			rc = __encode_tp_add(out_buf, out_len, msghdr_p);
			break;

		case UMSG_TP_CODE_RESP_ADD:
			rc = __encode_tp_add_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TP_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_DELETE:
			rc = __encode_tp_delete(out_buf, out_len, msghdr_p);
			break;

		case UMSG_TP_CODE_RESP_ADD:
			rc = __encode_tp_delete_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TP_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_S_DISP:
		case UMSG_TP_CODE_S_DISP_ALL:
		case UMSG_TP_CODE_W_DISP:
		case UMSG_TP_CODE_W_DISP_ALL:
			rc = __encode_tp_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_TP_CODE_S_RESP_DISP:
		case UMSG_TP_CODE_S_RESP_DISP_ALL:
		case UMSG_TP_CODE_W_RESP_DISP:
		case UMSG_TP_CODE_W_RESP_DISP_ALL:
		case UMSG_TP_CODE_DISP_END:
			rc = __encode_tp_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TP_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_HELLO:
		case UMSG_TP_CODE_RESP_HELLO:
			rc = __encode_tp_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_tp(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_tp";
	struct umessage_tp_hdr *msghdr_p = (struct umessage_tp_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_TP_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_STAT:
		case UMSG_TP_CODE_STAT_ALL:
			rc = __decode_tp_information(p, len, msghdr_p);
			break;

		case UMSG_TP_CODE_RESP_STAT:
		case UMSG_TP_CODE_RESP_STAT_ALL:
		case UMSG_TP_CODE_RESP_END:
		case UMSG_TP_CODE_RESP_NOT_FOUND:
			rc = __decode_tp_information_resp_stat(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TP_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_ADD:
			rc = __decode_tp_add(p, len, msghdr_p);
			break;

		case UMSG_TP_CODE_RESP_ADD:
			rc = __decode_tp_add_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TP_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_DELETE:
			rc = __decode_tp_delete(p, len, msghdr_p);
			break;

		case UMSG_TP_CODE_RESP_DELETE:
			rc = __decode_tp_delete_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TP_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_TP_CODE_S_DISP:
		case UMSG_TP_CODE_S_DISP_ALL:
		case UMSG_TP_CODE_W_DISP:
		case UMSG_TP_CODE_W_DISP_ALL:
			rc = __decode_tp_display(p, len, msghdr_p);
			break;

		case UMSG_TP_CODE_S_RESP_DISP:
		case UMSG_TP_CODE_S_RESP_DISP_ALL:
		case UMSG_TP_CODE_W_RESP_DISP:
		case UMSG_TP_CODE_W_RESP_DISP_ALL:
		case UMSG_TP_CODE_DISP_END:
			rc = __decode_tp_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TP_CMD_HELLO:
		switch(msghdr_p->um_req_code) {
		case UMSG_TP_CODE_HELLO:
		case UMSG_TP_CODE_RESP_HELLO:
			rc = __decode_tp_hello(p, len, msghdr_p);
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
