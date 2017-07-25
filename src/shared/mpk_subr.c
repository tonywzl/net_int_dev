/*
 * mpk_subr.c
 *
 *  Created on: Jun 22, 2015
 *      Author: root
 */


#ifdef __KERNEL__
//#include <asm/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#else
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#endif

#include "nid_log.h"
#include "nid_shared.h"
#include "mpk_if.h"


static int
__encode_bio_release(char *p, int *len, struct nid_message_hdr *msghdr_p)
{
	struct nid_message_bio_release *msg_p = (struct nid_message_bio_release *)msghdr_p;
	int size = *len, sub_len;

	*p++ = NID_ITEM_BIO_CHANNEL;
	size++;
	sub_len = msg_p->m_len;
	*(uint16_t *)p = x_htobe16(sub_len);
	p += 2;
	size += 2;
	memcpy (p, msg_p->m_chan_uuid, sub_len);
	p += sub_len;
	size += sub_len;

	*len = size;
	return 0;
}

static int
__decode_bio_release(char *p, int *left, struct nid_message_hdr *reshdr_p)
{
	char *log_header = "__decode_bio_release";
	struct nid_message_bio_release *res_p = (struct nid_message_bio_release *)reshdr_p;
	int rc = 0;
	int8_t item;
	int size = *left;

	while (size) {
		size -= 1;
		item = *p;
		switch (*p++) {
		case NID_ITEM_BIO_CHANNEL:
			res_p->m_len = x_be16toh(*(int16_t *)p);
			p += 2;
			size -= 2;
			memcpy(res_p->m_chan_uuid, p, res_p->m_len);
			res_p->m_chan_uuid[res_p->m_len] = 0;
			p += res_p->m_len;
			size -= res_p->m_len;
			break;

		default:
			nid_log_error("%s: got wrong item (%d)", log_header, item);
			rc = -1;
			break;
		}
	}

	return rc;
}

static int
__encode_type_bio_vec(char *p, int *len, struct nid_message_hdr *msghdr_p)
{
	struct nid_message_bio_vec *msg_p = (struct nid_message_bio_vec *)msghdr_p;
	int size = *len;

	*p++ = NID_ITEM_BIO_VEC_FLUSHNUM;
	size += 1;
	*(uint32_t *)p = x_htobe32(msg_p->m_vec_flushnum);
	size += 4;
	p += 4;

	*p++ = NID_ITEM_BIO_VEC_VECNUM;
	size +=1;
	*(uint64_t *)p = x_htobe64(msg_p->m_vec_vecnum);
	size += 8;
	p += 8;

	*p++ = NID_ITEM_BIO_VEC_WRITESZ;
	size +=1;
	*(uint64_t *)p = x_htobe64(msg_p->m_vec_writesz);
	size += 8;
	p += 8;
	*len = size;

	return 0;
}

int
encode_type_doa_lock (char *p, int *len, struct nid_message_hdr * msghdr_p)
{
	struct nid_message_doa *msg_p = (struct nid_message_doa *)msghdr_p;
	int size;
	uint16_t sub_len;
	size = *len;

		*p++ = NID_ITEM_DOA_VID;
		size ++;
		sub_len = msg_p->m_doa_vid_len;
		*(uint16_t *)p = x_htobe16(sub_len);
		p += 2;
		size +=2;
		memcpy (p, msg_p->m_doa_vid, sub_len);
		p += sub_len;
		size += sub_len;
		nid_log_debug("vid_len:%u, vid = %s", msg_p->m_doa_vid_len, msg_p->m_doa_vid );

		nid_log_debug("size:%d", size);


		*p++ = NID_ITEM_DOA_LID;
		size ++;
		sub_len = msg_p->m_doa_lid_len;
		*(uint16_t *)p = x_htobe16(sub_len);
		p += 2;
		size +=2;
		memcpy (p, msg_p->m_doa_lid, sub_len);
		p += sub_len;
		size += sub_len;
		nid_log_debug("lid_len:%u, vid =%s", msg_p->m_doa_lid_len, msg_p->m_doa_lid);
		nid_log_debug("size:%d", size);


		*p++ = NID_ITEM_DOA_TIME_OUT;
		size++;
		*(uint32_t *)p = x_htobe32(msg_p->m_doa_time_out);
		size += 4;
		p += 4;
		nid_log_debug("time_out");
		nid_log_debug("size:%d", size);


		*p++ = NID_ITEM_DOA_HOLD_TIME;
		size++;
		*(uint32_t *)p = x_htobe32(msg_p->m_doa_hold_time);
		size += 4;
		p += 4;
		nid_log_debug("hold_time");
		nid_log_debug("size:%d", size);


		*p++ = NID_ITEM_DOA_LOCK;
		size++;
		*(uint32_t *)p = x_htobe32(msg_p->m_doa_lock);
		size += 4;
		p += 4;
		nid_log_debug("doa_lock");
		nid_log_debug("size:%d", size);


	*len = size;

	nid_log_error("encode_len: %d", size);
	return 0;
}

