/*
 * umpk_mrw.c
 * 	umpk for MDS Read Write
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_mrw_if.h"


static int
__encode_mrw_information(char *out_buf, uint32_t *out_len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__encode_mrw_information";
	struct umessage_mrw_information *msg_p = (struct umessage_mrw_information *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: start...",log_header);

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_MRW_ITEM_MRW;
	len++;
	sub_len = msg_p->um_mrw_name_len;
	*(uint8_t *)p = (uint8_t)sub_len;
	p ++;
	len ++;
	memcpy(p, msg_p->um_mrw_name, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}


static int
__encode_mrw_information_stat_resp(char *out_buf, uint32_t *out_len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__encode_mrw_information_stat_resp";
	struct umessage_mrw_information_stat_resp *msg_p = (struct umessage_mrw_information_stat_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0;


	nid_log_warning("%s: start...",log_header);

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
    p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_MRW_ITEM_WOP_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_wop_num;
	p += 8;
	len += 8;

	*p++ = UMSG_MRW_ITEM_WFP_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_wfp_num;
	p += 8;
	len += 8;

	*p++ = UMSG_MRW_ITEM_MRW;
	len++;
	*(uint16_t *)p = msg_p->um_seq_mrw;
	p += 2;
	len += 2;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}


static int
__decode_mrw_information(char *p, int len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__decode_mrw_information";
	struct umessage_mrw_information *msg_p = (struct umessage_mrw_information *)msghdr_p;
	int8_t item;
	int rc = 0;
	nid_log_warning("%s: start...",log_header);


	assert(len >= UMSG_MRW_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_MRW_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_MRW_ITEM_MRW:
			msg_p->um_mrw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_mrw_name, p, msg_p->um_mrw_name_len);
			msg_p->um_mrw_name[msg_p->um_mrw_name_len] = '\0';
			p += msg_p->um_mrw_name_len;
			len -= msg_p->um_mrw_name_len;
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
__decode_mrw_information_stat_resp(char *p, int len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__decode_mrw_information_stat_resp";
	struct umessage_mrw_information_stat_resp *res_p = (struct umessage_mrw_information_stat_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	nid_log_warning("%s: start...",log_header);

	assert(len >= UMSG_MRW_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_MRW_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		item = *p;
		len --;
		p ++;
		switch (item) {

		case UMSG_MRW_ITEM_WOP_NUM:
			nid_log_error("%s: got item UMSG_MRW_ITEM_WOP_NUM (%d)", log_header, item);
			res_p->um_seq_wop_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_MRW_ITEM_WFP_NUM:
			nid_log_error("%s: got item UMSG_MRW_ITEM_WFP_NUM (%d)", log_header, item);
			res_p->um_seq_wfp_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_MRW_ITEM_MRW:
			nid_log_error("%s: got item UMSG_MRW_ITEM_MRW (%d)", log_header, item);
			res_p->um_seq_mrw = *(int16_t *)p;
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
__encode_mrw_add(char *out_buf, uint32_t *out_len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__encode_mrw_add";
	struct umessage_mrw_add *msg_p = (struct umessage_mrw_add *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_mrw_name_len);
	/* request */
	sub_len = sizeof(msghdr_p->um_req);
	*(typeof(msghdr_p->um_req) *)p = msghdr_p->um_req;
	len += sub_len;
	p += sub_len;

	/* request code */
	sub_len = sizeof(msghdr_p->um_req_code);
	*(typeof(msghdr_p->um_req_code) *)p = msghdr_p->um_req_code;
	len += sub_len;
	p += sub_len;

	/* request length (skip it, will know the length at the end) */
	len_p = p;
	sub_len = sizeof(msghdr_p->um_len);
	len += sub_len;
        p += sub_len;

        /* mrw name */
        item = UMSG_MRW_ITEM_MRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_mrw_name_len);
	*(typeof(msg_p->um_mrw_name_len) *)p = msg_p->um_mrw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_mrw_name_len;
	memcpy(p, msg_p->um_mrw_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_mrw_add(char *p, int len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__decode_mrw_add";
	struct umessage_mrw_add *msg_p = (struct umessage_mrw_add *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_MRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_MRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_MRW_ITEM_MRW:
			msg_p->um_mrw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_mrw_name, p, msg_p->um_mrw_name_len);
			msg_p->um_mrw_name[msg_p->um_mrw_name_len] = '\0';
			p += msg_p->um_mrw_name_len;
			len -= msg_p->um_mrw_name_len;
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
__encode_mrw_add_resp(char *out_buf, uint32_t *out_len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__encode_mrw_add_resp";
	struct umessage_mrw_add_resp *msg_p = (struct umessage_mrw_add_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_mrw_name_len);
	/* request */
	sub_len = sizeof(msghdr_p->um_req);
	*(typeof(msghdr_p->um_req) *)p = msghdr_p->um_req;
	len += sub_len;
	p += sub_len;

	/* request code */
	sub_len = sizeof(msghdr_p->um_req_code);
	*(typeof(msghdr_p->um_req_code) *)p = msghdr_p->um_req_code;
	len += sub_len;
	p += sub_len;

	/* request length (skip it, will know the length at the end) */
	len_p = p;
	sub_len = sizeof(msghdr_p->um_len);
	len += sub_len;
        p += sub_len;

        /* mrw name */
        item = UMSG_MRW_ITEM_MRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_mrw_name_len);
	*(typeof(msg_p->um_mrw_name_len) *)p = msg_p->um_mrw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_mrw_name_len;
	memcpy(p, msg_p->um_mrw_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
        item = UMSG_MRW_ITEM_RESP_CODE;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_resp_code);
	*(typeof(msg_p->um_resp_code) *)p = msg_p->um_resp_code;
	p += sub_len;
	len += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_mrw_add_resp(char *p, int len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__decode_mrw_add_resp";
	struct umessage_mrw_add_resp *msg_p = (struct umessage_mrw_add_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_MRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_MRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_MRW_ITEM_MRW:
			msg_p->um_mrw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_mrw_name, p, msg_p->um_mrw_name_len);
			msg_p->um_mrw_name[msg_p->um_mrw_name_len] = '\0';
			p += msg_p->um_mrw_name_len;
			len -= msg_p->um_mrw_name_len;
			break;

		case UMSG_MRW_ITEM_RESP_CODE:
			msg_p->um_resp_code = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
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
__encode_mrw_display(char *out_buf, uint32_t *out_len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__encode_mrw_display";
	struct umessage_mrw_display *msg_p = (struct umessage_mrw_display *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_mrw_name_len);
	/* request */
	sub_len = sizeof(msghdr_p->um_req);
	*(typeof(msghdr_p->um_req) *)p = msghdr_p->um_req;
	len += sub_len;
	p += sub_len;

	/* request code */
	sub_len = sizeof(msghdr_p->um_req_code);
	*(typeof(msghdr_p->um_req_code) *)p = msghdr_p->um_req_code;
	len += sub_len;
	p += sub_len;

	/* request length (skip it, will know the length at the end) */
	len_p = p;
	sub_len = sizeof(msghdr_p->um_len);
	len += sub_len;
        p += sub_len;

        /* mrw name */
        item = UMSG_MRW_ITEM_MRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_mrw_name_len);
	*(typeof(msg_p->um_mrw_name_len) *)p = msg_p->um_mrw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_mrw_name_len;
	memcpy(p, msg_p->um_mrw_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_mrw_display(char *p, int len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__decode_mrw_display";
	struct umessage_mrw_display *msg_p = (struct umessage_mrw_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_MRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_MRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_MRW_ITEM_MRW:
			msg_p->um_mrw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_mrw_name, p, msg_p->um_mrw_name_len);
			msg_p->um_mrw_name[msg_p->um_mrw_name_len] = '\0';
			p += msg_p->um_mrw_name_len;
			len -= msg_p->um_mrw_name_len;
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
__encode_mrw_hello(char *out_buf, uint32_t *out_len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__encode_mrw_hello";
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
__decode_mrw_hello(char *p, int len, struct umessage_mrw_hdr *msghdr_p)
{
	char *log_header = "__decode_mrw_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_MRW_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_MRW_HEADER_LEN;

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
umpk_encode_mrw(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "encode_type_wc";
	struct umessage_mrw_hdr *msghdr_p = (struct umessage_mrw_hdr *)data;
	int rc;
	nid_log_warning("%s: start...",log_header);


	switch (msghdr_p->um_req) {

	case UMSG_MRW_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {

		case UMSG_MRW_CODE_STAT:
			rc = __encode_mrw_information(out_buf, out_len, msghdr_p);
			break;

		case UMSG_MRW_CODE_RESP_STAT:
			rc = __encode_mrw_information_stat_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_MRW_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_MRW_CODE_ADD:
			rc = __encode_mrw_add(out_buf, out_len, msghdr_p);
			break;

		case UMSG_MRW_CODE_RESP_ADD:
			__encode_mrw_add_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_MRW_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_MRW_CODE_S_DISP:
		case UMSG_MRW_CODE_S_DISP_ALL:
		case UMSG_MRW_CODE_W_DISP:
		case UMSG_MRW_CODE_W_DISP_ALL:
		case UMSG_MRW_CODE_S_RESP_DISP:
		case UMSG_MRW_CODE_S_RESP_DISP_ALL:
		case UMSG_MRW_CODE_W_RESP_DISP:
		case UMSG_MRW_CODE_W_RESP_DISP_ALL:
		case UMSG_MRW_CODE_DISP_END:
			rc = __encode_mrw_display(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_MRW_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_MRW_CODE_HELLO:
		case UMSG_MRW_CODE_RESP_HELLO:
			rc = __encode_mrw_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_mrw(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_wc";
	struct umessage_mrw_hdr *msghdr_p = (struct umessage_mrw_hdr *)data;
	int rc = 0;

	nid_log_warning("%s: start...",log_header);

	switch (msghdr_p->um_req) {

	case UMSG_MRW_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_MRW_CODE_STAT:
			rc = __decode_mrw_information(p, len, msghdr_p);
			break;

		case UMSG_MRW_CODE_RESP_STAT:
			rc = __decode_mrw_information_stat_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_MRW_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_MRW_CODE_ADD:
			rc = __decode_mrw_add(p, len, msghdr_p);
			break;

		case UMSG_MRW_CODE_RESP_ADD:
			__decode_mrw_add_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_MRW_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_MRW_CODE_S_DISP:
		case UMSG_MRW_CODE_S_DISP_ALL:
		case UMSG_MRW_CODE_W_DISP:
		case UMSG_MRW_CODE_W_DISP_ALL:
		case UMSG_MRW_CODE_S_RESP_DISP:
		case UMSG_MRW_CODE_S_RESP_DISP_ALL:
		case UMSG_MRW_CODE_W_RESP_DISP:
		case UMSG_MRW_CODE_W_RESP_DISP_ALL:
			rc = __decode_mrw_display(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmm code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_MRW_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_MRW_CODE_HELLO:
		case UMSG_MRW_CODE_RESP_HELLO:
			rc = __decode_mrw_hello(p, len, msghdr_p);
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
