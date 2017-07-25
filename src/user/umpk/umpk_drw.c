/*
 * umpk_drw.c
 * 	umpk for Device Read Write
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_drw_if.h"

static int
__encode_drw_add(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_add";
	struct umessage_drw_add *msg_p = (struct umessage_drw_add *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_drw_name_len);
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

        /* drw name */
        item = UMSG_DRW_ITEM_DRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_drw_name_len);
	*(typeof(msg_p->um_drw_name_len) *)p = msg_p->um_drw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_drw_name_len;
	memcpy(p, msg_p->um_drw_name, sub_len);
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_exportname_len);
	*(typeof(msg_p->um_exportname_len) *)p = msg_p->um_exportname_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_exportname_len;
	memcpy(p, msg_p->um_exportname, sub_len);
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_device_provision);
	*(typeof(msg_p->um_device_provision) *)p = msg_p->um_device_provision;
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_drw_add(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_add";
	struct umessage_drw_add *msg_p = (struct umessage_drw_add *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_DRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_DRW_ITEM_DRW:
			msg_p->um_drw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_drw_name, p, msg_p->um_drw_name_len);
			msg_p->um_drw_name[msg_p->um_drw_name_len] = '\0';
			p += msg_p->um_drw_name_len;
			len -= msg_p->um_drw_name_len;

			msg_p->um_exportname_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_exportname, p, msg_p->um_exportname_len);
			msg_p->um_exportname[msg_p->um_exportname_len] = '\0';
			p += msg_p->um_exportname_len;
			len -= msg_p->um_exportname_len;

			msg_p->um_device_provision = *(int8_t *)p;
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
__encode_drw_add_resp(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_add_resp";
	struct umessage_drw_add_resp *msg_p = (struct umessage_drw_add_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_drw_name_len);
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

        /* drw name */
        item = UMSG_DRW_ITEM_DRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_drw_name_len);
	*(typeof(msg_p->um_drw_name_len) *)p = msg_p->um_drw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_drw_name_len;
	memcpy(p, msg_p->um_drw_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
        item = UMSG_DRW_ITEM_RESP_CODE;
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
__decode_drw_add_resp(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_add_resp";
	struct umessage_drw_add_resp *msg_p = (struct umessage_drw_add_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_DRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_DRW_ITEM_DRW:
			msg_p->um_drw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_drw_name, p, msg_p->um_drw_name_len);
			msg_p->um_drw_name[msg_p->um_drw_name_len] = '\0';
			p += msg_p->um_drw_name_len;
			len -= msg_p->um_drw_name_len;
			break;

		case UMSG_DRW_ITEM_RESP_CODE:
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
__encode_drw_delete(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_delete";
	struct umessage_drw_delete *msg_p = (struct umessage_drw_delete *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_drw_name_len);
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

        /* drw name */
        item = UMSG_DRW_ITEM_DRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_drw_name_len);
	*(typeof(msg_p->um_drw_name_len) *)p = msg_p->um_drw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_drw_name_len;
	memcpy(p, msg_p->um_drw_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_drw_delete(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_delete";
	struct umessage_drw_delete *msg_p = (struct umessage_drw_delete *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_DRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_DRW_ITEM_DRW:
			msg_p->um_drw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_drw_name, p, msg_p->um_drw_name_len);
			msg_p->um_drw_name[msg_p->um_drw_name_len] = '\0';
			p += msg_p->um_drw_name_len;
			len -= msg_p->um_drw_name_len;

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
__encode_drw_delete_resp(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_delete_resp";
	struct umessage_drw_delete_resp *msg_p = (struct umessage_drw_delete_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_drw_name_len);
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

        /* drw name */
        item = UMSG_DRW_ITEM_DRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_drw_name_len);
	*(typeof(msg_p->um_drw_name_len) *)p = msg_p->um_drw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_drw_name_len;
	memcpy(p, msg_p->um_drw_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
        item = UMSG_DRW_ITEM_RESP_CODE;
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
__decode_drw_delete_resp(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_delete_resp";
	struct umessage_drw_delete_resp *msg_p = (struct umessage_drw_delete_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_DRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_DRW_ITEM_DRW:
			msg_p->um_drw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_drw_name, p, msg_p->um_drw_name_len);
			msg_p->um_drw_name[msg_p->um_drw_name_len] = '\0';
			p += msg_p->um_drw_name_len;
			len -= msg_p->um_drw_name_len;
			break;

		case UMSG_DRW_ITEM_RESP_CODE:
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
__encode_drw_ready(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_ready";
	struct umessage_drw_ready *msg_p = (struct umessage_drw_ready *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_drw_name_len);
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

        /* drw name */
        item = UMSG_DRW_ITEM_DRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_drw_name_len);
	*(typeof(msg_p->um_drw_name_len) *)p = msg_p->um_drw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_drw_name_len;
	memcpy(p, msg_p->um_drw_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_drw_ready(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_ready";
	struct umessage_drw_ready *msg_p = (struct umessage_drw_ready *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_DRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_DRW_ITEM_DRW:
			msg_p->um_drw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_drw_name, p, msg_p->um_drw_name_len);
			msg_p->um_drw_name[msg_p->um_drw_name_len] = '\0';
			p += msg_p->um_drw_name_len;
			len -= msg_p->um_drw_name_len;
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
__encode_drw_ready_resp(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_ready_resp";
	struct umessage_drw_ready_resp *msg_p = (struct umessage_drw_ready_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_drw_name_len);
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

        /* drw name */
        item = UMSG_DRW_ITEM_DRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_drw_name_len);
	*(typeof(msg_p->um_drw_name_len) *)p = msg_p->um_drw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_drw_name_len;
	memcpy(p, msg_p->um_drw_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
        item = UMSG_DRW_ITEM_RESP_CODE;
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
__decode_drw_ready_resp(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_ready_resp";
	struct umessage_drw_ready_resp *msg_p = (struct umessage_drw_ready_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_DRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_DRW_ITEM_DRW:
			msg_p->um_drw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_drw_name, p, msg_p->um_drw_name_len);
			msg_p->um_drw_name[msg_p->um_drw_name_len] = '\0';
			p += msg_p->um_drw_name_len;
			len -= msg_p->um_drw_name_len;
			break;

		case UMSG_DRW_ITEM_RESP_CODE:
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
__encode_drw_display(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_display";
	struct umessage_drw_display *msg_p = (struct umessage_drw_display *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_drw_name_len);
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

        /* drw name */
        item = UMSG_DRW_ITEM_DRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_drw_name_len);
	*(typeof(msg_p->um_drw_name_len) *)p = msg_p->um_drw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_drw_name_len;
	memcpy(p, msg_p->um_drw_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_drw_display(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_display";
	struct umessage_drw_display *msg_p = (struct umessage_drw_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_DRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_DRW_ITEM_DRW:
			msg_p->um_drw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_drw_name, p, msg_p->um_drw_name_len);
			msg_p->um_drw_name[msg_p->um_drw_name_len] = '\0';
			p += msg_p->um_drw_name_len;
			len -= msg_p->um_drw_name_len;
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
__encode_drw_display_resp(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_display_resp";
	struct umessage_drw_display_resp *msg_p = (struct umessage_drw_display_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_drw_name_len);
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

        /* drw name */
        item = UMSG_DRW_ITEM_DRW;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_drw_name_len);
	*(typeof(msg_p->um_drw_name_len) *)p = msg_p->um_drw_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_drw_name_len;
	memcpy(p, msg_p->um_drw_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* exportname name */
        item = UMSG_DRW_ITEM_EXPORTNAME;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_exportname_len);
	*(typeof(msg_p->um_exportname_len) *)p = msg_p->um_exportname_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_exportname_len;
	memcpy(p, msg_p->um_exportname, sub_len);
	len += sub_len;
	p += sub_len;

	/* simulate */
        item = UMSG_DRW_ITEM_SIMULATE;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_simulate_async);
	*(typeof(msg_p->um_simulate_async) *)p = msg_p->um_simulate_async;
	p += sub_len;
	len += sub_len;

	sub_len = sizeof(msg_p->um_simulate_delay);
	*(typeof(msg_p->um_simulate_delay) *)p = msg_p->um_simulate_delay;
	p += sub_len;
	len += sub_len;

	sub_len = sizeof(msg_p->um_simulate_delay_min_gap);
	*(typeof(msg_p->um_simulate_delay_min_gap) *)p = msg_p->um_simulate_delay_min_gap;
	p += sub_len;
	len += sub_len;

	sub_len = sizeof(msg_p->um_simulate_delay_max_gap);
	*(typeof(msg_p->um_simulate_delay_max_gap) *)p = msg_p->um_simulate_delay_max_gap;
	p += sub_len;
	len += sub_len;

	sub_len = sizeof(msg_p->um_simulate_delay_time_us);
	*(typeof(msg_p->um_simulate_delay_time_us) *)p = msg_p->um_simulate_delay_time_us;
	p += sub_len;
	len += sub_len;

	/* provision */
	item = UMSG_DRW_ITEM_PROVISION;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_device_provision);
	*(typeof(msg_p->um_device_provision) *)p = msg_p->um_device_provision;
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_drw_display_resp(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_display_resp";
	struct umessage_drw_display_resp *msg_p = (struct umessage_drw_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_DRW_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_DRW_ITEM_DRW:
			msg_p->um_drw_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_drw_name, p, msg_p->um_drw_name_len);
			msg_p->um_drw_name[msg_p->um_drw_name_len] = '\0';
			p += msg_p->um_drw_name_len;
			len -= msg_p->um_drw_name_len;
			break;

		case UMSG_DRW_ITEM_EXPORTNAME:
			msg_p->um_exportname_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_exportname, p, msg_p->um_exportname_len);
			msg_p->um_exportname[msg_p->um_exportname_len] = '\0';
			p += msg_p->um_exportname_len;
			len -= msg_p->um_exportname_len;
			break;

		case UMSG_DRW_ITEM_SIMULATE:
			msg_p->um_simulate_async = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);

			msg_p->um_simulate_delay = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);

			msg_p->um_simulate_delay_min_gap = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);

			msg_p->um_simulate_delay_max_gap = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);

			msg_p->um_simulate_delay_time_us = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			break;

		case UMSG_DRW_ITEM_PROVISION:
			msg_p->um_device_provision = *(int8_t *)p;
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
__encode_drw_hello(char *out_buf, uint32_t *out_len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__encode_drw_hello";
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
__decode_drw_hello(char *p, int len, struct umessage_drw_hdr *msghdr_p)
{
	char *log_header = "__decode_drw_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DRW_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_DRW_HEADER_LEN;

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
umpk_encode_drw(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_drw";
	struct umessage_drw_hdr *msghdr_p = (struct umessage_drw_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_DRW_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_ADD:
			rc = __encode_drw_add(out_buf, out_len, msghdr_p);
			break;

		case UMSG_DRW_CODE_RESP_ADD:
			rc = __encode_drw_add_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DRW_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_DELETE:
			rc = __encode_drw_delete(out_buf, out_len, msghdr_p);
			break;

		case UMSG_DRW_CODE_RESP_ADD:
			rc = __encode_drw_delete_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DRW_CMD_READY:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_READY:
			rc = __encode_drw_ready(out_buf, out_len, msghdr_p);
			break;

		case UMSG_DRW_CODE_RESP_READY:
			rc = __encode_drw_ready_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DRW_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_S_DISP:
		case UMSG_DRW_CODE_S_DISP_ALL:
		case UMSG_DRW_CODE_W_DISP:
		case UMSG_DRW_CODE_W_DISP_ALL:
			rc = __encode_drw_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_DRW_CODE_S_RESP_DISP:
		case UMSG_DRW_CODE_S_RESP_DISP_ALL:
		case UMSG_DRW_CODE_W_RESP_DISP:
		case UMSG_DRW_CODE_W_RESP_DISP_ALL:
		case UMSG_DRW_CODE_DISP_END:
			rc = __encode_drw_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DRW_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_HELLO:
		case UMSG_DRW_CODE_RESP_HELLO:
			rc = __encode_drw_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_drw(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_drw";
	struct umessage_drw_hdr *msghdr_p = (struct umessage_drw_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_DRW_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_ADD:
			rc = __decode_drw_add(p, len, msghdr_p);
			break;

		case UMSG_DRW_CODE_RESP_ADD:
			rc = __decode_drw_add_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DRW_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_DELETE:
			rc = __decode_drw_delete(p, len, msghdr_p);
			break;

		case UMSG_DRW_CODE_RESP_DELETE:
			rc = __decode_drw_delete_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DRW_CMD_READY:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_READY:
			rc = __decode_drw_ready(p, len, msghdr_p);
			break;

		case UMSG_DRW_CODE_RESP_READY:
			rc = __decode_drw_ready_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DRW_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_S_DISP:
		case UMSG_DRW_CODE_S_DISP_ALL:
		case UMSG_DRW_CODE_W_DISP:
		case UMSG_DRW_CODE_W_DISP_ALL:
			rc = __decode_drw_display(p, len, msghdr_p);
			break;

		case UMSG_DRW_CODE_S_RESP_DISP:
		case UMSG_DRW_CODE_S_RESP_DISP_ALL:
		case UMSG_DRW_CODE_W_RESP_DISP:
		case UMSG_DRW_CODE_W_RESP_DISP_ALL:
		case UMSG_DRW_CODE_DISP_END:
			rc = __decode_drw_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DRW_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_DRW_CODE_HELLO:
		case UMSG_DRW_CODE_RESP_HELLO:
			rc = __decode_drw_hello(p, len, msghdr_p);
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
