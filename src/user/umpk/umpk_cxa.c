/*
 * umpk_cxa.c
 * 	umpk for write cache
 */

#include <stddef.h>
#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_cxa_if.h"


static int
__encode_cxa_channel_keepalive(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	char *p = out_buf;
	char *log_header = "__encode_cxa_channel_keepalive";
	uint32_t len = 0;

	nid_log_debug ("%s: start", log_header);

	p = encode_tx_hdr(p, &len, msghdr_p);
	*out_len = len;

	return 0;
}

static int
__encode_cxa_channel_drop(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_cxa_drop_channel *msg_p = (struct umessage_cxa_drop_channel *)msghdr_p;
	char *p = out_buf;
	char *log_header = "__encode_cxa_channel_drop";
	uint32_t len = 0, sub_len;

	nid_log_debug ("%s: start", log_header);
	p = encode_tx_hdr(p, &len, msghdr_p);

	*p++ = UMSG_CXA_ITEM_CXA_UUID;
	len++;
	sub_len = msg_p->um_chan_cxa_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_cxa_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CXA_ITEM_CXP_UUID;
	len++;
	sub_len = msg_p->um_chan_cxp_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_cxp_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;
	return 0;
}

static int
__encode_cxa_display(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_cxa_display *msg_p = (struct umessage_cxa_display *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	p = encode_tx_hdr(p, &len, msghdr_p);

	*p++ = UMSG_CXA_ITEM_CXA_UUID;
	len++;
	sub_len = msg_p->um_chan_cxa_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_cxa_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;
	return 0;
}

static int
__encode_cxa_display_resp(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_cxa_display_resp *msg_p = (struct umessage_cxa_display_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	p = encode_tx_hdr(p, &len, msghdr_p);

	*p++ = UMSG_CXA_ITEM_CXA_UUID;
	len++;
	sub_len = msg_p->um_chan_cxa_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_cxa_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CXA_ITEM_PEER_UUID;
	len++;
	sub_len = msg_p->um_peer_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_peer_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CXA_ITEM_IP;
	len++;
	sub_len = msg_p->um_ip_len;
	*(uint8_t *)p = (uint8_t)sub_len;
	p += 1;
	len += 1;
	memcpy(p, msg_p->um_ip, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CXA_ITEM_CXT_NAME;
	len++;
	sub_len = msg_p->um_cxt_name_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_cxt_name, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;
	return 0;
}

static int
__encode_cxa_hello(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	char *log_header = "__encode_cxa_hello";
	char *p = out_buf;
	uint32_t len = 0;

	p = encode_tx_hdr(p, &len, msghdr_p);

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;		//setup the length
	msghdr_p->um_len = len;		//also setup the length in the input header
	*out_len = len;
	nid_log_warning("%s:um_len:%d", log_header,(*(uint32_t *)p));

	return 0;
}

int
umpk_encode_cxa(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_cxa";
	struct umessage_tx_hdr *msghdr_p = (struct umessage_tx_hdr *)data;
	int rc;

	assert(out_buf && out_len && data);
	switch (msghdr_p->um_req) {
	case UMSG_CXA_CMD_KEEPALIVE:
		nid_log_debug ("%s:  dxa_code_keepalive", log_header);
		rc = __encode_cxa_channel_keepalive(out_buf, out_len, msghdr_p);
		break;

	case UMSG_CXA_CMD_DROP_CHANNEL:
		nid_log_debug ("%s:  dxa_code_delete_channel", log_header);
		rc = __encode_cxa_channel_drop(out_buf, out_len, msghdr_p);
		break;

	case UMSG_CXA_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_CXA_CODE_S_DISP:
		case UMSG_CXA_CODE_S_DISP_ALL:
		case UMSG_CXA_CODE_W_DISP:
		case UMSG_CXA_CODE_W_DISP_ALL:
			rc = __encode_cxa_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_CXA_CODE_S_RESP_DISP:
		case UMSG_CXA_CODE_S_RESP_DISP_ALL:
		case UMSG_CXA_CODE_W_RESP_DISP:
		case UMSG_CXA_CODE_W_RESP_DISP_ALL:
		case UMSG_CXA_CODE_DISP_END:
			rc = __encode_cxa_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd_code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CXA_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_CXA_CODE_HELLO:
		case UMSG_CXA_CODE_RESP_HELLO:
			rc = __encode_cxa_hello(out_buf, out_len, msghdr_p);
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


static int
__decode_cxa_channel_keepalive(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	char *log_header = "__decode_cxa_channel_keepalive";

	nid_log_debug ("%s: start", log_header);

	p = decode_tx_hdr(p, &len, msghdr_p);

	return 0;
}

static int
__decode_cxa_channel_drop(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	char *log_header = "__decode_cxa_channel_drop";
	struct umessage_cxa_drop_channel *msg_p = (struct umessage_cxa_drop_channel *)msghdr_p;
	int8_t item;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

	nid_log_debug ("%s: start", log_header);
	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CXA_ITEM_CXA_UUID:
			msg_p->um_chan_cxa_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_cxa_uuid, p, msg_p->um_chan_cxa_uuid_len);
			msg_p->um_chan_cxa_uuid[msg_p->um_chan_cxa_uuid_len] = 0;
			p += msg_p->um_chan_cxa_uuid_len;
			len -= msg_p->um_chan_cxa_uuid_len;
			break;

		case UMSG_CXA_ITEM_CXP_UUID:
			msg_p->um_chan_cxp_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_cxp_uuid, p, msg_p->um_chan_cxp_uuid_len);
			msg_p->um_chan_cxp_uuid[msg_p->um_chan_cxp_uuid_len] = 0;
			p += msg_p->um_chan_cxp_uuid_len;
			len -= msg_p->um_chan_cxp_uuid_len;
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
__decode_cxa_display(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	char *log_header = "__decode_cxa_display";
	struct umessage_cxa_display *msg_p = (struct umessage_cxa_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CXA_ITEM_CXA_UUID:
			msg_p->um_chan_cxa_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_cxa_uuid, p, msg_p->um_chan_cxa_uuid_len);
			msg_p->um_chan_cxa_uuid[msg_p->um_chan_cxa_uuid_len] = 0;
			p += msg_p->um_chan_cxa_uuid_len;
			len -= msg_p->um_chan_cxa_uuid_len;
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
__decode_cxa_display_resp(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	char *log_header = "__decode_cxa_display_resp";
	struct umessage_cxa_display_resp *msg_p = (struct umessage_cxa_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CXA_ITEM_CXA_UUID:
			msg_p->um_chan_cxa_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_chan_cxa_uuid, p, msg_p->um_chan_cxa_uuid_len);
			msg_p->um_chan_cxa_uuid[msg_p->um_chan_cxa_uuid_len] = 0;
			p += msg_p->um_chan_cxa_uuid_len;
			len -= msg_p->um_chan_cxa_uuid_len;
			break;

		case UMSG_CXA_ITEM_PEER_UUID:
			msg_p->um_peer_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_peer_uuid, p, msg_p->um_peer_uuid_len);
			msg_p->um_peer_uuid[msg_p->um_peer_uuid_len] = 0;
			p += msg_p->um_peer_uuid_len;
			len -= msg_p->um_peer_uuid_len;
			break;

		case UMSG_CXA_ITEM_IP:
			msg_p->um_ip_len = *(int8_t *)p;
			p += 1;
			len -= 1;
			memcpy(msg_p->um_ip, p, msg_p->um_ip_len);
			msg_p->um_ip[msg_p->um_ip_len] = 0;
			p += msg_p->um_ip_len;
			len -= msg_p->um_ip_len;
			break;

		case UMSG_CXA_ITEM_CXT_NAME:
			msg_p->um_cxt_name_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(msg_p->um_cxt_name, p, msg_p->um_cxt_name_len);
			msg_p->um_cxt_name[msg_p->um_cxt_name_len] = 0;
			p += msg_p->um_cxt_name_len;
			len -= msg_p->um_cxt_name_len;
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
__decode_cxa_hello(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	char *log_header = "__decode_cxa_hello";
	int8_t item;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

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
umpk_decode_cxa(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_cxa";
	struct umessage_tx_hdr *msghdr_p = (struct umessage_tx_hdr *)data;
	int rc = 0;

	assert(p && len && data);
	switch (msghdr_p->um_req) {
	case UMSG_CXA_CMD_KEEPALIVE:
		nid_log_debug("%s: keepalive message decode", log_header);
		rc = __decode_cxa_channel_keepalive(p, len, msghdr_p);
		break;

	case UMSG_CXA_CMD_DROP_CHANNEL:
		nid_log_debug("%s: delete channel message decode", log_header);
		rc = __decode_cxa_channel_drop(p, len, msghdr_p);
		break;

	case UMSG_CXA_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_CXA_CODE_S_DISP:
		case UMSG_CXA_CODE_S_DISP_ALL:
		case UMSG_CXA_CODE_W_DISP:
		case UMSG_CXA_CODE_W_DISP_ALL:
			rc = __decode_cxa_display(p, len, msghdr_p);
			break;

		case UMSG_CXA_CODE_S_RESP_DISP:
		case UMSG_CXA_CODE_S_RESP_DISP_ALL:
		case UMSG_CXA_CODE_W_RESP_DISP:
		case UMSG_CXA_CODE_W_RESP_DISP_ALL:
		case UMSG_CXA_CODE_DISP_END:
			rc = __decode_cxa_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CXA_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_CXA_CODE_HELLO:
		case UMSG_CXA_CODE_RESP_HELLO:
			rc = __decode_cxa_hello(p, len, msghdr_p);
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
