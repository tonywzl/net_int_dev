/*
 * umpk_dxp.c
 * 	umpk for write cache
 */

#include <stddef.h>
#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_dxp_if.h"

static int
__encode_dxp_create_channel_uuid(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_create_channel *msg_p = (struct umessage_dxp_create_channel *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	p = encode_tx_hdr(p, &len, msghdr_p);

	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;
	return 0;
}


static int
__decode_dxp_create_channel_uuid(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_create_channel *msg_p = (struct umessage_dxp_create_channel *)msghdr_p;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

	msg_p->um_chan_uuid_len = *(int16_t *)p;
	p += 2;
	len -= 2;
	memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
	msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
	p += msg_p->um_chan_uuid_len;
	len -= msg_p->um_chan_uuid_len;

	return rc;
}

static int
__encode_dxp_channel_keepalive(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	char *p = out_buf;
	uint32_t len = 0;

	encode_tx_hdr(p, &len, msghdr_p);

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_dxp_channel_keepalive(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	char *log_header = "__decode_dxp_channel_keepalive";

	nid_log_debug ("%s: start", log_header);

	decode_tx_hdr(p, &len, msghdr_p);

	return 0;
}

static int
__encode_dxp_channel_drop(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_drop_channel *msg_p = (struct umessage_dxp_drop_channel *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	p = encode_tx_hdr(p, &len, msghdr_p);

	sub_len = msg_p->um_chan_dxp_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_dxp_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;
	return 0;
}

static int
__decode_dxp_channel_drop(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	char *log_header = "__decode_dxp_channel_drop";
	struct umessage_dxp_drop_channel *msg_p = (struct umessage_dxp_drop_channel *)msghdr_p;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

	nid_log_debug ("%s: start", log_header);

	msg_p->um_chan_dxp_uuid_len = *(int16_t *)p;
	p += 2;
	len -= 2;
	memcpy(msg_p->um_chan_dxp_uuid, p, msg_p->um_chan_dxp_uuid_len);
	msg_p->um_chan_dxp_uuid[msg_p->um_chan_dxp_uuid_len] = 0;
	p += msg_p->um_chan_dxp_uuid_len;
	len -= msg_p->um_chan_dxp_uuid_len;

	return rc;
}

static int
__encode_dxp_channel_drop_resp(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_drop_channel_resp *msg_p = (struct umessage_dxp_drop_channel_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	p = encode_tx_hdr(p, &len, msghdr_p);

	sub_len = msg_p->um_chan_dxp_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_dxp_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_dxp_channel_drop_resp(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_drop_channel_resp *msg_p = (struct umessage_dxp_drop_channel_resp *)msghdr_p;

	p = decode_tx_hdr(p, &len, msghdr_p);

	msg_p->um_chan_dxp_uuid_len = *(int16_t *)p;
	p += 2;
	len -= 2;
	memcpy(msg_p->um_chan_dxp_uuid, p, msg_p->um_chan_dxp_uuid_len);
	msg_p->um_chan_dxp_uuid[msg_p->um_chan_dxp_uuid_len] = 0;
	p += msg_p->um_chan_dxp_uuid_len;
	len -= msg_p->um_chan_dxp_uuid_len;

	return 0;
}

static int
__encode_dxp_channel_stat(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_stat_channel *msg_p = (struct umessage_dxp_stat_channel *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	p = encode_tx_hdr(p, &len, msghdr_p);

	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;
	return 0;
}


static int
__decode_dxp_channel_stat(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_stat_channel *msg_p = (struct umessage_dxp_stat_channel *)msghdr_p;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

	msg_p->um_chan_uuid_len = *(int16_t *)p;
	p += 2;
	len -= 2;
	memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
	msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
	p += msg_p->um_chan_uuid_len;
	len -= msg_p->um_chan_uuid_len;

	return rc;
}

static int
__encode_dxp_channel_stat_resp(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_stat_channel_resp *msg_p = (struct umessage_dxp_stat_channel_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;
	uint64_t sub_len64;

	p = encode_tx_hdr(p, &len, msghdr_p);

	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	sub_len64 = msg_p->dsend_seq;
	*(uint64_t *)p = sub_len64;
	p += 8;
	len += 8;

	sub_len64 = msg_p->send_seq;
	*(uint64_t *)p = sub_len64;
	p += 8;
	len += 8;

	sub_len64 = msg_p->ack_seq;
	*(uint64_t *)p = sub_len64;
	p += 8;
	len += 8;

	sub_len64 = msg_p->recv_seq;
	*(uint64_t *)p = sub_len64;
	p += 8;
	len += 8;

	sub_len64 = msg_p->recv_dseq;
	*(uint64_t *)p = sub_len64;
	p += 8;
	len += 8;

	sub_len = msg_p->cmsg_left;
	*(uint32_t *)p = sub_len;
	p += 4;
	len += 4;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;
	return 0;
}


static int
__decode_dxp_channel_stat_resp(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_stat_channel_resp *msg_p = (struct umessage_dxp_stat_channel_resp *)msghdr_p;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

	msg_p->um_chan_uuid_len = *(int16_t *)p;
	p += 2;
	len -= 2;
	memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
	msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
	p += msg_p->um_chan_uuid_len;
	len -= msg_p->um_chan_uuid_len;

	msg_p->dsend_seq = *(uint64_t *)p;
	p += 8;
	len -= 8;

	msg_p->send_seq = *(uint64_t *)p;
	p += 8;
	len -= 8;

	msg_p->ack_seq = *(uint64_t *)p;
	p += 8;
	len -= 8;

	msg_p->recv_seq = *(uint64_t *)p;
	p += 8;
	len -= 8;

	msg_p->recv_dseq = *(uint64_t *)p;
	p += 8;
	len -= 8;

	msg_p->cmsg_left = *(uint32_t *)p;
	p += 4;
	len -= 4;

	return rc;
}

static int
__encode_dxp_start_work(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_start_work *msg_p = (struct umessage_dxp_start_work *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	p = encode_tx_hdr(p, &len, msghdr_p);

	sub_len = msg_p->um_chan_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + offsetof(struct umessage_tx_hdr, um_len);	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;
	return 0;
}


static int
__decode_dxp_start_work(char *p, uint32_t len, struct umessage_tx_hdr *msghdr_p)
{
	struct umessage_dxp_start_work *msg_p = (struct umessage_dxp_start_work *)msghdr_p;
	int rc = 0;

	p = decode_tx_hdr(p, &len, msghdr_p);

	msg_p->um_chan_uuid_len = *(int16_t *)p;
	p += 2;
	len -= 2;
	memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
	msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = 0;
	p += msg_p->um_chan_uuid_len;
	len -= msg_p->um_chan_uuid_len;

	return rc;
}

int
umpk_encode_dxp(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_dxp";
	struct umessage_tx_hdr *msghdr_p = (struct umessage_tx_hdr *)data;
	int rc;

	assert(out_buf && out_len && data);
	switch (msghdr_p->um_req) {
	case UMSG_DXP_CMD_CREATE_CHANNEL:
		switch (msghdr_p->um_req_code) {
		case UMSG_DXP_CODE_UUID:
			rc = __encode_dxp_create_channel_uuid(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DXP_CMD_KEEPALIVE:
		nid_log_debug ("%s:  keepalive message encode", log_header);
		rc = __encode_dxp_channel_keepalive(out_buf, out_len, msghdr_p);
		break;

	case UMSG_DXP_CMD_DROP_CHANNEL:
		nid_log_debug ("%s:  delete channel message encode", log_header);
		rc = __encode_dxp_channel_drop(out_buf, out_len, msghdr_p);
		break;

	case UMSG_DXP_CMD_DROP_CHANNEL_RESP:
		nid_log_debug ("%s:  delete channel response message encode", log_header);
		rc = __encode_dxp_channel_drop_resp(out_buf, out_len, msghdr_p);
		break;

	case UMSG_DXP_CMD_STAT_CHANNEL:
		switch (msghdr_p->um_req_code) {
		case UMSG_DXP_CODE_NO:
			nid_log_debug("%s: query channel status message encode", log_header);
			rc = __encode_dxp_channel_stat(out_buf, out_len, msghdr_p);
			break;
		case UMSG_DXP_CODE_STAT_RESP:
			nid_log_debug("%s: query channel status response message encode", log_header);
			rc = __encode_dxp_channel_stat_resp(out_buf, out_len, msghdr_p);
			break;
		}
		break;

	case UMSG_DXP_CMD_START_WORK:
		nid_log_debug ("%s:  start work message encode", log_header);
		rc = __encode_dxp_start_work(out_buf, out_len, msghdr_p);
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd (%d)", log_header, msghdr_p->um_req);
			break;
	}
	return rc;
}

int
umpk_decode_dxp(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_dxp";
	struct umessage_tx_hdr *msghdr_p = (struct umessage_tx_hdr *)data;
	int rc = 0;

	char* header_index = p;
	msghdr_p->um_req = *header_index++;
	msghdr_p->um_req_code = *header_index++;

	switch (msghdr_p->um_req) {
	case UMSG_DXP_CMD_CREATE_CHANNEL:
		switch (msghdr_p->um_req_code) {
		case UMSG_DXP_CODE_UUID:
			rc = __decode_dxp_create_channel_uuid(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DXP_CMD_KEEPALIVE:
		nid_log_debug("%s: keepalive message decode", log_header);
		rc = __decode_dxp_channel_keepalive(p, len, msghdr_p);
		break;

	case UMSG_DXP_CMD_DROP_CHANNEL:
		nid_log_debug("%s: delete channel message decode", log_header);
		rc = __decode_dxp_channel_drop(p, len, msghdr_p);
		break;

	case UMSG_DXP_CMD_DROP_CHANNEL_RESP:
		nid_log_debug("%s: delete channel response message decode", log_header);
		rc = __decode_dxp_channel_drop_resp(p, len, msghdr_p);
		break;

	case UMSG_DXP_CMD_STAT_CHANNEL:
		switch (msghdr_p->um_req_code) {
		case UMSG_DXP_CODE_NO:
			nid_log_debug("%s: query channel status message decode", log_header);
			rc = __decode_dxp_channel_stat(p, len, msghdr_p);
			break;
		case UMSG_DXP_CODE_STAT_RESP:
			nid_log_debug("%s: query channel status response message decode", log_header);
			rc = __decode_dxp_channel_stat_resp(p, len, msghdr_p);
			break;
		}
		break;

	case UMSG_DXP_CMD_START_WORK:
		nid_log_debug ("%s:  start work message decode", log_header);
		rc = __decode_dxp_start_work(p, len, msghdr_p);
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd (%d)", log_header, msghdr_p->um_req);
		break;
	}
	return rc;
}
