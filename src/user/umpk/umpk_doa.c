/*
 * umpk_rc.c
 * 	umpk for write cache
 */

#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_doa_if.h"

static int
__encode_doa_lock(char *out_buf, uint32_t *out_len, struct umessage_doa_hdr *msghdr_p)
{
	char *log_header = "__encode_doa_lock";
	assert( out_buf && out_len && msghdr_p );
	struct umessage_doa_information *msg_p = (struct umessage_doa_information*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;
	uint32_t sub_len;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	if (msg_p->um_doa_vid_len) {
		*p++ = UMSG_DOA_ITEM_VID;
		len++;
		sub_len = msg_p->um_doa_vid_len;
		*(uint16_t *)p = (uint16_t)sub_len;
		p += 2;
		len += 2;
		memcpy(p, msg_p->um_doa_vid, sub_len);
		p += sub_len;
		len += sub_len;
		nid_log_debug("vid_len:%u, vid = %s", msg_p->um_doa_vid_len, msg_p->um_doa_vid );

		nid_log_debug("size:%d", len);

	}

	if (msg_p->um_doa_lid_len) {
		*p++ = UMSG_DOA_ITEM_LID;
		len++;
		sub_len = msg_p->um_doa_lid_len;
		*(uint16_t *)p = (uint16_t)sub_len;
		p += 2;
		len += 2;
		memcpy(p, msg_p->um_doa_lid, sub_len);
		p += sub_len;
		len += sub_len;
		nid_log_debug("lid_len:%u, lid =%s", msg_p->um_doa_lid_len, msg_p->um_doa_lid);
		nid_log_debug("size:%d", len);

	}

	*p++ = UMSG_DOA_ITEM_TIME_OUT;
	len++;
	*(uint32_t *)p = (msg_p->um_doa_time_out);
	len += 4;
	p += 4;
	nid_log_debug("time_out");
	nid_log_debug("size:%d", len);


	*p++ = UMSG_DOA_ITEM_HOLD_TIME;
	len++;
	*(uint32_t *)p = (msg_p->um_doa_hold_time);
	len += 4;
	p += 4;
	nid_log_debug("hold_time");
	nid_log_debug("size:%d", len);


	*p++ = UMSG_DOA_ITEM_LOCK;
	len++;
	*(uint32_t *)p = (msg_p->um_doa_lock);
	len += 4;
	p += 4;
	nid_log_debug("doa_lock");
	nid_log_debug("size:%d", len);



	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

	return 0;
}

static int
__decode_doa_lock(char *p, uint32_t len, struct umessage_doa_hdr *msghdr_p)
{
  	assert( p && len && msghdr_p );
	char *log_header = "__decode_doa_lock";

	struct umessage_doa_information *res_p = (struct umessage_doa_information *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DOA_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_DOA_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_DOA_ITEM_VID:
			res_p->um_doa_vid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_doa_vid, p, res_p->um_doa_vid_len);
			res_p->um_doa_vid[res_p->um_doa_vid_len] = 0;
			p += res_p->um_doa_vid_len;
			len -= res_p->um_doa_vid_len;
			nid_log_debug("vid_len:%u, vid = %s", res_p->um_doa_vid_len, res_p->um_doa_vid );
			nid_log_debug("size:%d", len);
			break;

		case UMSG_DOA_ITEM_LID:
			res_p->um_doa_lid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_doa_lid, p, res_p->um_doa_lid_len);
			res_p->um_doa_lid[res_p->um_doa_lid_len] = 0;
			p += res_p->um_doa_lid_len;
			len -= res_p->um_doa_lid_len;
			nid_log_debug("lid_len:%u, lid = %s", res_p->um_doa_lid_len, res_p->um_doa_lid );
			nid_log_debug("size:%d", len);
			break;

		case UMSG_DOA_ITEM_TIME_OUT:
				nid_log_debug("NID_ITEM_TIME_OUT");
				res_p->um_doa_time_out = (*(uint32_t *)p);
				p += 4;
				len -=4;
				nid_log_debug("len_left:%d", len);
				break;

		case UMSG_DOA_ITEM_HOLD_TIME:
			 	nid_log_debug("NID_ITEM_HOLD_TIME");
			 	res_p->um_doa_hold_time = (*(uint32_t *)p);
			 	p += 4;
			 	len -=4;
			 	nid_log_debug("len_left:%d", len);
			 	break;

		case UMSG_DOA_ITEM_LOCK:
				nid_log_debug("NID_ITEM_DOA_LOCK");
				res_p->um_doa_lock = (*(uint32_t *)p);
				p += 4;
				len -=4;
				nid_log_debug("len_left:%d", len);
				break;



		default:
			nid_log_error("%s: got wrong item (%d)", log_header, item);
			nid_log_debug( "%s: req = %d, req code = %d",
				       log_header,
				       msghdr_p->um_req, 
				       msghdr_p->um_req_code );
			rc = -1;
			goto out;
		}
	}

