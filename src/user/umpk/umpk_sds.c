/*
 * umpk_sds.c
 * 	umpk for Split Data Stream
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_sds_if.h"

static int
__encode_sds_add(char *out_buf, uint32_t *out_len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__encode_sds_add";
	struct umessage_sds_add *msg_p = (struct umessage_sds_add *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_sds_name_len);
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

        /* sds name */
        item = UMSG_SDS_ITEM_SDS;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_sds_name_len);
	*(typeof(msg_p->um_sds_name_len) *)p = msg_p->um_sds_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_sds_name_len;
	memcpy(p, msg_p->um_sds_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* sds pp name */
        item = UMSG_SDS_ITEM_PP;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pp_name_len);
	*(typeof(msg_p->um_pp_name_len) *)p = msg_p->um_pp_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_pp_name_len;
	memcpy(p, msg_p->um_pp_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* sds wc name */
        item = UMSG_SDS_ITEM_WC;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_wc_uuid_len);
	*(typeof(msg_p->um_wc_uuid_len) *)p = msg_p->um_wc_uuid_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_wc_uuid_len;
	memcpy(p, msg_p->um_wc_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* sds rc name */
        item = UMSG_SDS_ITEM_RC;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rc_uuid_len);
	*(typeof(msg_p->um_rc_uuid_len) *)p = msg_p->um_rc_uuid_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_rc_uuid_len;
	memcpy(p, msg_p->um_rc_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sds_add(char *p, int len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__decode_sds_add";
	struct umessage_sds_add *msg_p = (struct umessage_sds_add *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SDS_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SDS_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SDS_ITEM_SDS:
			msg_p->um_sds_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_sds_name, p, msg_p->um_sds_name_len);
			msg_p->um_sds_name[msg_p->um_sds_name_len] = '\0';
			p += msg_p->um_sds_name_len;
			len -= msg_p->um_sds_name_len;
			break;

		case UMSG_SDS_ITEM_PP:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
			break;

		case UMSG_SDS_ITEM_WC:
			msg_p->um_wc_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_wc_uuid, p, msg_p->um_wc_uuid_len);
			msg_p->um_wc_uuid[msg_p->um_wc_uuid_len] = 0;
			p += msg_p->um_wc_uuid_len;
			len -= msg_p->um_wc_uuid_len;
			break;

		case UMSG_SDS_ITEM_RC:
			msg_p->um_rc_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_rc_uuid, p, msg_p->um_rc_uuid_len);
			msg_p->um_rc_uuid[msg_p->um_rc_uuid_len] = 0;
			p += msg_p->um_rc_uuid_len;
			len -= msg_p->um_rc_uuid_len;
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
__encode_sds_add_resp(char *out_buf, uint32_t *out_len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__encode_sds_add_resp";
	struct umessage_sds_add_resp *msg_p = (struct umessage_sds_add_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_sds_name_len);
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

        /* sds name */
        item = UMSG_SDS_ITEM_SDS;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_sds_name_len);
	*(typeof(msg_p->um_sds_name_len) *)p = msg_p->um_sds_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_sds_name_len;
	memcpy(p, msg_p->um_sds_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
        item = UMSG_SDS_ITEM_RESP_CODE;
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
__decode_sds_add_resp(char *p, int len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__decode_sds_add_resp";
	struct umessage_sds_add_resp *msg_p = (struct umessage_sds_add_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SDS_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SDS_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SDS_ITEM_SDS:
			msg_p->um_sds_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_sds_name, p, msg_p->um_sds_name_len);
			msg_p->um_sds_name[msg_p->um_sds_name_len] = '\0';
			p += msg_p->um_sds_name_len;
			len -= msg_p->um_sds_name_len;
			break;

		case UMSG_SDS_ITEM_RESP_CODE:
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
__encode_sds_delete(char *out_buf, uint32_t *out_len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__encode_sds_delete";
	struct umessage_sds_delete *msg_p = (struct umessage_sds_delete *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_sds_name_len);
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

        /* sds name */
        item = UMSG_SDS_ITEM_SDS;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_sds_name_len);
	*(typeof(msg_p->um_sds_name_len) *)p = msg_p->um_sds_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_sds_name_len;
	memcpy(p, msg_p->um_sds_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sds_delete(char *p, int len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__decode_sds_delete";
	struct umessage_sds_delete *msg_p = (struct umessage_sds_delete *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SDS_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SDS_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SDS_ITEM_SDS:
			msg_p->um_sds_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_sds_name, p, msg_p->um_sds_name_len);
			msg_p->um_sds_name[msg_p->um_sds_name_len] = '\0';
			p += msg_p->um_sds_name_len;
			len -= msg_p->um_sds_name_len;
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
__encode_sds_delete_resp(char *out_buf, uint32_t *out_len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__encode_sds_delete_resp";
	struct umessage_sds_delete_resp *msg_p = (struct umessage_sds_delete_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_sds_name_len);
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

        /* sds name */
        item = UMSG_SDS_ITEM_SDS;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_sds_name_len);
	*(typeof(msg_p->um_sds_name_len) *)p = msg_p->um_sds_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_sds_name_len;
	memcpy(p, msg_p->um_sds_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
        item = UMSG_SDS_ITEM_RESP_CODE;
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
__decode_sds_delete_resp(char *p, int len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__decode_sds_delete_resp";
	struct umessage_sds_delete_resp *msg_p = (struct umessage_sds_delete_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SDS_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SDS_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SDS_ITEM_SDS:
			msg_p->um_sds_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_sds_name, p, msg_p->um_sds_name_len);
			msg_p->um_sds_name[msg_p->um_sds_name_len] = '\0';
			p += msg_p->um_sds_name_len;
			len -= msg_p->um_sds_name_len;
			break;

		case UMSG_SDS_ITEM_RESP_CODE:
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
__encode_sds_display(char *out_buf, uint32_t *out_len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__encode_sds_display";
	struct umessage_sds_display *msg_p = (struct umessage_sds_display *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_sds_name_len);
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

        /* sds name */
        item = UMSG_SDS_ITEM_SDS;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_sds_name_len);
	*(typeof(msg_p->um_sds_name_len) *)p = msg_p->um_sds_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_sds_name_len;
	memcpy(p, msg_p->um_sds_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sds_display(char *p, int len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__decode_sds_display";
	struct umessage_sds_display *msg_p = (struct umessage_sds_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SDS_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SDS_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SDS_ITEM_SDS:
			msg_p->um_sds_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_sds_name, p, msg_p->um_sds_name_len);
			msg_p->um_sds_name[msg_p->um_sds_name_len] = '\0';
			p += msg_p->um_sds_name_len;
			len -= msg_p->um_sds_name_len;
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
__encode_sds_display_resp(char *out_buf, uint32_t *out_len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__encode_sds_display_resp";
	struct umessage_sds_display_resp *msg_p = (struct umessage_sds_display_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_sds_name_len);
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

        /* sds name */
        item = UMSG_SDS_ITEM_SDS;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_sds_name_len);
	*(typeof(msg_p->um_sds_name_len) *)p = msg_p->um_sds_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_sds_name_len;
	memcpy(p, msg_p->um_sds_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* sds pp name */
        item = UMSG_SDS_ITEM_PP;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pp_name_len);
	*(typeof(msg_p->um_pp_name_len) *)p = msg_p->um_pp_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_pp_name_len;
	memcpy(p, msg_p->um_pp_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* sds wc name */
        item = UMSG_SDS_ITEM_WC;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_wc_uuid_len);
	*(typeof(msg_p->um_wc_uuid_len) *)p = msg_p->um_wc_uuid_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_wc_uuid_len;
	memcpy(p, msg_p->um_wc_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* sds rc name */
        item = UMSG_SDS_ITEM_RC;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rc_uuid_len);
	*(typeof(msg_p->um_rc_uuid_len) *)p = msg_p->um_rc_uuid_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_rc_uuid_len;
	memcpy(p, msg_p->um_rc_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sds_display_resp(char *p, int len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__decode_sds_display_resp";
	struct umessage_sds_display_resp *msg_p = (struct umessage_sds_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SDS_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SDS_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SDS_ITEM_SDS:
			msg_p->um_sds_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_sds_name, p, msg_p->um_sds_name_len);
			msg_p->um_sds_name[msg_p->um_sds_name_len] = '\0';
			p += msg_p->um_sds_name_len;
			len -= msg_p->um_sds_name_len;
			break;

		case UMSG_SDS_ITEM_PP:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
			break;

		case UMSG_SDS_ITEM_WC:
			msg_p->um_wc_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_wc_uuid, p, msg_p->um_wc_uuid_len);
			msg_p->um_wc_uuid[msg_p->um_wc_uuid_len] = 0;
			p += msg_p->um_wc_uuid_len;
			len -= msg_p->um_wc_uuid_len;
			break;

		case UMSG_SDS_ITEM_RC:
			msg_p->um_rc_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_rc_uuid, p, msg_p->um_rc_uuid_len);
			msg_p->um_rc_uuid[msg_p->um_rc_uuid_len] = 0;
			p += msg_p->um_rc_uuid_len;
			len -= msg_p->um_rc_uuid_len;
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
__encode_sds_hello(char *out_buf, uint32_t *out_len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__encode_sds_hello";
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
__decode_sds_hello(char *p, int len, struct umessage_sds_hdr *msghdr_p)
{
	char *log_header = "__decode_sds_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SDS_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_SDS_HEADER_LEN;

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
umpk_encode_sds(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_sds";
	struct umessage_sds_hdr *msghdr_p = (struct umessage_sds_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_SDS_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_SDS_CODE_ADD:
			rc = __encode_sds_add(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SDS_CODE_RESP_ADD:
			rc = __encode_sds_add_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SDS_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_SDS_CODE_DELETE:
			rc = __encode_sds_delete(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SDS_CODE_RESP_DELETE:
			rc = __encode_sds_delete_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SDS_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_SDS_CODE_S_DISP:
		case UMSG_SDS_CODE_S_DISP_ALL:
		case UMSG_SDS_CODE_W_DISP:
		case UMSG_SDS_CODE_W_DISP_ALL:
			rc = __encode_sds_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SDS_CODE_S_RESP_DISP:
		case UMSG_SDS_CODE_S_RESP_DISP_ALL:
		case UMSG_SDS_CODE_W_RESP_DISP:
		case UMSG_SDS_CODE_W_RESP_DISP_ALL:
		case UMSG_SDS_CODE_DISP_END:
			rc = __encode_sds_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SDS_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_SDS_CODE_HELLO:
		case UMSG_SDS_CODE_RESP_HELLO:
			rc = __encode_sds_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_sds(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_sds";
	struct umessage_sds_hdr *msghdr_p = (struct umessage_sds_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_SDS_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_SDS_CODE_ADD:
			rc = __decode_sds_add(p, len, msghdr_p);
			break;

		case UMSG_SDS_CODE_RESP_ADD:
			__decode_sds_add_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SDS_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_SDS_CODE_DELETE:
			rc = __decode_sds_delete(p, len, msghdr_p);
			break;

		case UMSG_SDS_CODE_RESP_DELETE:
			rc = __decode_sds_delete_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SDS_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_SDS_CODE_S_DISP:
		case UMSG_SDS_CODE_S_DISP_ALL:
		case UMSG_SDS_CODE_W_DISP:
		case UMSG_SDS_CODE_W_DISP_ALL:
			rc = __decode_sds_display(p, len, msghdr_p);
			break;

		case UMSG_SDS_CODE_S_RESP_DISP:
		case UMSG_SDS_CODE_S_RESP_DISP_ALL:
		case UMSG_SDS_CODE_W_RESP_DISP:
		case UMSG_SDS_CODE_W_RESP_DISP_ALL:
		case UMSG_SDS_CODE_DISP_END:
			rc = __decode_sds_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;	
		}
		break;

	case UMSG_SDS_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_SDS_CODE_HELLO:
		case UMSG_SDS_CODE_RESP_HELLO:
			rc = __decode_sds_hello(p, len, msghdr_p);
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