int
decode_type_doa_lock (char *p, int *left, struct nid_message_hdr *reshdr_p)
{
	char *log_header = "decode_type_doa";
	struct nid_message_doa *res_p = (struct nid_message_doa *)reshdr_p;
	int rc = 0;
	int8_t item;
	int size;
	size = *left;
	nid_log_error("decode len: %d", size);
	while (size){
		size -= 1;
		item = *p;
		switch (*p++){
			case NID_ITEM_DOA_VID:
				nid_log_debug("NID_ITEM_DOA_VID");
				res_p->m_doa_vid_len = x_be16toh(*(int16_t *)p);
				p += 2;
				size -= 2;
				res_p->m_doa_vid = p;
				p += res_p->m_doa_vid_len;
				size -= res_p->m_doa_vid_len;
				nid_log_debug("vid_len:%u, vid= %s", res_p->m_doa_vid_len, res_p->m_doa_vid );
				nid_log_debug("len_left:%d", size);
				break;

			case NID_ITEM_DOA_LID:
				 nid_log_debug("NID_ITEM_DOA_LID");
				res_p->m_doa_lid_len = x_be16toh(*(int16_t *)p);
				p += 2;
				size -= 2;
				res_p->m_doa_lid = p;
				p += res_p->m_doa_lid_len;
				size -= res_p->m_doa_lid_len;
				nid_log_debug("vid_len:%u, lid=%s", res_p->m_doa_lid_len, res_p->m_doa_lid );
				nid_log_debug("len_left:%d", size);
				break;

			case NID_ITEM_DOA_TIME_OUT:
				nid_log_debug("NID_ITEM_TIME_OUT");
				res_p->m_doa_time_out = x_be32toh(*(uint32_t *)p);
				p += 4;
				size -=4;
				nid_log_debug("len_left:%d", size);
				break;

			case NID_ITEM_DOA_HOLD_TIME:
				 nid_log_debug("NID_ITEM_HOLD_TIME");
				res_p->m_doa_hold_time = x_be32toh(*(uint32_t *)p);
				p += 4;
				size -=4;
				nid_log_debug("len_left:%d", size);
				break;

			case NID_ITEM_DOA_LOCK:
				 nid_log_debug("NID_ITEM_DOA_LOCK");
				res_p->m_doa_lock = x_be32toh(*(uint32_t *)p);
				p += 4;
				size -=4;
				nid_log_debug("len_left:%d", size);
				break;

			default:
				rc = -1;
				nid_log_debug("%s: unknown item %d", log_header, item);
				break;

		}
	}
	*left = size;
//	rc = 0;
	return rc;

}