out:
  	return rc;
}

static int
__encode_doa_old_lock(char *out_buf, uint32_t *out_len, struct umessage_doa_hdr *msghdr_p)
{
	char *log_header = "__encode_doa_lock";
	assert( out_buf && out_len && msghdr_p );
	struct umessage_doa_information *msg_p = (struct umessage_doa_information*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;
	uint32_t sub_len;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	*p++ = NID_ITEM_DOA_VID;
	len ++;
	sub_len = msg_p->um_doa_vid_len;
	*(uint16_t *)p = htobe16(sub_len);
	p += 2;
	len +=2;
	memcpy (p, msg_p->um_doa_vid, sub_len);
	p += sub_len;
	len += sub_len;
	nid_log_debug("vid_len:%u, vid = %s", msg_p->um_doa_vid_len, msg_p->um_doa_vid );

	nid_log_debug("size:%d", len);


	*p++ = NID_ITEM_DOA_LID;
	len ++;
	sub_len = msg_p->um_doa_lid_len;
	*(uint16_t *)p = htobe16(sub_len);
	p += 2;
	len +=2;
	memcpy (p, msg_p->um_doa_lid, sub_len);
	p += sub_len;
	len += sub_len;
	nid_log_debug("lid_len:%u, vid =%s", msg_p->um_doa_lid_len, msg_p->um_doa_lid);
	nid_log_debug("size:%d", len);


	*p++ = NID_ITEM_DOA_TIME_OUT;
	len++;
	*(uint32_t *)p = htobe32(msg_p->um_doa_time_out);
	len += 4;
	p += 4;
	nid_log_debug("time_out");
	nid_log_debug("size:%d", len);


	*p++ = NID_ITEM_DOA_HOLD_TIME;
	len++;
	*(uint32_t *)p = htobe32(msg_p->um_doa_hold_time);
	len += 4;
	p += 4;
	nid_log_debug("hold_time");
	nid_log_debug("size:%d", len);


	*p++ = NID_ITEM_DOA_LOCK;
	len++;
	*(uint32_t *)p = htobe32(msg_p->um_doa_lock);
	len += 4;
	p += 4;
	nid_log_debug("doa_lock");
	nid_log_debug("size:%d", len);

	p = out_buf + 2;
	*(uint32_t* )p = htobe32(len);
	msghdr_p->um_len = len;
	*out_len = len;

	return 0;
}

static int
__decode_doa_old_lock(char *p, uint32_t len, struct umessage_doa_hdr *msghdr_p)
{
	assert( p && len && msghdr_p );
	char *log_header = "__decode_doa_lock";

	struct umessage_doa_information *res_p = (struct umessage_doa_information *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DOA_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_DOA_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case NID_ITEM_DOA_VID:
			nid_log_debug("NID_ITEM_DOA_VID");
			res_p->um_doa_vid_len = be16toh(*(int16_t *)p);
			p += 2;
			len -= 2;
			memcpy(res_p->um_doa_vid, p, res_p->um_doa_vid_len);
			res_p->um_doa_vid[res_p->um_doa_vid_len] = 0;
			p += res_p->um_doa_vid_len;
			len -= res_p->um_doa_vid_len;
			nid_log_debug("vid_len:%u, vid= %s", res_p->um_doa_vid_len, res_p->um_doa_vid );
			nid_log_debug("len_left:%d", len);
			break;

		case NID_ITEM_DOA_LID:
			nid_log_debug("NID_ITEM_DOA_LID");
			res_p->um_doa_lid_len = be16toh(*(int16_t *)p);
			p += 2;
			len -= 2;
			memcpy(res_p->um_doa_lid, p, res_p->um_doa_lid_len);
			res_p->um_doa_lid[res_p->um_doa_lid_len] = 0;
			p += res_p->um_doa_lid_len;
			len -= res_p->um_doa_lid_len;
			nid_log_debug("vid_len:%u, lid=%s", res_p->um_doa_lid_len, res_p->um_doa_lid );
			nid_log_debug("len_left:%d", len);
			break;

		case NID_ITEM_DOA_TIME_OUT:
			nid_log_debug("NID_ITEM_TIME_OUT");
			res_p->um_doa_time_out = be32toh(*(uint32_t *)p);
			p += 4;
			len -=4;
			nid_log_debug("len_left:%d", len);
			break;

		case NID_ITEM_DOA_HOLD_TIME:
			nid_log_debug("NID_ITEM_HOLD_TIME");
			res_p->um_doa_hold_time = be32toh(*(uint32_t *)p);
			p += 4;
			len -=4;
			nid_log_debug("len_left:%d", len);
			break;

		case NID_ITEM_DOA_LOCK:
			nid_log_debug("NID_ITEM_DOA_LOCK");
			res_p->um_doa_lock = be32toh(*(uint32_t *)p);
			p += 4;
			len -=4;
			nid_log_debug("len_left:%d", len);
			break;

		default:
			nid_log_error("%s: got wrong item (%d)", log_header, item);
			nid_log_debug( "%s: req = %d, req code = %d",
				       log_header,
				       msghdr_p->um_req,
				       msghdr_p->um_req_code );
			rc = -1;
			goto out;
		}
	}

out:
	return rc;
}

