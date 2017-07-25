/*
 * umpk_bwc.c
 * 	umpk for None Memory Write Cache
 */

#include <string.h>
#include <sys/types.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_bwc_if.h"

static int
__encode_bwc_dropcache(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_dropcache";
	struct umessage_bwc_dropcache *msg_p = (struct umessage_bwc_dropcache *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
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
__decode_bwc_dropcache(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_dropcache";
	struct umessage_bwc_dropcache *msg_p = (struct umessage_bwc_dropcache *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
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
__encode_bwc_information_list_stat(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_information_list_stat";
	struct umessage_bwc_information_list_stat *msg_p = (struct umessage_bwc_information_list_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
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
__decode_bwc_information_list_stat(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_information_list_stat";
	struct umessage_bwc_information_list_stat *msg_p = (struct umessage_bwc_information_list_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
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
__encode_bwc_water_mark(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_water_mark";
	struct umessage_bwc_water_mark *msg_p = (struct umessage_bwc_water_mark *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*(uint16_t *)p = msg_p->um_high_water_mark;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_low_water_mark;
	p += 2;
	len += 2;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_water_mark(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_water_mark";
	struct umessage_bwc_water_mark *msg_p = (struct umessage_bwc_water_mark *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;

			msg_p->um_high_water_mark = *(uint16_t *)p;
			p += 2;
			len -= 2;

			msg_p->um_low_water_mark = *(uint16_t *)p;
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
__encode_bwc_add(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_add";
	struct umessage_bwc_add *msg_p = (struct umessage_bwc_add *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_bufdev_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bufdev, sub_len);
	p += sub_len;
	len += sub_len;

	*(uint32_t *)p = msg_p->um_bufdev_sz;
	p += 4;
	len += 4;

	*(uint8_t *)p = msg_p->um_rw_sync;
	p ++;
	len ++;

	*(uint8_t *)p = msg_p->um_two_step_read;
	p ++;
	len ++;

	*(uint8_t *)p = msg_p->um_do_fp;
	p ++;
	len ++;

	*(uint8_t *)p = msg_p->um_bfp_type;
	p ++;
	len ++;

	*(double *)p = msg_p->um_coalesce_ratio;
	p += sizeof(double);
	len += sizeof(double);

	*(double *)p = msg_p->um_load_ratio_max;
	p += sizeof(double);
	len += sizeof(double);

	*(double *)p = msg_p->um_load_ratio_min;
	p += sizeof(double);
	len += sizeof(double);

	*(double *)p = msg_p->um_load_ctrl_level;
	p += sizeof(double);
	len += sizeof(double);

	*(double *)p = msg_p->um_flush_delay_ctl;
	p += sizeof(double);
	len += sizeof(double);

	*(double *)p = msg_p->um_throttle_ratio;
	p += sizeof(double);
	len += sizeof(double);

	*(uint8_t *)p = msg_p->um_ssd_mode;
	p ++;
	len ++;

	*(uint8_t *)p = msg_p->um_max_flush_size;
	p ++;
	len ++;

	*(uint16_t *)p = msg_p->um_write_delay_first_level;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_write_delay_second_level;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_high_water_mark;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_low_water_mark;
	p += 2;
	len += 2;

	/* sac tp name */
	*p++ = UMSG_BWC_ITEM_TP;
	len++;
	sub_len = msg_p->um_tp_name_len;
	*(uint8_t *)p = msg_p->um_tp_name_len;
	len ++;
	p ++;
	memcpy(p, msg_p->um_tp_name, sub_len);
	len += sub_len;
	p += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_add(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_add";
	struct umessage_bwc_add *msg_p = (struct umessage_bwc_add *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;

			msg_p->um_bufdev_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bufdev, p, msg_p->um_bufdev_len);
			msg_p->um_bufdev[msg_p->um_bufdev_len] = 0;
			p += msg_p->um_bufdev_len;
			len -= msg_p->um_bufdev_len;
			msg_p->um_bufdev_sz = *(int32_t *)p;
			p += 4;
			len -= 4;

			msg_p->um_rw_sync = *(uint8_t *)p;
			p ++;
			len --;

			msg_p->um_two_step_read = *(uint8_t *)p;
			p ++;
			len --;

			msg_p->um_do_fp = *(uint8_t *)p;
			p ++;
			len --;

			msg_p->um_bfp_type = *(uint8_t *)p;
			p ++;
			len --;

			msg_p->um_coalesce_ratio = *(double *)p;
			p += sizeof(double);
			len -= sizeof(double);

			msg_p->um_load_ratio_max = *(double *)p;
			p += sizeof(double);
			len -= sizeof(double);

			msg_p->um_load_ratio_min = *(double *)p;
			p += sizeof(double);
			len -= sizeof(double);

			msg_p->um_load_ctrl_level = *(double *)p;
			p += sizeof(double);
			len -= sizeof(double);

			msg_p->um_flush_delay_ctl = *(double *)p;
			p += sizeof(double);
			len -= sizeof(double);

			msg_p->um_throttle_ratio = *(double *)p;
			p += sizeof(double);
			len -= sizeof(double);

			msg_p->um_ssd_mode = *(uint8_t *)p;
			p ++;
			len --;

			msg_p->um_max_flush_size = *(uint8_t *)p;
			p ++;
			len --;

			msg_p->um_write_delay_first_level = *(uint16_t *)p;
			p += 2;
			len -= 2;

			msg_p->um_write_delay_second_level = *(uint16_t *)p;
			p += 2;
			len -= 2;

			msg_p->um_high_water_mark = *(uint16_t *)p;
			p += 2;
			len -= 2;

			msg_p->um_low_water_mark = *(uint16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_TP:
			msg_p->um_tp_name_len = *(int8_t *)p;
			p ++;
			len --;
			memcpy(msg_p->um_tp_name, p, msg_p->um_tp_name_len);
			msg_p->um_tp_name[msg_p->um_tp_name_len] = '\0';
			p += msg_p->um_tp_name_len;
			len -= msg_p->um_tp_name_len;
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
__encode_bwc_remove(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_remove";
	struct umessage_bwc_remove *msg_p = (struct umessage_bwc_remove *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_remove(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_remove";
	struct umessage_bwc_remove *msg_p = (struct umessage_bwc_remove *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
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
__encode_bwc_water_mark_resp(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_water_mark_resp";
	struct umessage_bwc_water_mark_resp *msg_p = (struct umessage_bwc_water_mark_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_WATER_MARK_RSLT;
	len++;
	*(uint8_t *)p = msg_p->um_resp_code;
	p ++;
	len ++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_water_mark_resp(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_water_mark_resp";
	struct umessage_bwc_water_mark_resp *msg_p = (struct umessage_bwc_water_mark_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_BWC_ITEM_CACHE (%d)", log_header, item);
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_WATER_MARK_RSLT:
			nid_log_error("%s: got item UMSG_BWC_ITEM_WATER_MARK_RSLT (%d)", log_header, item);
			msg_p->um_resp_code = *(int8_t *)p;
			p ++;
			len --;
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
__encode_bwc_add_resp(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_add_resp";
	struct umessage_bwc_add_resp *msg_p = (struct umessage_bwc_add_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_ADD_RSLT;
	len++;
	*(uint8_t *)p = msg_p->um_resp_code;
	p ++;
	len ++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_add_resp(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_add_resp";
	struct umessage_bwc_add_resp *msg_p = (struct umessage_bwc_add_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_BWC_ITEM_CACHE (%d)", log_header, item);
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_ADD_RSLT:
			nid_log_error("%s: got item UMSG_BWC_ITEM_ADD_RSLT (%d)", log_header, item);
			msg_p->um_resp_code = *(int8_t *)p;
			p ++;
			len --;
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
__encode_bwc_remove_resp(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_remove_resp";
	struct umessage_bwc_remove_resp *msg_p = (struct umessage_bwc_remove_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_REMOVE_RSLT;
	len++;
	*(uint8_t *)p = msg_p->um_resp_code;
	p ++;
	len ++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_remove_resp(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_remove_resp";
	struct umessage_bwc_remove_resp *msg_p = (struct umessage_bwc_remove_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_BWC_ITEM_CACHE (%d)", log_header, item);
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_REMOVE_RSLT:
			nid_log_error("%s: got item UMSG_BWC_ITEM_ADD_RSLT (%d)", log_header, item);
			msg_p->um_resp_code = *(int8_t *)p;
			p ++;
			len --;
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
__encode_bwc_information(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_information";
	struct umessage_bwc_information *msg_p = (struct umessage_bwc_information *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
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
__encode_bwc_throughput(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_throughput";
	struct umessage_bwc_throughput *msg_p = (struct umessage_bwc_throughput *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_bwc_information_resp_stat(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_information_resp_stat";
	struct umessage_bwc_information_resp_stat *msg_p = (struct umessage_bwc_information_resp_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_BLOCK_OCCUPIED;
	len++;
	*(uint16_t *)p = msg_p->um_block_occupied;
	p += 2;
	len += 2;

	*p++ = UMSG_BWC_ITEM_NBLOCKS;
	len++;
	*(uint16_t *)p = msg_p->um_flush_nblocks;
	p += 2;
	len += 2;

	*p++ = UMSG_BWC_ITEM_BLOCK_WRITE;
	len++;
	*(uint16_t *)p = msg_p->um_cur_write_block;
	p += 2;
	len += 2;

	*p++ = UMSG_BWC_ITEM_BLOCK_FLUSH;
	len++;
	*(uint16_t *)p = msg_p->um_cur_flush_block;
	p += 2;
	len += 2;

	*p++ = UMSG_BWC_ITEM_SEQ_ASSIGNED;
	len++;
	*(uint64_t *)p = msg_p->um_seq_assigned;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_SEQ_FLUSHED;
	len++;
	*(uint64_t *)p = msg_p->um_seq_flushed;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_RESP_CONTER;
	len++;
	*(uint64_t *)p = msg_p->um_resp_counter;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_RECFLUSHED;
	len++;
	*(uint64_t *)p = msg_p->um_rec_seq_flushed;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_STATE;
	len++;
	*(uint8_t *)p = msg_p->um_state;
	p += 1;
	len += 1;

	*p++ = UMSG_BWC_ITEM_BUF_AVAIL;
	len++;
	*(uint32_t *)p = msg_p->um_buf_avail;
	p += 4;
	len += 4;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_bwc_information_throughput_stat(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_information_throughput_stat";
	struct umessage_bwc_information_resp_throughput_stat *msg_p = (struct umessage_bwc_information_resp_throughput_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;


	*p++ = UMSG_BWC_ITEM_DATA_FLUSHED;
	len++;
	*(uint64_t *)p = msg_p->um_seq_data_flushed;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_DATA_PKG_FLUSHED;
	len++;
	*(uint64_t *)p = msg_p->um_seq_data_pkg_flushed;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_NONDATA_PKG_FLUSHED;
	len++;
	*(uint64_t *)p = msg_p->um_seq_nondata_pkg_flushed;
	p += 8;
	len += 8;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_information_throughput_stat(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_information_resp_stat";
	struct umessage_bwc_information_resp_throughput_stat *msg_p = (struct umessage_bwc_information_resp_throughput_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_BWC_ITEM_CACHE (%d)", log_header, item);
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_DATA_FLUSHED:
			nid_log_error("%s: got item UMSG_BWC_ITEM_DATA_FLUSHED (%d)", log_header, item);
			msg_p->um_seq_data_flushed= *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_DATA_PKG_FLUSHED:
			nid_log_error("%s: got item UMSG_BWC_ITEM_DATA_PKG_FLUSHED (%d)", log_header, item);
			msg_p->um_seq_data_pkg_flushed = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_NONDATA_PKG_FLUSHED:
			nid_log_error("%s: got item UMSG_BWC_ITEM_NONDATA_PKG_FLUSHED (%d)", log_header, item);
			msg_p->um_seq_nondata_pkg_flushed = *(int64_t *)p;
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
__encode_bwc_information_rw_stat(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_information_rw_stat";
	struct umessage_bwc_information_resp_rw_stat *msg_p = (struct umessage_bwc_information_resp_rw_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
	len++;
	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_OVERWRITTEN_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_overwritten_counter;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_OVERWRITTEN_BACK_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_overwritten_back_counter;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_COALESCE_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_coalesce_flush_num;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_COALESCE_BACK_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_coalesce_flush_back_num;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_FLUSH_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_flush_num;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_FLUSH_BACK_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_flush_back_num;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_FLUSH_PAGE_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_flush_page_num;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_NOT_READY_NUM;
	len++;
	*(uint64_t *)p = msg_p->um_seq_not_ready_num;
	p += 8;
	len += 8;

	*p++ = UMSG_BWC_ITEM_RES_NUM;
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
__decode_bwc_information_rw_stat(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_information_resp_stat";
	struct umessage_bwc_information_resp_rw_stat *msg_p = (struct umessage_bwc_information_resp_rw_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			nid_log_error("%s: got item UMSG_BWC_ITEM_CACHE (%d)", log_header, item);
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			nid_log_error("%s: got item UMSG_BWC_ITEM_CHANNEL (%d)", log_header, item);
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;


		case UMSG_BWC_ITEM_OVERWRITTEN_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_OVERWRITTEN_NUM (%d)", log_header, item);
			msg_p->um_seq_overwritten_counter= *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_OVERWRITTEN_BACK_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_OVERWRITTEN_BACK_NUM (%d)", log_header, item);
			msg_p->um_seq_overwritten_back_counter= *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_COALESCE_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_COALESCE_NUM (%d)", log_header, item);
			msg_p->um_seq_coalesce_flush_num= *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_COALESCE_BACK_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_COALESCE_BACK_NUM (%d)", log_header, item);
			msg_p->um_seq_coalesce_flush_back_num= *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_FLUSH_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_FLUSH_NUM (%d)", log_header, item);
			msg_p->um_seq_flush_num= *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_FLUSH_BACK_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_FLUSH_BACK_NUM (%d)", log_header, item);
			msg_p->um_seq_flush_back_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_FLUSH_PAGE_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_FLUSH_PAGE_NUM (%d)", log_header, item);
			msg_p->um_seq_flush_page_num= *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_NOT_READY_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_NOT_READY_NUM (%d)", log_header, item);
			msg_p->um_seq_not_ready_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_RES_NUM:
			nid_log_error("%s: got item UMSG_BWC_ITEM_RES_NUM (%d)", log_header, item);
			msg_p->res = *(int64_t *)p;
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
__decode_bwc_information(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_information";
	struct umessage_bwc_information *msg_p = (struct umessage_bwc_information *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
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
__decode_bwc_throughput(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_throughput";
	struct umessage_bwc_throughput *msg_p = (struct umessage_bwc_throughput *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
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
__decode_bwc_information_resp_stat(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_information_resp_stat";
	struct umessage_bwc_information_resp_stat *msg_p = (struct umessage_bwc_information_resp_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_CACHE (%d)", log_header, item);
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_BLOCK_OCCUPIED:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_BLOCK_OCCUPIED (%d)", log_header, item);
			msg_p->um_block_occupied = *(int16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_NBLOCKS:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_NBLOCKS (%d)", log_header, item);
			msg_p->um_flush_nblocks = *(int16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_BLOCK_WRITE:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_BLOCK_WRITE (%d)", log_header, item);
			msg_p->um_cur_write_block = *(int16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_BLOCK_FLUSH:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_BLOCK_FLUSH (%d)", log_header, item);
			msg_p->um_cur_flush_block = *(int16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_SEQ_ASSIGNED:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_SEQ_ASSIGNED (%d)", log_header, item);
			msg_p->um_seq_assigned = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_SEQ_FLUSHED:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_SEQ_FLUSHED (%d)", log_header, item);
			msg_p->um_seq_flushed = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_RESP_CONTER:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_RESP_CONTER (%d)", log_header, item);
			msg_p->um_resp_counter = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_RECFLUSHED:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_RECFLUSHED (%d)", log_header, item);
			msg_p->um_rec_seq_flushed = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_BWC_ITEM_STATE:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_STATE (%d)", log_header, item);
			msg_p->um_state = *(int8_t *)p;
			p += 1;
			len -= 1;
			break;

		case UMSG_BWC_ITEM_BUF_AVAIL:
			nid_log_debug("%s: got item UMSG_BWC_ITEM_BUF_AVAIL (%d)", log_header, item);
			msg_p->um_buf_avail = *(int32_t *)p;
			p += 4;
			len -= 4;
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
__encode_bwc_information_delay_stat(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_information_resp_delay_stat";
	struct umessage_bwc_information_resp_delay_stat *msg_p = (struct umessage_bwc_information_resp_delay_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_DELAY_FIRST_LEVEL;
	len++;
	*(uint16_t *)p = msg_p->um_write_delay_first_level;
	p += 2;
	len += 2;

	*p++ = UMSG_BWC_ITEM_DELAY_SECOND_LEVEL;
	len++;
	*(uint16_t *)p = msg_p->um_write_delay_second_level;
	p += 2;
	len += 2;

	*p++ = UMSG_BWC_ITEM_DELAY_FIRST_LEVEL_MAX;
	len++;
	*(uint32_t *)p = msg_p->um_write_delay_first_level_max;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_DELAY_SECOND_LEVEL_MAX;
	len++;
	*(uint32_t *)p = msg_p->um_write_delay_second_level_max;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_DELAY_FIRST_INTERVAL;
	len++;
	*(uint32_t *)p = msg_p->um_write_delay_first_level_increase_interval;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_DELAY_SECOND_INTERVAL;
	*(uint32_t *)p = msg_p->um_write_delay_second_level_increase_interval;
	len++;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_DELAY_SECOND_START;
	len++;
	*(uint32_t *)p = msg_p->um_write_delay_second_level_start;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_DELAY_TIME;
	len++;
	*(uint32_t *)p = msg_p->um_write_delay_time;
	p += 4;
	len += 4;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_information_delay_stat(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_information_resp_delay_stat";
	struct umessage_bwc_information_resp_delay_stat *msg_p = (struct umessage_bwc_information_resp_delay_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_DELAY_FIRST_LEVEL:
			msg_p->um_write_delay_first_level = *(uint16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_DELAY_SECOND_LEVEL:
			msg_p->um_write_delay_second_level = *(uint16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_DELAY_FIRST_LEVEL_MAX:
			msg_p->um_write_delay_first_level_max = *(uint32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_DELAY_SECOND_LEVEL_MAX:
			msg_p->um_write_delay_second_level_max = *(uint32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_DELAY_FIRST_INTERVAL:
			msg_p->um_write_delay_first_level_increase_interval = *(uint32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_DELAY_SECOND_INTERVAL:
			msg_p->um_write_delay_second_level_increase_interval = *(uint32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_DELAY_SECOND_START:
			msg_p->um_write_delay_second_level_start = *(uint32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_DELAY_TIME:
			msg_p->um_write_delay_time = *(uint32_t *)p;
			p += 4;
			len -= 4;
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
__encode_bwc_fflush(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_fflush";
	struct umessage_bwc_fflush *msg_p = (struct umessage_bwc_fflush *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
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
__decode_bwc_fflush(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_fflush";
	struct umessage_bwc_fflush *msg_p = (struct umessage_bwc_fflush *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
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
__encode_bwc_fflush_resp_get(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_fflush_resp_get";
	struct umessage_bwc_fflush_resp_get *msg_p = (struct umessage_bwc_fflush_resp_get *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
	len++;
	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_FF_STATE;
	len++;
	*(uint8_t *)p = msg_p->um_ff_state;
	p ++;
	len ++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_fflush_resp_get(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_fflush_resp_get";
	struct umessage_bwc_fflush_resp_get *msg_p = (struct umessage_bwc_fflush_resp_get *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_BWC_ITEM_FF_STATE:
			msg_p->um_ff_state = *(int8_t *)p;
			p ++;
			len --;
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
__encode_bwc_recover(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_recover";
	struct umessage_bwc_recover *msg_p = (struct umessage_bwc_recover *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_recover(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_recover";
	struct umessage_bwc_recover *msg_p = (struct umessage_bwc_recover *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
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
__encode_bwc_recover_resp_get(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_recover_resp_get";
	struct umessage_bwc_recover_resp_get *msg_p = (struct umessage_bwc_recover_resp_get *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_RCV_STATE;
	len++;
	*(uint8_t *)p = msg_p->um_rcv_state;
	p ++;
	len ++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_recover_resp_get(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_recover_resp_get";
	struct umessage_bwc_recover_resp_get *msg_p = (struct umessage_bwc_recover_resp_get *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_RCV_STATE:
			msg_p->um_rcv_state = *(int8_t *)p;
			p ++;
			len --;
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
__encode_bwc_snapshot_admin(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_snapshot_admin";
	struct umessage_bwc_snapshot *msg_p = (struct umessage_bwc_snapshot *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: bwc_uuid:%s chan_uuid:%s", log_header, msg_p->um_bwc_uuid, msg_p->um_chan_uuid);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_SNAPSHOT;
	len++;

	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

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
__decode_bwc_snapshot_admin(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_snapshot_admin";
	struct umessage_bwc_snapshot *msg_p = (struct umessage_bwc_snapshot *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_SNAPSHOT:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;

			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;

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
__encode_bwc_snapshot_admin_resp(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_snapshot_admin_resp";
	struct umessage_bwc_snapshot_resp *msg_p = (struct umessage_bwc_snapshot_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: bwc_uuid:%s chan_uuid:%s", log_header, msg_p->um_bwc_uuid, msg_p->um_chan_uuid);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_SNAPSHOT;
	len++;

	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*(int8_t *)p = (int8_t)msg_p->um_resp_code;
	p ++;
	len ++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_snapshot_admin_resp(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_snapshot_admin_resp";
	struct umessage_bwc_snapshot_resp *msg_p = (struct umessage_bwc_snapshot_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_SNAPSHOT:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;

			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;

			msg_p->um_resp_code = *(int8_t *)p;
			p ++;
			len --;
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
__encode_bwc_ioinfo(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_ioinfo";
	struct umessage_bwc_ioinfo *msg_p = (struct umessage_bwc_ioinfo *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
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
__decode_bwc_ioinfo(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_ioinfo";
	struct umessage_bwc_ioinfo *msg_p = (struct umessage_bwc_ioinfo *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
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
__encode_bwc_ioinfo_resp(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_ioinfo_resp";
	struct umessage_bwc_ioinfo_resp *msg_p = (struct umessage_bwc_ioinfo_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
	len++;
	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_IOINFO;
	len++;

	*(uint8_t *)p = msg_p->um_is_running;
	p++;
	len++;

	*(uint64_t *)p = msg_p->um_req_num;
	p += 8;
	len += 8;

	*(uint64_t *)p = msg_p->um_req_len;
	p += 8;
	len += 8;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_ioinfo_resp(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_ioinfo_resp";
	struct umessage_bwc_ioinfo_resp *msg_p = (struct umessage_bwc_ioinfo_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_BWC_ITEM_IOINFO:
			msg_p->um_is_running = *(int8_t *)p;
			p++;
			len--;

			msg_p->um_req_num = *(int64_t *)p;
			p += 8;
			len -= 8;

			msg_p->um_req_len = *(int64_t *)p;
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
__encode_bwc_delay(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_delay";
	struct umessage_bwc_delay *msg_p = (struct umessage_bwc_delay *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_DELAY_FIRST_LEVEL;
	len++;
	*(uint16_t *)p = msg_p->um_write_delay_first_level;
	p += 2;
	len += 2;

	*p++ = UMSG_BWC_ITEM_DELAY_SECOND_LEVEL;
	len++;
	*(uint16_t *)p = msg_p->um_write_delay_second_level;
	p += 2;
	len += 2;

	*p++ = UMSG_BWC_ITEM_DELAY_FIRST_LEVEL_MAX;
	len++;
	*(uint32_t *)p = msg_p->um_write_delay_first_level_max;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_DELAY_SECOND_LEVEL_MAX;
	len++;
	*(uint32_t *)p = msg_p->um_write_delay_second_level_max;
	p += 4;
	len += 4;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_delay(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_delay";
	struct umessage_bwc_delay *msg_p = (struct umessage_bwc_delay *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_DELAY_FIRST_LEVEL:
			msg_p->um_write_delay_first_level = *(uint16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_DELAY_SECOND_LEVEL:
			msg_p->um_write_delay_second_level = *(uint16_t *)p;
			p += 2;
			len -= 2;
			break;

		case UMSG_BWC_ITEM_DELAY_FIRST_LEVEL_MAX:
			msg_p->um_write_delay_first_level_max = *(uint32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_DELAY_SECOND_LEVEL_MAX:
			msg_p->um_write_delay_second_level_max = *(uint32_t *)p;
			p += 4;
			len -= 4;
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
__encode_bwc_delay_resp(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_delay_resp";
	struct umessage_bwc_delay_resp *msg_p = (struct umessage_bwc_delay_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: bwc_uuid:%s", log_header, msg_p->um_bwc_uuid);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;

	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_RESP_CODE;
	len++;

	*(int8_t *)p = (int8_t)msg_p->um_resp_code;
	p++;
	len++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_delay_resp(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_delay_resp";
	struct umessage_bwc_delay_resp *msg_p = (struct umessage_bwc_delay_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_RESP_CODE:
			msg_p->um_resp_code = *(int8_t *)p;
			p++;
			len--;
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
__encode_bwc_flush_empty(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_flush_empty";
	struct umessage_bwc_flush_empty *msg_p = (struct umessage_bwc_flush_empty *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_flush_empty(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_flush_empty";
	struct umessage_bwc_flush_empty *msg_p = (struct umessage_bwc_flush_empty *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
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
__encode_bwc_flush_empty_resp(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_flush_empty_resp";
	struct umessage_bwc_flush_empty_resp *msg_p = (struct umessage_bwc_flush_empty_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: bwc_uuid:%s", log_header, msg_p->um_bwc_uuid);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;

	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_RESP_CODE;
	len++;

	*(int8_t *)p = (int8_t)msg_p->um_resp_code;
	p++;
	len++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_flush_empty_resp(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_flush_empty_resp";
	struct umessage_bwc_flush_empty_resp *msg_p = (struct umessage_bwc_flush_empty_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_RESP_CODE:
			msg_p->um_resp_code = *(int8_t *)p;
			p++;
			len--;
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
__encode_bwc_display(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_display";
	struct umessage_bwc_display *msg_p = (struct umessage_bwc_display *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_display(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_display";
	struct umessage_bwc_display *msg_p = (struct umessage_bwc_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
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
__encode_bwc_display_resp(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_display_resp";
	struct umessage_bwc_display_resp *msg_p = (struct umessage_bwc_display_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_bwc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_BUFDEV;
	len++;
	sub_len = msg_p->um_bufdev_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bufdev, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_TP;
	len++;
	sub_len = msg_p->um_tp_name_len;
	*(uint8_t *)p = (uint8_t)sub_len;
	p += 1;
	len += 1;
	memcpy(p, msg_p->um_tp_name, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_SETUP;
	len++;

	*(uint32_t *)p = msg_p->um_bufdev_sz;
	p += 4;
	len += 4;

	*(uint8_t *)p = msg_p->um_rw_sync;
	p += 1;
	len += 1;

	*(uint8_t *)p = msg_p->um_two_step_read;
	p += 1;
	len += 1;

	*(uint8_t *)p = msg_p->um_do_fp;
	p += 1;
	len += 1;

	*(uint8_t *)p = msg_p->um_bfp_type;
	p += 1;
	len += 1;

	*(double *)p = msg_p->um_coalesce_ratio;
	p += 8;
	len += 8;

	*(double *)p = msg_p->um_load_ratio_max;
	p += 8;
	len += 8;

	*(double *)p = msg_p->um_load_ratio_min;
	p += 8;
	len += 8;

	*(double *)p = msg_p->um_load_ctrl_level;
	p += 8;
	len += 8;

	*(double *)p = msg_p->um_flush_delay_ctl;
	p += 8;
	len += 8;

	*(double *)p = msg_p->um_throttle_ratio;
	p += 8;
	len += 8;

	*(uint8_t *)p = msg_p->um_ssd_mode;
	p += 1;
	len += 1;

	*(uint8_t *)p = msg_p->um_max_flush_size;
	p += 1;
	len += 1;

	*(uint16_t *)p = msg_p->um_write_delay_first_level;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_write_delay_second_level;
	p += 2;
	len += 2;

	*(uint32_t *)p = msg_p->um_write_delay_first_level_max;
	p += 4;
	len += 4;

	*(uint32_t *)p = msg_p->um_write_delay_second_level_max;
	p += 4;
	len += 4;

	*(uint16_t *)p = msg_p->um_high_water_mark;
	p += 2;
	len += 2;

	*(uint16_t *)p = msg_p->um_low_water_mark;
	p += 2;
	len += 2;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_display_resp(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_display_resp";
	struct umessage_bwc_display_resp *msg_p = (struct umessage_bwc_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_BUFDEV:
			msg_p->um_bufdev_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bufdev, p, msg_p->um_bufdev_len);
			msg_p->um_bufdev[msg_p->um_bufdev_len] = 0;
			p += msg_p->um_bufdev_len;
			len -= msg_p->um_bufdev_len;
			break;

		case UMSG_BWC_ITEM_TP:
			msg_p->um_tp_name_len = *(int8_t *)p;
			p += 1;
			len -= 1;
			memcpy(msg_p->um_tp_name, p, msg_p->um_tp_name_len);
			msg_p->um_tp_name[msg_p->um_tp_name_len] = 0;
			p += msg_p->um_tp_name_len;
			len -= msg_p->um_tp_name_len;
			break;

		case UMSG_BWC_ITEM_SETUP:
			msg_p->um_bufdev_sz = *(int32_t *)p;
			p += 4;
			len -= 4;

			msg_p->um_rw_sync = *(int8_t *)p;
			p += 1;
			len -= 1;

			msg_p->um_two_step_read = *(int8_t *)p;
			p += 1;
			len -= 1;

			msg_p->um_do_fp = *(int8_t *)p;
			p += 1;
			len -= 1;

			msg_p->um_bfp_type = *(int8_t *)p;
			p += 1;
			len -= 1;

			msg_p->um_coalesce_ratio = *(double *)p;
			p += 8;
			len -= 8;

			msg_p->um_load_ratio_max = *(double *)p;
			p += 8;
			len -= 8;

			msg_p->um_load_ratio_min = *(double *)p;
			p += 8;
			len -= 8;

			msg_p->um_load_ctrl_level = *(double *)p;
			p += 8;
			len -= 8;

			msg_p->um_flush_delay_ctl = *(double *)p;
			p += 8;
			len -= 8;

			msg_p->um_throttle_ratio = *(double *)p;
			p += 8;
			len -= 8;

			msg_p->um_ssd_mode = *(int8_t *)p;
			p += 1;
			len -= 1;

			msg_p->um_max_flush_size = *(int8_t *)p;
			p += 1;
			len -= 1;

			msg_p->um_write_delay_first_level = *(uint16_t *)p;
			p += 2;
			len -= 2;

			msg_p->um_write_delay_second_level = *(uint16_t *)p;
			p += 2;
			len -= 2;

			msg_p->um_write_delay_first_level_max = *(uint32_t *)p;
			p += 4;
			len -= 4;

			msg_p->um_write_delay_second_level_max = *(uint32_t *)p;
			p += 4;
			len -= 4;

			msg_p->um_high_water_mark = *(uint16_t *)p;
			p += 2;
			len -= 2;

			msg_p->um_low_water_mark = *(uint16_t *)p;
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
__encode_bwc_hello(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_hello";
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
__decode_bwc_hello(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_BWC_HEADER_LEN;

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
__encode_bwc_information_list_resp_stat(char *out_buf, uint32_t *out_len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__encode_bwc_information_list_resp_stat";
	struct umessage_bwc_information_list_resp_stat *msg_p = (struct umessage_bwc_information_list_resp_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_BWC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_bwc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_CHANNEL;
	len++;
	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_BWC_ITEM_TO_WRITE_COUNTER;
	len++;
	*(uint32_t *)p = msg_p->um_to_write_list_counter;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_TO_READ_COUNTER;
	len++;
	*(uint32_t *)p = msg_p->um_to_read_list_counter;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_WRITE_COUNTER;
	len++;
	*(uint32_t *)p = msg_p->um_write_list_counter;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_WRITE_VEC_COUNTER;
	len++;
	*(uint32_t *)p = msg_p->um_write_vec_list_counter;
	p += 4;
	len += 4;

	*p++ = UMSG_BWC_ITEM_WRITE_BUSY;
	len++;
	*(uint8_t *)p = msg_p->um_write_busy;
	p ++;
	len ++;

	*p++ = UMSG_BWC_ITEM_READ_BUSY;
	len++;
	*(uint8_t *)p = msg_p->um_read_busy;
	p ++;
	len ++;

	*p++ = UMSG_BWC_ITEM_FOUND_IT;
	len++;
	*(uint8_t *)p = msg_p->um_found_it;
	p ++;
	len ++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_bwc_information_list_resp_stat(char *p, int len, struct umessage_bwc_hdr *msghdr_p)
{
	char *log_header = "__decode_bwc_information_list_resp_stat";
	struct umessage_bwc_information_list_resp_stat *msg_p = (struct umessage_bwc_information_list_resp_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_BWC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_BWC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_BWC_ITEM_CACHE:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = 0;
			p += msg_p->um_bwc_uuid_len;
			len -= msg_p->um_bwc_uuid_len;
			break;

		case UMSG_BWC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_BWC_ITEM_TO_WRITE_COUNTER:
			msg_p->um_to_write_list_counter = *(int32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_TO_READ_COUNTER:
			msg_p->um_to_read_list_counter = *(int32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_WRITE_COUNTER:
			msg_p->um_write_list_counter = *(int32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_WRITE_VEC_COUNTER:
			msg_p->um_write_vec_list_counter = *(int32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_BWC_ITEM_WRITE_BUSY:
			msg_p->um_write_busy = *(int8_t *)p;
			p ++;
			len --;
			break;

		case UMSG_BWC_ITEM_READ_BUSY:
			msg_p->um_read_busy = *(int8_t *)p;
			p ++;
			len --;
			break;

		case UMSG_BWC_ITEM_FOUND_IT:
			msg_p->um_found_it = *(int8_t *)p;
			p ++;
			len --;
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
umpk_encode_bwc(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_bwc";
	struct umessage_bwc_hdr *msghdr_p = (struct umessage_bwc_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_BWC_CMD_DROPCACHE:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_START:
		case UMSG_BWC_CODE_START_SYNC:
		case UMSG_BWC_CODE_STOP:
			rc = __encode_bwc_dropcache(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_FLUSHING:
		case UMSG_BWC_CODE_THROUGHPUT_STAT:
		case UMSG_BWC_CODE_RW_STAT:
		case UMSG_BWC_CODE_STAT:
		case UMSG_BWC_CODE_DELAY_STAT:
			rc = __encode_bwc_information(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_LIST_STAT:
			rc = __encode_bwc_information_list_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_LIST_RESP_STAT:
			rc = __encode_bwc_information_list_resp_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_STAT:
			rc = __encode_bwc_information_resp_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_THROUGHPUT_STAT_RSLT:
			rc = __encode_bwc_information_throughput_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RW_STAT_RSLT:
			rc = __encode_bwc_information_rw_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_DELAY_STAT:
			rc = __encode_bwc_information_delay_stat(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;


	case UMSG_BWC_CMD_THROUGHPUT:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_THROUGHPUT_RESET:
			rc = __encode_bwc_throughput(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_ADD:
			rc = __encode_bwc_add(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_ADD:
			rc = __encode_bwc_add_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_UPDATE_WATER_MARK:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_UPDATE_WATER_MARK:
			rc = __encode_bwc_water_mark(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_UPDATE_WATER_MARK:
			rc = __encode_bwc_water_mark_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_REMOVE:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_REMOVE:
			rc = __encode_bwc_remove(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_REMOVE:
			rc = __encode_bwc_remove_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
			break;

	case UMSG_BWC_CMD_FFLUSH:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_FF_START:
		case UMSG_BWC_CODE_FF_STOP:
		case UMSG_BWC_CODE_FF_GET:
			rc = __encode_bwc_fflush(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_FF_GET:
			rc = __encode_bwc_fflush_resp_get(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_RECOVER:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_RCV_START:
		case UMSG_BWC_CODE_RCV_GET:
			rc = __encode_bwc_recover(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_RCV_GET:
			rc = __encode_bwc_recover_resp_get(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_SNAPSHOT:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_1:
		case UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_2:
		case UMSG_BWC_CODE_SNAPSHOT_UNFREEZE:
			rc = __encode_bwc_snapshot_admin(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_SNAPSHOT_FREEZE_RESP:
		case UMSG_BWC_CODE_SNAPSHOT_UNFREEZE_RESP:
			rc = __encode_bwc_snapshot_admin_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_IOINFO:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_IOINFO_START:
		case UMSG_BWC_CODE_IOINFO_START_ALL:
		case UMSG_BWC_CODE_IOINFO_STOP:
		case UMSG_BWC_CODE_IOINFO_STOP_ALL:
		case UMSG_BWC_CODE_IOINFO_CHECK:
		case UMSG_BWC_CODE_IOINFO_CHECK_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_START:
		case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_START_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_START_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_STOP:
		case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_STOP_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_STOP_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_CHECK:
		case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_CHECK_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_CHECK_ALL:
			rc = __encode_bwc_ioinfo(out_buf, out_len, msghdr_p);
			break;
		case UMSG_BWC_CODE_RESP_IOINFO_START:
		case UMSG_BWC_CODE_RESP_IOINFO_START_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_STOP:
		case UMSG_BWC_CODE_RESP_IOINFO_STOP_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_CHECK:
		case UMSG_BWC_CODE_RESP_IOINFO_CHECK_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_START:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_START_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_START_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_STOP_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_CHECK_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_END:
			rc = __encode_bwc_ioinfo_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_UPDATE_DELAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_UPDATE_DELAY:
			rc = __encode_bwc_delay(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_UPDATE_DELAY:
			rc = __encode_bwc_delay_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_FLUSH_EMPTY:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_FLUSH_EMPTY_START:
		case UMSG_BWC_CODE_FLUSH_EMPTY_STOP:
			rc = __encode_bwc_flush_empty(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_FLUSH_EMPTY_START:
		case UMSG_BWC_CODE_RESP_FLUSH_EMPTY_STOP:
			rc = __encode_bwc_flush_empty_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_S_DISP:
		case UMSG_BWC_CODE_S_DISP_ALL:
		case UMSG_BWC_CODE_W_DISP:
		case UMSG_BWC_CODE_W_DISP_ALL:
			rc = __encode_bwc_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_BWC_CODE_S_RESP_DISP:
		case UMSG_BWC_CODE_S_RESP_DISP_ALL:
		case UMSG_BWC_CODE_W_RESP_DISP:
		case UMSG_BWC_CODE_W_RESP_DISP_ALL:
		case UMSG_BWC_CODE_DISP_END:
			rc = __encode_bwc_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_HELLO:
		case UMSG_BWC_CODE_RESP_HELLO:
			rc = __encode_bwc_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_bwc(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_bwc";
	struct umessage_bwc_hdr *msghdr_p = (struct umessage_bwc_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_BWC_CMD_DROPCACHE:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_START:
		case UMSG_BWC_CODE_START_SYNC:
		case UMSG_BWC_CODE_STOP:
			rc = __decode_bwc_dropcache(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_FLUSHING:
		case UMSG_BWC_CODE_THROUGHPUT_STAT:
		case UMSG_BWC_CODE_RW_STAT:
		case UMSG_BWC_CODE_STAT:
		case UMSG_BWC_CODE_DELAY_STAT:
			rc = __decode_bwc_information(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_LIST_STAT:
			rc = __decode_bwc_information_list_stat(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_LIST_RESP_STAT:
			rc = __decode_bwc_information_list_resp_stat(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_THROUGHPUT_STAT_RSLT:
			rc = __decode_bwc_information_throughput_stat(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_STAT:
			rc = __decode_bwc_information_resp_stat(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RW_STAT_RSLT:
			rc = __decode_bwc_information_rw_stat(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_DELAY_STAT:
			rc = __decode_bwc_information_delay_stat(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_THROUGHPUT:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_THROUGHPUT_RESET:
			rc = __decode_bwc_throughput(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_ADD:
			rc = __decode_bwc_add(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_ADD:
			rc = __decode_bwc_add_resp(p, len, msghdr_p);
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_UPDATE_WATER_MARK:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_UPDATE_WATER_MARK:
			rc = __decode_bwc_water_mark(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_UPDATE_WATER_MARK:
			rc = __decode_bwc_water_mark_resp(p, len, msghdr_p);
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_REMOVE:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_REMOVE:
			rc = __decode_bwc_remove(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_REMOVE:
			rc = __decode_bwc_remove_resp(p, len, msghdr_p);
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_FFLUSH:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_FF_START:
		case UMSG_BWC_CODE_FF_STOP:
		case UMSG_BWC_CODE_FF_GET:
			rc = __decode_bwc_fflush(p, len, msghdr_p);
			break;


		case UMSG_BWC_CODE_RESP_FF_GET:
			rc = __decode_bwc_fflush_resp_get(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_RECOVER:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_RCV_START:
		case UMSG_BWC_CODE_RCV_GET:
			rc = __decode_bwc_recover(p, len, msghdr_p);
			break;


		case UMSG_BWC_CODE_RESP_RCV_GET:
			rc = __decode_bwc_recover_resp_get(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_SNAPSHOT:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_1:
		case UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_2:
		case UMSG_BWC_CODE_SNAPSHOT_UNFREEZE:
			rc = __decode_bwc_snapshot_admin(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_SNAPSHOT_FREEZE_RESP:
		case UMSG_BWC_CODE_SNAPSHOT_UNFREEZE_RESP:
			rc = __decode_bwc_snapshot_admin_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_IOINFO:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_IOINFO_START:
		case UMSG_BWC_CODE_IOINFO_START_ALL:
		case UMSG_BWC_CODE_IOINFO_STOP:
		case UMSG_BWC_CODE_IOINFO_STOP_ALL:
		case UMSG_BWC_CODE_IOINFO_CHECK:
		case UMSG_BWC_CODE_IOINFO_CHECK_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_START:
		case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_START_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_START_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_STOP:
		case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_STOP_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_STOP_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_CHECK:
		case UMSG_BWC_CODE_IOINFO_BFE_CHAIN_CHECK_ALL:
		case UMSG_BWC_CODE_IOINFO_BFE_CHECK_ALL:
			rc = __decode_bwc_ioinfo(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_IOINFO_START:
		case UMSG_BWC_CODE_RESP_IOINFO_START_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_STOP:
		case UMSG_BWC_CODE_RESP_IOINFO_STOP_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_CHECK:
		case UMSG_BWC_CODE_RESP_IOINFO_CHECK_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_START:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_START_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_START_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_STOP_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_CHECK_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK_ALL:
		case UMSG_BWC_CODE_RESP_IOINFO_END:
			rc = __decode_bwc_ioinfo_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_UPDATE_DELAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_UPDATE_DELAY:
			rc = __decode_bwc_delay(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_UPDATE_DELAY:
			rc = __decode_bwc_delay_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_FLUSH_EMPTY:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_FLUSH_EMPTY_START:
		case UMSG_BWC_CODE_FLUSH_EMPTY_STOP:
			rc = __decode_bwc_flush_empty(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_RESP_FLUSH_EMPTY_START:
		case UMSG_BWC_CODE_RESP_FLUSH_EMPTY_STOP:
			rc = __decode_bwc_flush_empty_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_S_DISP:
		case UMSG_BWC_CODE_S_DISP_ALL:
		case UMSG_BWC_CODE_W_DISP:
		case UMSG_BWC_CODE_W_DISP_ALL:
			rc = __decode_bwc_display(p, len, msghdr_p);
			break;

		case UMSG_BWC_CODE_S_RESP_DISP:
		case UMSG_BWC_CODE_S_RESP_DISP_ALL:
		case UMSG_BWC_CODE_W_RESP_DISP:
		case UMSG_BWC_CODE_W_RESP_DISP_ALL:
		case UMSG_BWC_CODE_DISP_END:
			rc = __decode_bwc_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_BWC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_BWC_CODE_HELLO:
		case UMSG_BWC_CODE_RESP_HELLO:
			rc = __decode_bwc_hello(p, len, msghdr_p);
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
