/*
 * umpk_pp.c
 * 	umpk for Page Pool
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_pp_if.h"

static int
__encode_pp_add(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_add";
	struct umessage_pp_add *msg_p = (struct umessage_pp_add *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_pp_name_len);
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

	/* pp name */
        item = UMSG_PP_ITEM_NAME;
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

	/* set id */
        item = UMSG_PP_ITEM_SET;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_set_id);
	*(typeof(msg_p->um_set_id) *)p = msg_p->um_set_id;
	len += sub_len;
	p += sub_len;

	/* pool size */
        item = UMSG_PP_ITEM_POOL_SZ;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pool_size);
	*(typeof(msg_p->um_pool_size) *)p = msg_p->um_pool_size;
	len += sub_len;
	p += sub_len;

	/* page size */
        item = UMSG_PP_ITEM_PAGE_SZ;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_page_size);
	*(typeof(msg_p->um_page_size) *)p = msg_p->um_page_size;
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_pp_add(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_add";
	struct umessage_pp_add *msg_p = (struct umessage_pp_add *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_PP_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_PP_ITEM_NAME:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
			break;

		case UMSG_PP_ITEM_SET:
			msg_p->um_set_id = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			break;

		case UMSG_PP_ITEM_POOL_SZ:
			msg_p->um_pool_size = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_PP_ITEM_PAGE_SZ:
			msg_p->um_page_size = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
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
__encode_pp_add_resp(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_add_resp";
	struct umessage_pp_add_resp *msg_p = (struct umessage_pp_add_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_pp_name_len);
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

        /* pp name */
        item = UMSG_PP_ITEM_NAME;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pp_name_len);
	*(typeof(msg_p->um_pp_name_len) *)p = msg_p->um_pp_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_pp_name_len;
	memcpy(p, msg_p->um_pp_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
        item = UMSG_PP_ITEM_RESP_CODE;
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
__decode_pp_add_resp(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_add_resp";
	struct umessage_pp_add_resp *msg_p = (struct umessage_pp_add_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_PP_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_PP_ITEM_NAME:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
			break;

		case UMSG_PP_ITEM_RESP_CODE:
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
__encode_pp_delete(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_delete";
	struct umessage_pp_delete *msg_p = (struct umessage_pp_delete *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_pp_name_len);
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

	/* pp name */
        item = UMSG_PP_ITEM_NAME;
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

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_pp_delete(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_delete";
	struct umessage_pp_delete *msg_p = (struct umessage_pp_delete *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_PP_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_PP_ITEM_NAME:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
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
__encode_pp_delete_resp(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_delete_resp";
	struct umessage_pp_delete_resp *msg_p = (struct umessage_pp_delete_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_pp_name_len);
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

        /* pp name */
        item = UMSG_PP_ITEM_NAME;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pp_name_len);
	*(typeof(msg_p->um_pp_name_len) *)p = msg_p->um_pp_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_pp_name_len;
	memcpy(p, msg_p->um_pp_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
        item = UMSG_PP_ITEM_RESP_CODE;
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
__decode_pp_delete_resp(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_delete_resp";
	struct umessage_pp_delete_resp *msg_p = (struct umessage_pp_delete_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_PP_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_PP_ITEM_NAME:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
			break;

		case UMSG_PP_ITEM_RESP_CODE:
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
__encode_pp_stat(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_stat";
	struct umessage_pp_stat *msg_p = (struct umessage_pp_stat *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_pp_name_len);
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

        /* pp name */
        item = UMSG_PP_ITEM_NAME;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pp_name_len);
	*(typeof(msg_p->um_pp_name_len) *)p = msg_p->um_pp_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_pp_name_len;
	memcpy(p, msg_p->um_pp_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_pp_stat(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_stat";
	struct umessage_pp_stat *msg_p = (struct umessage_pp_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_PP_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_PP_ITEM_NAME:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
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
__encode_pp_stat_resp(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_stat_resp";
	struct umessage_pp_stat_resp *msg_p = (struct umessage_pp_stat_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_pp_name_len);
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

        /* pp name */
        item = UMSG_PP_ITEM_NAME;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pp_name_len);
	*(typeof(msg_p->um_pp_name_len) *)p = msg_p->um_pp_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_pp_name_len;
	memcpy(p, msg_p->um_pp_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* pp stat */
	item = UMSG_PP_ITEM_STAT;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pp_poolsz);
	*(typeof(msg_p->um_pp_poolsz) *)p = msg_p->um_pp_poolsz;
	p += sub_len;
	len += sub_len;
	
	sub_len = sizeof(msg_p->um_pp_pagesz);
	*(typeof(msg_p->um_pp_pagesz) *)p = msg_p->um_pp_pagesz;
	p += sub_len;
	len += sub_len;
	
	sub_len = sizeof(msg_p->um_pp_poollen);
	*(typeof(msg_p->um_pp_poollen) *)p = msg_p->um_pp_poollen;
	p += sub_len;
	len += sub_len;
	
	sub_len = sizeof(msg_p->um_pp_nfree);
	*(typeof(msg_p->um_pp_nfree) *)p = msg_p->um_pp_nfree;
	p += sub_len;
	len += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_pp_stat_resp(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_stat_resp";
	struct umessage_pp_stat_resp *msg_p = (struct umessage_pp_stat_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_PP_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_PP_ITEM_NAME:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
			break;

		case UMSG_PP_ITEM_STAT:
			msg_p->um_pp_poolsz = *(uint32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);

			msg_p->um_pp_pagesz = *(uint32_t *)p;
			p += sizeof(int32_t);
                        len -= sizeof(int32_t);

			msg_p->um_pp_poollen = *(uint32_t *)p;
			p += sizeof(int32_t);
                        len -= sizeof(int32_t);

			msg_p->um_pp_nfree = *(uint32_t *)p;
			p += sizeof(int32_t);
                        len -= sizeof(int32_t);
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
__encode_pp_display(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_display";
	struct umessage_pp_display *msg_p = (struct umessage_pp_display *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_pp_name_len);
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

        /* pp name */
        item = UMSG_PP_ITEM_NAME;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pp_name_len);
	*(typeof(msg_p->um_pp_name_len) *)p = msg_p->um_pp_name_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_pp_name_len;
	memcpy(p, msg_p->um_pp_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_pp_display(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_display";
	struct umessage_pp_display *msg_p = (struct umessage_pp_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_PP_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_PP_ITEM_NAME:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
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
__encode_pp_display_resp(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_display_resp";
	struct umessage_pp_display_resp *msg_p = (struct umessage_pp_display_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: name_len:%d", log_header, msg_p->um_pp_name_len);
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

	/* pp name */
        item = UMSG_PP_ITEM_NAME;
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

	/* set id */
        item = UMSG_PP_ITEM_SET;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_set_id);
	*(typeof(msg_p->um_set_id) *)p = msg_p->um_set_id;
	len += sub_len;
	p += sub_len;

	/* pool size */
        item = UMSG_PP_ITEM_POOL_SZ;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_pool_size);
	*(typeof(msg_p->um_pool_size) *)p = msg_p->um_pool_size;
	len += sub_len;
	p += sub_len;

	/* page size */
        item = UMSG_PP_ITEM_PAGE_SZ;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_page_size);
	*(typeof(msg_p->um_page_size) *)p = msg_p->um_page_size;
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_pp_display_resp(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_display_resp";
	struct umessage_pp_display_resp *msg_p = (struct umessage_pp_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_PP_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_PP_ITEM_NAME:
			msg_p->um_pp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_pp_name, p, msg_p->um_pp_name_len);
			msg_p->um_pp_name[msg_p->um_pp_name_len] = '\0';
			p += msg_p->um_pp_name_len;
			len -= msg_p->um_pp_name_len;
			break;

		case UMSG_PP_ITEM_SET:
			msg_p->um_set_id = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			break;

		case UMSG_PP_ITEM_POOL_SZ:
			msg_p->um_pool_size = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_PP_ITEM_PAGE_SZ:
			msg_p->um_page_size = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
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
__encode_pp_hello(char *out_buf, uint32_t *out_len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__encode_pp_hello";
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
__decode_pp_hello(char *p, int len, struct umessage_pp_hdr *msghdr_p)
{
	char *log_header = "__decode_pp_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_PP_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_PP_HEADER_LEN;

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
umpk_encode_pp(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_pp";
	struct umessage_pp_hdr *msghdr_p = (struct umessage_pp_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_PP_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_ADD:
			rc = __encode_pp_add(out_buf, out_len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_ADD:
			rc = __encode_pp_add_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_PP_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_DELETE:
			rc = __encode_pp_delete(out_buf, out_len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_DELETE:
			rc = __encode_pp_delete_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_PP_CMD_STAT:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_STAT:
			rc = __encode_pp_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_STAT:
			rc = __encode_pp_stat_resp(out_buf, out_len, msghdr_p);
			break;

		case UMSG_PP_CODE_STAT_ALL:
			rc = __encode_pp_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_STAT_ALL:
			rc = __encode_pp_stat_resp(out_buf, out_len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_END:
			rc = __encode_pp_stat_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
		}
		break;

	case UMSG_PP_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_S_DISP:
		case UMSG_PP_CODE_S_DISP_ALL:
		case UMSG_PP_CODE_W_DISP:
		case UMSG_PP_CODE_W_DISP_ALL:
			rc = __encode_pp_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_PP_CODE_S_RESP_DISP:
		case UMSG_PP_CODE_S_RESP_DISP_ALL:
		case UMSG_PP_CODE_W_RESP_DISP:
		case UMSG_PP_CODE_W_RESP_DISP_ALL:
		case UMSG_PP_CODE_DISP_END:
			rc = __encode_pp_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_PP_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_HELLO:
		case UMSG_PP_CODE_RESP_HELLO:
			rc = __encode_pp_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_pp(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_pp";
	struct umessage_pp_hdr *msghdr_p = (struct umessage_pp_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_PP_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_ADD:
			rc = __decode_pp_add(p, len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_ADD:
			rc = __decode_pp_add_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_PP_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_DELETE:
			rc = __decode_pp_delete(p, len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_DELETE:
			rc = __decode_pp_delete_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_PP_CMD_STAT:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_STAT:
			rc = __decode_pp_stat(p, len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_STAT:
			rc = __decode_pp_stat_resp(p, len, msghdr_p);
			break;

		case UMSG_PP_CODE_STAT_ALL:
			rc = __decode_pp_stat(p, len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_STAT_ALL:
			rc = __decode_pp_stat_resp(p, len, msghdr_p);
			break;

		case UMSG_PP_CODE_RESP_END:
			rc = __decode_pp_stat_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
		}
		break;

	case UMSG_PP_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_S_DISP:
		case UMSG_PP_CODE_S_DISP_ALL:
		case UMSG_PP_CODE_W_DISP:
		case UMSG_PP_CODE_W_DISP_ALL:
			rc = __decode_pp_display(p, len, msghdr_p);
			break;

		case UMSG_PP_CODE_S_RESP_DISP:
		case UMSG_PP_CODE_S_RESP_DISP_ALL:
		case UMSG_PP_CODE_W_RESP_DISP:
		case UMSG_PP_CODE_W_RESP_DISP_ALL:
		case UMSG_PP_CODE_DISP_END:
			rc = __decode_pp_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_PP_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_PP_CODE_HELLO:
		case UMSG_PP_CODE_RESP_HELLO:
			rc = __decode_pp_hello(p, len, msghdr_p);
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
