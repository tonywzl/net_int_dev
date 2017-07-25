/*
 * umpk_twc.c
 * 	umpk for Write Through Memory Write Cache
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_twc_if.h"


static int
__encode_twc_information(char *out_buf, uint32_t *out_len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__encode_twc_information";
	struct umessage_twc_information *msg_p = (struct umessage_twc_information *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_twc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_TWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_twc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_twc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TWC_ITEM_CHANNEL;
	len++;
	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_twc_throughput(char *out_buf, uint32_t *out_len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__encode_twc_throughput";
	struct umessage_twc_throughput *msg_p = (struct umessage_twc_throughput *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_twc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_TWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_twc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_twc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_twc_information_resp_stat(char *out_buf, uint32_t *out_len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__encode_twc_information_resp_stat";
	struct umessage_twc_information_resp_stat *msg_p = (struct umessage_twc_information_resp_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_twc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_TWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_twc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_twc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TWC_ITEM_SEQ_ASSIGNED;
	len++;
	*(uint64_t *)p = msg_p->um_seq_assigned;
	p += 8;
	len += 8;

	*p++ = UMSG_TWC_ITEM_SEQ_FLUSHED;
	len++;
	*(uint64_t *)p = msg_p->um_seq_flushed;
	p += 8;
	len += 8;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_twc_information_throughput_stat(char *out_buf, uint32_t *out_len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__encode_twc_information_throughput_stat";
	struct umessage_twc_information_throughput_stat *msg_p = (struct umessage_twc_information_throughput_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_twc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_TWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_twc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_twc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TWC_ITEM_DATA_FLUSHED;
	len++;
	*(uint64_t *)p = msg_p->um_seq_data_flushed;
	p += 8;
	len += 8;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_twc_information_throughput_stat(char *p, int len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__decode_twc_information_resp_stat";
	struct umessage_twc_information_throughput_stat *res_p = (struct umessage_twc_information_throughput_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_TWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TWC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_TWC_ITEM_CACHE (%d)", log_header, item);
			res_p->um_twc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_twc_uuid, p, res_p->um_twc_uuid_len);
			res_p->um_twc_uuid[res_p->um_twc_uuid_len] = 0;
			p += res_p->um_twc_uuid_len;
			len -= res_p->um_twc_uuid_len;
			break;

		case UMSG_TWC_ITEM_DATA_FLUSHED:
			nid_log_error("%s: got item UMSG_TWC_ITEM_DATA_FLUSHED (%d)", log_header, item);
			res_p->um_seq_data_flushed= *(int64_t *)p;
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
__encode_twc_information_rw_stat(char *out_buf, uint32_t *out_len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__encode_twc_information_rw_stat";
	struct umessage_twc_information_rw_stat *msg_p = (struct umessage_twc_information_rw_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_twc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_TWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_twc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_twc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TWC_ITEM_CHANNEL;
	len++;
	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TWC_ITEM_RES_NUM;
	len++;
	*(uint64_t *)p = msg_p->res;
	p += 8;
	len += 8;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_twc_information_rw_stat(char *p, int len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__decode_twc_information_resp_stat";
	struct umessage_twc_information_rw_stat *res_p = (struct umessage_twc_information_rw_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_TWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TWC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_TWC_ITEM_CACHE (%d)", log_header, item);
			res_p->um_twc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_twc_uuid, p, res_p->um_twc_uuid_len);
			res_p->um_twc_uuid[res_p->um_twc_uuid_len] = 0;
			p += res_p->um_twc_uuid_len;
			len -= res_p->um_twc_uuid_len;
			break;

		case UMSG_TWC_ITEM_CHANNEL:
			nid_log_error("%s: got item UMSG_TWC_ITEM_CHANNEL (%d)", log_header, item);
			res_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_chan_uuid, p, res_p->um_chan_uuid_len);
			res_p->um_chan_uuid[res_p->um_chan_uuid_len] = 0;
			p += res_p->um_chan_uuid_len;
			len -= res_p->um_chan_uuid_len;
			break;

		case UMSG_TWC_ITEM_RES_NUM:
			nid_log_error("%s: got item UMSG_TWC_ITEM_RES_NUM (%d)", log_header, item);
			res_p->res = *(int64_t *)p;
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
__decode_twc_information(char *p, int len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__decode_twc_information";
	struct umessage_twc_information *res_p = (struct umessage_twc_information *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_TWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TWC_ITEM_CACHE:
			res_p->um_twc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_twc_uuid, p, res_p->um_twc_uuid_len);
			res_p->um_twc_uuid[res_p->um_twc_uuid_len] = 0;
			p += res_p->um_twc_uuid_len;
			len -= res_p->um_twc_uuid_len;
			break;

		case UMSG_TWC_ITEM_CHANNEL:
			res_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_chan_uuid, p, res_p->um_chan_uuid_len);
			res_p->um_chan_uuid[res_p->um_chan_uuid_len] = 0;
			p += res_p->um_chan_uuid_len;
			len -= res_p->um_chan_uuid_len;
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
__decode_twc_throughput(char *p, int len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__decode_twc_throughput";
	struct umessage_twc_throughput *res_p = (struct umessage_twc_throughput *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_TWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TWC_ITEM_CACHE:
			res_p->um_twc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_twc_uuid, p, res_p->um_twc_uuid_len);
			res_p->um_twc_uuid[res_p->um_twc_uuid_len] = 0;
			p += res_p->um_twc_uuid_len;
			len -= res_p->um_twc_uuid_len;
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
__decode_twc_information_resp_stat(char *p, int len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__decode_twc_information_resp_stat";
	struct umessage_twc_information_resp_stat *res_p = (struct umessage_twc_information_resp_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_TWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TWC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_TWC_ITEM_CACHE (%d)", log_header, item);
			res_p->um_twc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_twc_uuid, p, res_p->um_twc_uuid_len);
			res_p->um_twc_uuid[res_p->um_twc_uuid_len] = 0;
			p += res_p->um_twc_uuid_len;
			len -= res_p->um_twc_uuid_len;
			break;

		case UMSG_TWC_ITEM_SEQ_ASSIGNED:
			nid_log_error("%s: got item UMSG_TWC_ITEM_SEQ_ASSIGNED (%d)", log_header, item);
			res_p->um_seq_assigned = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_TWC_ITEM_SEQ_FLUSHED:
			nid_log_error("%s: got item UMSG_TWC_ITEM_SEQ_FLUSHED (%d)", log_header, item);
			res_p->um_seq_flushed = *(int64_t *)p;
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
__encode_twc_display(char *out_buf, uint32_t *out_len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__encode_twc_display";
	char *p = out_buf;
	struct umessage_twc_display *msg_p = (struct umessage_twc_display *)msghdr_p;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_twc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_twc_uuid, sub_len);
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
__decode_twc_display(char *p, int len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__decode_twc_display";
	struct umessage_twc_display *res_p = (struct umessage_twc_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_TWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TWC_ITEM_CACHE:
			res_p->um_twc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_twc_uuid, p, res_p->um_twc_uuid_len);
			res_p->um_twc_uuid[res_p->um_twc_uuid_len] = 0;
			p += res_p->um_twc_uuid_len;
			len -= res_p->um_twc_uuid_len;
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
__encode_twc_display_resp(char *out_buf, uint32_t *out_len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__encode_twc_display_resp";
	char *p = out_buf;
	struct umessage_twc_display_resp *msg_p = (struct umessage_twc_display_resp *)msghdr_p;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	*p++ = UMSG_TWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_twc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_twc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TWC_ITEM_TP_NAME;
	len++;
	sub_len = msg_p->um_tp_name_len;
	*(uint8_t *)p = (uint8_t)sub_len;
	p += 1;
	len += 1;
	memcpy(p, msg_p->um_tp_name, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_TWC_ITEM_DO_FP;
	len++;
	*(uint8_t *)p = msg_p->um_do_fp;
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
__decode_twc_display_resp(char *p, int len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__decode_twc_display_resp";
	struct umessage_twc_display_resp *res_p = (struct umessage_twc_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_TWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_TWC_ITEM_CACHE:
			res_p->um_twc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_twc_uuid, p, res_p->um_twc_uuid_len);
			res_p->um_twc_uuid[res_p->um_twc_uuid_len] = 0;
			p += res_p->um_twc_uuid_len;
			len -= res_p->um_twc_uuid_len;
			break;

		case UMSG_TWC_ITEM_TP_NAME:
			res_p->um_tp_name_len = *(int8_t *)p;
			p += 1;
			len -= 1;
			memcpy(res_p->um_tp_name, p, res_p->um_tp_name_len);
			res_p->um_tp_name[res_p->um_tp_name_len] = 0;
			p += res_p->um_tp_name_len;
			len -= res_p->um_tp_name_len;
			break;		

		case UMSG_TWC_ITEM_DO_FP:
			res_p->um_do_fp = *(int8_t *)p;
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
__encode_twc_hello(char *out_buf, uint32_t *out_len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__encode_twc_hello";
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
__decode_twc_hello(char *p, int len, struct umessage_twc_hdr *msghdr_p)
{
	char *log_header = "__decode_twc_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_TWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_TWC_HEADER_LEN;

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
umpk_encode_twc(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "encode_type_wc";
	struct umessage_twc_hdr *msghdr_p = (struct umessage_twc_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_TWC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_TWC_CODE_FLUSHING:
		case UMSG_TWC_CODE_THROUGHPUT_STAT:
		case UMSG_TWC_CODE_RW_STAT:
		case UMSG_TWC_CODE_STAT:
			rc = __encode_twc_information(out_buf, out_len, msghdr_p);
			break;

		case UMSG_TWC_CODE_RESP_STAT:
			rc = __encode_twc_information_resp_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_TWC_CODE_THROUGHPUT_STAT_RSLT:
			rc = __encode_twc_information_throughput_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_TWC_CODE_RW_STAT_RSLT:
			rc = __encode_twc_information_rw_stat(out_buf, out_len, msghdr_p);
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;


	case UMSG_TWC_CMD_THROUGHPUT:
		switch (msghdr_p->um_req_code) {
		case UMSG_TWC_CODE_THROUGHPUT_RESET:
			rc = __encode_twc_throughput(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TWC_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_TWC_CODE_S_DISP:
		case UMSG_TWC_CODE_S_DISP_ALL:
		case UMSG_TWC_CODE_W_DISP:
		case UMSG_TWC_CODE_W_DISP_ALL:
			rc = __encode_twc_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_TWC_CODE_S_RESP_DISP:
		case UMSG_TWC_CODE_S_RESP_DISP_ALL:
		case UMSG_TWC_CODE_W_RESP_DISP:
		case UMSG_TWC_CODE_W_RESP_DISP_ALL:
		case UMSG_TWC_CODE_DISP_END:
			rc = __encode_twc_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TWC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_TWC_CODE_HELLO:
		case UMSG_TWC_CODE_RESP_HELLO:
			rc = __encode_twc_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_twc(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_wc";
	struct umessage_twc_hdr *msghdr_p = (struct umessage_twc_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_TWC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_TWC_CODE_FLUSHING:
		case UMSG_TWC_CODE_THROUGHPUT_STAT:
		case UMSG_TWC_CODE_RW_STAT:
		case UMSG_TWC_CODE_STAT:
			rc = __decode_twc_information(p, len, msghdr_p);
			break;

		case UMSG_TWC_CODE_THROUGHPUT_STAT_RSLT:
			rc = __decode_twc_information_throughput_stat(p, len, msghdr_p);
			break;

		case UMSG_TWC_CODE_RESP_STAT:
			rc = __decode_twc_information_resp_stat(p, len, msghdr_p);
			break;

		case UMSG_TWC_CODE_RW_STAT_RSLT:
			rc = __decode_twc_information_rw_stat(p, len, msghdr_p);
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TWC_CMD_THROUGHPUT:
		switch (msghdr_p->um_req_code) {
		case UMSG_TWC_CODE_THROUGHPUT_RESET:
			rc = __decode_twc_throughput(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TWC_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_TWC_CODE_S_DISP:
		case UMSG_TWC_CODE_S_DISP_ALL:
		case UMSG_TWC_CODE_W_DISP:
		case UMSG_TWC_CODE_W_DISP_ALL:
			rc = __decode_twc_display(p, len, msghdr_p);
			break;

		case UMSG_TWC_CODE_S_RESP_DISP:
		case UMSG_TWC_CODE_S_RESP_DISP_ALL:
		case UMSG_TWC_CODE_W_RESP_DISP:
		case UMSG_TWC_CODE_W_RESP_DISP_ALL:
		case UMSG_TWC_CODE_DISP_END:
			rc = __decode_twc_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_TWC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_TWC_CODE_HELLO:
		case UMSG_TWC_CODE_RESP_HELLO:
			rc = -__decode_twc_hello(p, len, msghdr_p);
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
