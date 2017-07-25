/*
 * umpk_rept.c
 *	umpk for Replication Target
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_rept_if.h"

static int
__encode_rept_hello(char *out_buf, uint32_t *out_len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__encode_rept_hello";
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
__decode_rept_hello(char *p, int len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__decode_rept_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_REPT_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_REPT_HEADER_LEN;

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
__encode_rept_start(char *out_buf, uint32_t *out_len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__encode_rept_start";
	struct umessage_rept_start *msg_p = (struct umessage_rept_start *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	/* item rept */
	*p ++ = UMSG_REPT_ITEM_REPT;
	len ++;
	sub_len = msg_p->um_rt_name_len;
	*(uint16_t *)p = sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rt_name, sub_len);
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
__decode_rept_start(char *p, int len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__decode_rept_start";
	struct umessage_rept_start *msg_p = (struct umessage_rept_start *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_REPT_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_REPT_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_REPT_ITEM_REPT:
			msg_p->um_rt_name_len = *(uint16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_rt_name, p, msg_p->um_rt_name_len);
			msg_p->um_rt_name[msg_p->um_rt_name_len] = 0;
			p += msg_p->um_rt_name_len;
			len -= msg_p->um_rt_name_len;
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
__encode_rept_start_resp(char *out_buf, uint32_t *out_len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__encode_rept_start_resp";
	struct umessage_rept_start_resp *msg_p = (struct umessage_rept_start_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	/* item resp_code */
	*p ++ = UMSG_REPT_ITEM_RESP_CODE;
	len ++;
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
__decode_rept_start_resp(char *p, int len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__decode_rept_start_resp";
	struct umessage_rept_start_resp *msg_p = (struct umessage_rept_start_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_REPT_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_REPT_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_REPT_ITEM_RESP_CODE:
			msg_p->um_resp_code = *(uint8_t *)p;
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
__encode_rept_display(char *out_buf, uint32_t *out_len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__encode_rept_display";
	struct umessage_rept_display *msg_p = (struct umessage_rept_display *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	/* item rept */
	*p ++ = UMSG_REPT_ITEM_REPT;
	len ++;
	sub_len = msg_p->um_rt_name_len;
	*(uint16_t *)p = sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rt_name, sub_len);
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
__decode_rept_display(char *p, int len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__decode_rept_display";
	struct umessage_rept_display *msg_p = (struct umessage_rept_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_REPT_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_REPT_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_REPT_ITEM_REPT:
			msg_p->um_rt_name_len = *(uint16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_rt_name, p, msg_p->um_rt_name_len);
			msg_p->um_rt_name[msg_p->um_rt_name_len] = 0;
			p += msg_p->um_rt_name_len;
			len -= msg_p->um_rt_name_len;
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
__encode_rept_display_resp(char *out_buf, uint32_t *out_len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__encode_rept_display_resp";
	struct umessage_rept_display_resp *msg_p = (struct umessage_rept_display_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	/* item rept */
	*p ++ = UMSG_REPT_ITEM_REPT;
	len ++;
	sub_len = msg_p->um_rt_name_len;
	*(uint16_t *)p = sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rt_name, sub_len);
	p += sub_len;
	len += sub_len;

	/* item dxp */
	*p ++ = UMSG_REPT_ITEM_DXP;
	len ++;
	sub_len = msg_p->um_dxp_name_len;
	*(uint16_t *)p = sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_dxp_name, sub_len);
	p += sub_len;
	len += sub_len;
	
	/* item voluuid */
	*p ++ = UMSG_REPT_ITEM_VOLUUID;
	len ++;
	sub_len = msg_p->um_voluuid_len;
	*(uint16_t *)p = sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_voluuid, sub_len);
	p += sub_len;
	len += sub_len;

	/* item bit */
	*p ++ = UMSG_REPT_ITEM_BITMAP;
	len ++;
	*(size_t *)p = msg_p->um_bitmap_len;
	p += 8;
	len += 8;

	/* item vol */
	*p ++ = UMSG_REPT_ITEM_VOL;
	len ++;
	*(size_t *)p = msg_p->um_vol_len;
	p += 8;
	len += 8;

	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;
	nid_log_warning("%s:um_len:%d", log_header,(*(uint32_t *)p));

	return 0;
}

static int
__decode_rept_display_resp(char *p, int len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__decode_rept_display_resp";
	struct umessage_rept_display_resp *msg_p = (struct umessage_rept_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_REPT_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_REPT_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_REPT_ITEM_REPT:
			msg_p->um_rt_name_len = *(uint16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_rt_name, p, msg_p->um_rt_name_len);
			msg_p->um_rt_name[msg_p->um_rt_name_len] = 0;
			p += msg_p->um_rt_name_len;
			len -= msg_p->um_rt_name_len;
			break;

		case UMSG_REPT_ITEM_DXP:
			msg_p->um_dxp_name_len = *(uint16_t *)p;
			p += 2;
			len += 2;
			memcpy(msg_p->um_dxp_name, p, msg_p->um_dxp_name_len);
			msg_p->um_dxp_name[msg_p->um_dxp_name_len] = 0;
			p += msg_p->um_dxp_name_len;
			len -= msg_p->um_dxp_name_len;
			break;

		case UMSG_REPT_ITEM_VOLUUID:
			msg_p->um_voluuid_len = *(uint16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_voluuid, p, msg_p->um_voluuid_len);
			msg_p->um_voluuid[msg_p->um_voluuid_len] = 0;
			p += msg_p->um_voluuid_len;
			len -= msg_p->um_voluuid_len;
			break;
			
		case UMSG_REPT_ITEM_BITMAP:
			msg_p->um_bitmap_len = *(size_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_REPT_ITEM_VOL:
			msg_p->um_vol_len = *(size_t *)p;
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
__encode_rept_info(char *out_buf, uint32_t *out_len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__encode_rept_info_pro";
	struct umessage_rept_info *msg_p = (struct umessage_rept_info *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	/* item rept */
	*p ++ = UMSG_REPT_ITEM_REPT;
	len ++;
	sub_len = msg_p->um_rt_name_len;
	*(uint16_t *)p = sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rt_name, sub_len);
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
__decode_rept_info(char *p, int len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__decode_rept_info_pro";
	struct umessage_rept_info *msg_p = (struct umessage_rept_info *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_REPT_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_REPT_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_REPT_ITEM_REPT:
			msg_p->um_rt_name_len = *(uint16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_rt_name, p, msg_p->um_rt_name_len);
			p += msg_p->um_rt_name_len;
			len -= msg_p->um_rt_name_len;
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
__encode_rept_info_pro_resp(char *out_buf, uint32_t *out_len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__encode_rept_info_pro_resp";
	struct umessage_rept_info_pro_resp *msg_p = (struct umessage_rept_info_pro_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;		//skip the length filed
	len += 4;

	/* item rept */
	*p ++ = UMSG_REPT_ITEM_PROGRESS;
	len ++;
	*(float *)p = msg_p->um_progress;
	p += 4;
	len += 4;
	
	p = out_buf + 2;		//go back to the length filed
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;
	nid_log_warning("%s:um_len:%d", log_header,(*(uint32_t *)p));

	return 0;
}


static int
__decode_rept_info_pro_resp(char *p, int len, struct umessage_rept_hdr *msghdr_p)
{
	char *log_header = "__decode_rept_info_pro_resp";
	struct umessage_rept_info_pro_resp *msg_p = (struct umessage_rept_info_pro_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_REPT_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_REPT_HEADER_LEN;

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_REPT_ITEM_PROGRESS:
			msg_p->um_progress = *(float *)p;
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

int
umpk_encode_rept(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_rept";
	struct umessage_rept_hdr *msghdr_p = (struct umessage_rept_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_REPT_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_REPT_CODE_HELLO:
		case UMSG_REPT_CODE_RESP_HELLO:
			rc = __encode_rept_hello(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_REPT_CMD_START:
		switch (msghdr_p->um_req_code) {
		case UMSG_REPT_CODE_START:
			rc = __encode_rept_start(out_buf, out_len, msghdr_p);
			break;

		case UMSG_REPT_CODE_RESP_START:
			rc = __encode_rept_start_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_REPT_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_REPT_CODE_S_DISP:
		case UMSG_REPT_CODE_S_DISP_ALL:
		case UMSG_REPT_CODE_W_DISP:
		case UMSG_REPT_CODE_W_DISP_ALL:
			rc = __encode_rept_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_REPT_CODE_S_RESP_DISP:
		case UMSG_REPT_CODE_S_RESP_DISP_ALL:
		case UMSG_REPT_CODE_W_RESP_DISP:
		case UMSG_REPT_CODE_W_RESP_DISP_ALL:
		case UMSG_REPT_CODE_DISP_END:
			rc = __encode_rept_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_REPT_CMD_INFO:
		switch (msghdr_p->um_req_code) {
		case UMSG_REPT_CODE_INFO_PRO:
			rc = __encode_rept_info(out_buf, out_len, msghdr_p);
			break;

		case UMSG_REPT_CODE_RESP_INFO_PRO:
			rc = __encode_rept_info_pro_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req);
	}
	return rc;
}

int
umpk_decode_rept(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_rept";
	struct umessage_rept_hdr *msghdr_p = (struct umessage_rept_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_REPT_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_REPT_CODE_HELLO:
		case UMSG_REPT_CODE_RESP_HELLO:
			rc = __decode_rept_hello(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_REPT_CMD_START:
		switch (msghdr_p->um_req_code) {
		case UMSG_REPT_CODE_START:
			rc = __decode_rept_start(p, len, msghdr_p);
			break;

		case UMSG_REPT_CODE_RESP_START:
			rc = __decode_rept_start_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_REPT_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_REPT_CODE_S_DISP:
		case UMSG_REPT_CODE_S_DISP_ALL:
		case UMSG_REPT_CODE_W_DISP:
		case UMSG_REPT_CODE_W_DISP_ALL:
			rc = __decode_rept_display(p, len, msghdr_p);
			break;

		case UMSG_REPT_CODE_S_RESP_DISP:
		case UMSG_REPT_CODE_S_RESP_DISP_ALL:
		case UMSG_REPT_CODE_W_RESP_DISP:
		case UMSG_REPT_CODE_W_RESP_DISP_ALL:
		case UMSG_REPT_CODE_DISP_END:
			rc = __decode_rept_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_REPT_CMD_INFO:
		switch (msghdr_p->um_req_code) {
		case UMSG_REPT_CODE_INFO_PRO:
			rc = __decode_rept_info(p, len, msghdr_p);
			break;

		case UMSG_REPT_CODE_RESP_INFO_PRO:
			rc = __decode_rept_info_pro_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req);
	}
	return rc;
}