int
decode_type_bio_vec (char * p, int *left, struct nid_message_hdr * reshdr_p)
{
	char *log_header = "decode_type_bio_vec";
	struct nid_message_bio_vec *res_p = (struct nid_message_bio_vec *)reshdr_p;
	int rc;
	int8_t item;
	int size;
	size = *left;
	nid_log_debug("decode len: %d", size);
	while (size){
		size -=1;
		item = *p;
		switch (*p++){
		case NID_ITEM_BIO_VEC_FLUSHNUM:
			res_p->m_vec_flushnum = x_be32toh(*(uint32_t *)p);
			p += 4;
			size -=4;
			break;
		case NID_ITEM_BIO_VEC_VECNUM:
			res_p->m_vec_vecnum = x_be64toh(*(uint64_t *)p);
			p += 8;
			size -=8;
			break;
		case NID_ITEM_BIO_VEC_WRITESZ:
			res_p->m_vec_writesz = x_be64toh(*(uint64_t *)p);
			p += 8;
			size -=8;
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown item %d", log_header, item);
			break;
		}


	}
	*left = size;
	rc = 0;
	return rc;
}

int
encode_type_bio(char *p, int *len, struct nid_message_hdr *msghdr_p)
{
	int rc;
	char *log_header = "encode_type_bio";

#ifndef __KERNEL__
	assert(msghdr_p->m_req == NID_REQ_BIO);
#endif
	switch (msghdr_p->m_req_code){
	case NID_REQ_BIO_VEC_STOP:
	case NID_REQ_BIO_VEC_STAT:
		rc = __encode_type_bio_vec(p, len, msghdr_p);
		break;

	case NID_REQ_BIO_RELEASE_START:
	case NID_REQ_BIO_RELEASE_STOP:
		rc = __encode_bio_release(p, len, msghdr_p);
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd", log_header);
		break;
	}

	return rc;
}

int
decode_type_bio(char * p, int *left, struct nid_message_hdr * msghdr_p)
{
	char *log_header = "decode_type_bio";
	struct nid_message_bio_vec *res_p = (struct nid_message_bio_vec *)msghdr_p;
	int rc = 0;

#ifndef __KERNEL__
	assert(msghdr_p->m_req == NID_REQ_BIO);
#endif
	switch (res_p->m_req_code){
	case NID_REQ_BIO_VEC_STOP:
	case NID_REQ_BIO_VEC_STAT:
		rc = decode_type_bio_vec(p, left, msghdr_p);
		break;

	case NID_REQ_BIO_RELEASE_START:
	case NID_REQ_BIO_RELEASE_STOP:
		rc = __decode_bio_release(p, left, msghdr_p);
		break;

	default:
		rc = -1;
		nid_log_error("%s: unknown cmd", log_header);
		break;
	}

	return rc;
}

int
encode_type_doa (char *p, int *len, struct nid_message_hdr *msghdr_p)
{
	int rc;
	char *log_header = "encode_type_doa";
	struct nid_message_doa *res_p = (struct nid_message_doa *)msghdr_p;

	switch (res_p->m_req_code){
		case NID_REQ_DOA_REQUEST:
			rc = encode_type_doa_lock(p, len, msghdr_p);
			break;

		case NID_REQ_DOA_CHECK:
			rc = encode_type_doa_lock(p, len, msghdr_p);
			break;

		case NID_REQ_DOA_RELEASE:
			rc = encode_type_doa_lock(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd", log_header);
			break;
	}
	return rc;
}

int
decode_type_doa (char *p, int *len, struct nid_message_hdr *msghdr_p)
{
	int rc;
	char *log_header = "decode_type_doa";
	struct nid_message_doa *res_p = (struct nid_message_doa *)msghdr_p;

	switch (res_p->m_req_code){
		case NID_REQ_DOA_REQUEST:
			rc = decode_type_doa_lock(p, len, msghdr_p);
			break;

		case NID_REQ_DOA_CHECK:
			rc = decode_type_doa_lock(p, len, msghdr_p);
			break;

		case NID_REQ_DOA_RELEASE:
			rc = decode_type_doa_lock(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd", log_header);
			break;
	}
	return rc;
}
