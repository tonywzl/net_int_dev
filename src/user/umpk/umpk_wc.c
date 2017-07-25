/*
 * umpk_wc.c
 * 	umpk for write cache
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_wc_if.h"

static int
__encode_wc_information(char *out_buf, uint32_t *out_len, struct umessage_wc_hdr *msghdr_p)
{
	char *log_header = "__encode_wc_information";
	struct umessage_wc_information *msg_p = (struct umessage_wc_information *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_WC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_wc_information_resp_stat(char *out_buf, uint32_t *out_len, struct umessage_wc_hdr *msghdr_p)
{
	char *log_header = "__encode_wc_information_resp_stat";
	struct umessage_wc_information_resp_stat *msg_p = (struct umessage_wc_information_resp_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->wc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_WC_ITEM_CACHE;
	len++;
	sub_len = msg_p->wc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->wc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_WC_ITEM_BLOCK_OCCUPIED;
	len++;
	*(uint16_t *)p = msg_p->wc_block_occupied;
	p += 2;
	len += 2;

	*p++ = UMSG_WC_ITEM_NBLOCKS;
	len++;
	*(uint16_t *)p = msg_p->wc_flush_nblocks;
	p += 2;
	len += 2;

	*p++ = UMSG_WC_ITEM_BLOCK_WRITE;
	len++;
	*(uint16_t *)p = msg_p->wc_cur_write_block;
	p += 2;
	len += 2;

	*p++ = UMSG_WC_ITEM_BLOCK_FLUSH;
	len++;
	*(uint16_t *)p = msg_p->wc_cur_flush_block;
	p += 2;
	len += 2;

	*p++ = UMSG_WC_ITEM_SEQ_ASSIGNED;
	len++;
	*(uint64_t *)p = msg_p->wc_seq_assigned;
	p += 8;
	len += 8;

	*p++ = UMSG_WC_ITEM_SEQ_FLUSHED;
	len++;
	*(uint64_t *)p = msg_p->wc_seq_flushed;
	p += 8;
	len += 8;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_wc_hello(char *out_buf, uint32_t *out_len, struct umessage_wc_hdr *msghdr_p)
{
	char *log_header = "__encode_wc_hello";
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
__decode_wc_information(char *p, int len, struct umessage_wc_hdr *msghdr_p)
{
	char *log_header = "__decode_wc_information";
	struct umessage_wc_information *res_p = (struct umessage_wc_information *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_WC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_WC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_WC_ITEM_CACHE:
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
__decode_wc_information_resp_stat(char *p, int len, struct umessage_wc_hdr *msghdr_p)
{
	char *log_header = "__decode_wc_information_resp_stat";
	struct umessage_wc_information_resp_stat *res_p = (struct umessage_wc_information_resp_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_WC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_WC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_WC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_WC_ITEM_CACHE (%d)", log_header, item);
			res_p->wc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->wc_uuid, p, res_p->wc_uuid_len);
			res_p->wc_uuid[res_p->wc_uuid_len] = 0;
			p += res_p->wc_uuid_len;
			len -= res_p->wc_uuid_len;
			break;

		case UMSG_WC_ITEM_BLOCK_OCCUPIED:
			nid_log_error("%s: got item UMSG_WC_ITEM_BLOCK_OCCUPIED (%d)", log_header, item);
			res_p->wc_block_occupied = *(int16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_WC_ITEM_NBLOCKS:
			nid_log_error("%s: got item UMSG_WC_ITEM_BLOCK_OCCUPIED (%d)", log_header, item);
			res_p->wc_flush_nblocks = *(int16_t *)p;
			p += 2;
			len += 2;
			break;

		case UMSG_WC_ITEM_BLOCK_WRITE:
			nid_log_error("%s: got item UMSG_WC_ITEM_BLOCK_WRITE (%d)", log_header, item);
			res_p->wc_cur_write_block = *(int16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_WC_ITEM_BLOCK_FLUSH:
			nid_log_error("%s: got item UMSG_WC_ITEM_BLOCK_FLUSH (%d)", log_header, item);
			res_p->wc_cur_flush_block = *(int16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_WC_ITEM_SEQ_ASSIGNED:
			nid_log_error("%s: got item UMSG_WC_ITEM_SEQ_ASSIGNED (%d)", log_header, item);
			res_p->wc_seq_assigned = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_WC_ITEM_SEQ_FLUSHED:
			nid_log_error("%s: got item UMSG_WC_ITEM_SEQ_FLUSHED (%d)", log_header, item);
			res_p->wc_seq_flushed = *(int64_t *)p;
			p += 8;
			len -= 8;
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
__decode_wc_hello(char *p, int len, struct umessage_wc_hdr *msghdr_p)
{
	char *log_header = "__decode_wc_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_WC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_WC_HEADER_LEN;

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
umpk_encode_wc(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_wc";
	struct umessage_wc_hdr *msghdr_p = (struct umessage_wc_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_WC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_WC_CODE_FLUSHING:
		case UMSG_WC_CODE_STAT:
			rc = __encode_wc_information(out_buf, out_len, msghdr_p);
			break;

		case UMSG_WC_CODE_RESP_STAT:
			rc = __encode_wc_information_resp_stat(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_WC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_WC_CODE_HELLO:
		case UMSG_WC_CODE_RESP_HELLO:
			rc = __encode_wc_hello(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd (%d)", log_header, msghdr_p->um_req);
			break;
	}
	return rc;
}

int
umpk_decode_wc(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_wc";
	struct umessage_wc_hdr *msghdr_p = (struct umessage_wc_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_WC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_WC_CODE_FLUSHING:
		case UMSG_WC_CODE_STAT:
			rc = __decode_wc_information(p, len, msghdr_p);
			break;

		case UMSG_WC_CODE_RESP_STAT:
			rc = __decode_wc_information_resp_stat(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_WC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_WC_CODE_HELLO:
		case UMSG_WC_CODE_RESP_HELLO:
			rc = __decode_wc_hello(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd (%d)", log_header, msghdr_p->um_req);
		break;
	}
	return rc;
}