static int
__encode_doa_hello(char *out_buf, uint32_t *out_len, struct umessage_doa_hdr *msghdr_p)
{
	char *log_header = "__encode_doa_hello";
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
__decode_doa_hello(char *p, int len, struct umessage_doa_hdr *msghdr_p)
{
	char *log_header = "__decode_doa_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_DOA_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_DOA_HEADER_LEN;

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
umpk_encode_doa(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "encode_type_doa";
	struct umessage_doa_hdr *msghdr_p = (struct umessage_doa_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_DOA_CMD:
		switch (msghdr_p->um_req_code) {
		case UMSG_DOA_CODE_REQUEST:
		case UMSG_DOA_CODE_CHECK:
		case UMSG_DOA_CODE_RELEASE:
			rc = __encode_doa_lock(out_buf, out_len, msghdr_p);
			break;

		case UMSG_DOA_CODE_OLD_REQUEST:
		case UMSG_DOA_CODE_OLD_CHECK:
		case UMSG_DOA_CODE_OLD_RELEASE:
			rc = __encode_doa_old_lock(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DOA_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_DOA_CODE_HELLO:
		case UMSG_DOA_CODE_RESP_HELLO:
			rc = __encode_doa_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_doa(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_doa";
	struct umessage_doa_hdr *msghdr_p = (struct umessage_doa_hdr *)data;
	int rc = 0;

	char* header_index = p;
	assert(len >= UMSG_DOA_HEADER_LEN);
	msghdr_p->um_req = *header_index++;
	msghdr_p->um_req_code = *header_index++;

	switch (msghdr_p->um_req) {
	case UMSG_DOA_CMD:
		switch (msghdr_p->um_req_code) {
		case UMSG_DOA_CODE_REQUEST:
		case UMSG_DOA_CODE_CHECK:
		case UMSG_DOA_CODE_RELEASE:
			rc = __decode_doa_lock(p, len, msghdr_p);
			break;

		case UMSG_DOA_CODE_OLD_REQUEST:
		case UMSG_DOA_CODE_OLD_CHECK:
		case UMSG_DOA_CODE_OLD_RELEASE:
			rc = __decode_doa_old_lock(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_DOA_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_DOA_CODE_HELLO:
		case UMSG_DOA_CODE_RESP_HELLO:
			rc = __decode_doa_hello(p, len, msghdr_p);
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
