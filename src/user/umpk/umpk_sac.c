/*
 * umpk_sac.c
 * 	umpk for Server agent channel
 */

#include <string.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_sac_if.h"

static int
__encode_sac_information(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_information";
	struct umessage_sac_information *msg_p = (struct umessage_sac_information *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

        /* sac channel uuid */
        item = UMSG_SAC_ITEM_CHANNEL;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_information(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_information";
	struct umessage_sac_information *msg_p = (struct umessage_sac_information *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
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
__encode_sac_add(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_add";
	struct umessage_sac_add *msg_p = (struct umessage_sac_add *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

        /* sac channel uuid */
        item = UMSG_SAC_ITEM_CHANNEL;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* sac sync */
        item = UMSG_SAC_ITEM_SYNC;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_sync);
	*(typeof(msg_p->um_sync) *)p = msg_p->um_sync;
	len += sub_len;
	p += sub_len;

	/* sac direct io */
        item = UMSG_SAC_ITEM_DIRECT_IO;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_direct_io);
	*(typeof(msg_p->um_direct_io) *)p = msg_p->um_direct_io;
	len += sub_len;
	p += sub_len;

	/* sac enable kill myself*/
        item = UMSG_SAC_ITEM_ENABLE_KILL_MYSELF;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_enable_kill_myself);
	*(typeof(msg_p->um_enable_kill_myself) *)p = msg_p->um_enable_kill_myself;
	len += sub_len;
	p += sub_len;

	/* sac alignment */
        item = UMSG_SAC_ITEM_ALIGNMENT;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_alignment);
	*(typeof(msg_p->um_alignment) *)p = msg_p->um_alignment;
	len += sub_len;
	p += sub_len;

	/* sac ds name */
        item = UMSG_SAC_ITEM_DS;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_ds_name_len);
	*(typeof(msg_p->um_ds_name_len) *)p = msg_p->um_ds_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_ds_name_len;
	memcpy(p, msg_p->um_ds_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* sac dev name */
        item = UMSG_SAC_ITEM_DEV;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_dev_name_len);
	*(typeof(msg_p->um_dev_name_len) *)p = msg_p->um_dev_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_dev_name_len;
	memcpy(p, msg_p->um_dev_name, sub_len);
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

	sub_len = sizeof(msg_p->um_dev_size);
	*(typeof(msg_p->um_dev_size) *)p = msg_p->um_dev_size;
	len += sub_len;
	p += sub_len;

	/* sac tp name */
        item = UMSG_SAC_ITEM_TP;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_tp_name_len);
	*(typeof(msg_p->um_tp_name_len) *)p = msg_p->um_tp_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_tp_name_len;
	memcpy(p, msg_p->um_tp_name, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_add(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_add";
	struct umessage_sac_add *msg_p = (struct umessage_sac_add *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_SYNC:
			msg_p->um_sync = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			break;

		case UMSG_SAC_ITEM_DIRECT_IO:
			msg_p->um_direct_io = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			break;

		case UMSG_SAC_ITEM_ENABLE_KILL_MYSELF:
			msg_p->um_enable_kill_myself = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			break;

		case UMSG_SAC_ITEM_ALIGNMENT:
			msg_p->um_alignment = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_DS:
			msg_p->um_ds_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_ds_name, p, msg_p->um_ds_name_len);
			msg_p->um_ds_name[msg_p->um_ds_name_len] = '\0';
			p += msg_p->um_ds_name_len;
			len -= msg_p->um_ds_name_len;
			break;

		case UMSG_SAC_ITEM_DEV:
			msg_p->um_dev_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_dev_name, p, msg_p->um_dev_name_len);
			msg_p->um_dev_name[msg_p->um_dev_name_len] = '\0';
			p += msg_p->um_dev_name_len;
			len -= msg_p->um_dev_name_len;

			msg_p->um_exportname_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_exportname, p, msg_p->um_exportname_len);
			msg_p->um_exportname[msg_p->um_exportname_len] = '\0';
			p += msg_p->um_exportname_len;
			len -= msg_p->um_exportname_len;

			msg_p->um_dev_size = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_TP:
			msg_p->um_tp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
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
__encode_sac_delete(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_del";
	struct umessage_sac_delete *msg_p = (struct umessage_sac_delete *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

        /* sac channel uuid */
        item = UMSG_SAC_ITEM_CHANNEL;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_delete(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_delete";
	struct umessage_sac_delete *msg_p = (struct umessage_sac_delete *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
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
__encode_sac_set_keepalive(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_set_keepalive";
	struct umessage_sac_set_keepalive *msg_p = (struct umessage_sac_set_keepalive *)msghdr_p;
	char *p = out_buf, *len_p;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);

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

	/* sac channel uuid */
	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_enable_keepalive);
	memcpy(p, &msg_p->um_enable_keepalive, sub_len);
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_max_keepalive);
	memcpy(p, &msg_p->um_max_keepalive, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_set_keepalive(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_set_keepalive";
	struct umessage_sac_set_keepalive *msg_p = (struct umessage_sac_set_keepalive *)msghdr_p;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		msg_p->um_chan_uuid_len = *(int16_t *)p;
		p += sizeof(int16_t);
		len -= sizeof(int16_t);
		memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
		msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
		p += msg_p->um_chan_uuid_len;
		len -= msg_p->um_chan_uuid_len;
		msg_p->um_enable_keepalive = *(int16_t *)p;
		p += sizeof(int16_t);
		len -= sizeof(int16_t);
		msg_p->um_max_keepalive = *(int16_t *)p;
		p += sizeof(int16_t);
		len -= sizeof(int16_t);
	}
	assert(len == 0);

	nid_log_warning("%s: uuid:%s", log_header, msg_p->um_chan_uuid);

	return rc;
}


static int
__encode_sac_switch(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_switch";
	struct umessage_sac_switch *msg_p = (struct umessage_sac_switch *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

	/* sac channel uuid */
	item = UMSG_SAC_ITEM_CHANNEL;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	 /* sac new bwc */
	item = UMSG_SAC_ITEM_BWC;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_bwc_uuid_len);
	*(typeof(msg_p->um_bwc_uuid_len) *)p = msg_p->um_bwc_uuid_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_bwc_uuid_len;
	memcpy(p, msg_p->um_bwc_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_switch(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_switch";
	struct umessage_sac_switch *msg_p = (struct umessage_sac_switch *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_BWC:
			msg_p->um_bwc_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_bwc_uuid, p, msg_p->um_bwc_uuid_len);
			msg_p->um_bwc_uuid[msg_p->um_bwc_uuid_len] = '\0';
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
__encode_sac_fast_release(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_fast_release";
	struct umessage_sac_fast_release *msg_p = (struct umessage_sac_fast_release *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

	/* sac channel uuid */
	item = UMSG_SAC_ITEM_CHANNEL;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_fast_release(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_fast_release";
	struct umessage_sac_fast_release *msg_p = (struct umessage_sac_fast_release *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
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
__encode_sac_ioinfo(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_ioinfo";
	struct umessage_sac_ioinfo *msg_p = (struct umessage_sac_ioinfo *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

        /* sac channel uuid */
        item = UMSG_SAC_ITEM_CHANNEL;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_ioinfo(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_ioinfo";
	struct umessage_sac_ioinfo *msg_p = (struct umessage_sac_ioinfo *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
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
__encode_sac_display(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_display";
	struct umessage_sac_display *msg_p = (struct umessage_sac_display *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

        /* sac channel uuid */
        item = UMSG_SAC_ITEM_CHANNEL;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_display(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_display";
	struct umessage_sac_display *msg_p = (struct umessage_sac_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
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
__encode_sac_information_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_information_resp";
	struct umessage_sac_information_resp *msg_p = (struct umessage_sac_information_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

	/* sac channel uuid */
	item = UMSG_SAC_ITEM_CHANNEL;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* ip */
	item = UMSG_SAC_ITEM_IP;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_ip_len);
	*(typeof(msg_p->um_ip_len) *)p = msg_p->um_ip_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_ip_len;
	memcpy(p, msg_p->um_ip, sub_len);
	len += sub_len;
	p += sub_len;

	/* bfe page counter */
	item = UMSG_SAC_ITEM_BFE_COUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_bfe_page_counter);
	*(typeof(msg_p->um_bfe_page_counter) *)p = msg_p->um_bfe_page_counter;
	p += sub_len;
	len += sub_len;

	/* bre page counter */
	item = UMSG_SAC_ITEM_BRE_COUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_bre_page_counter);
	*(typeof(msg_p->um_bre_page_counter) *)p = msg_p->um_bre_page_counter;
	p += sub_len;
	len += sub_len;

	/* state */
	item = UMSG_SAC_ITEM_STATE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_state);
	*(typeof(msg_p->um_state) *)p = msg_p->um_state;
	p += sub_len;
	len += sub_len;

	/* alevel */
	item = UMSG_SAC_ITEM_ALEVEL;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_alevel);
	*(typeof(msg_p->um_alevel) *)p = msg_p->um_alevel;
	p += sub_len;
	len += sub_len;

	/* rcounter */
	item = UMSG_SAC_ITEM_RCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rcounter);
	*(typeof(msg_p->um_rcounter) *)p = msg_p->um_rcounter;
	p += sub_len;
	len += sub_len;

	/* rrcounter */
	item = UMSG_SAC_ITEM_RRCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_r_rcounter);
	*(typeof(msg_p->um_r_rcounter) *)p = msg_p->um_r_rcounter;
	p += sub_len;
	len += sub_len;

	/* rreadycounter */
	item = UMSG_SAC_ITEM_RREADYCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rreadycounter);
	*(typeof(msg_p->um_rreadycounter) *)p = msg_p->um_rreadycounter;
	p += sub_len;
	len += sub_len;

	/* wcounter */
	item = UMSG_SAC_ITEM_WCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_wcounter);
	*(typeof(msg_p->um_wcounter) *)p = msg_p->um_wcounter;
	p += sub_len;
	len += sub_len;

	/* wreadycounter */
	item = UMSG_SAC_ITEM_WREADYCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_wreadycounter);
	*(typeof(msg_p->um_wreadycounter) *)p = msg_p->um_wreadycounter;
	p += sub_len;
	len += sub_len;

	/* rwcounter */
	item = UMSG_SAC_ITEM_RWCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_r_wcounter);
	*(typeof(msg_p->um_r_wcounter) *)p = msg_p->um_r_wcounter;
	p += sub_len;
	len += sub_len;

	/* kcounter */
	item = UMSG_SAC_ITEM_KCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_kcounter);
	*(typeof(msg_p->um_kcounter) *)p = msg_p->um_kcounter;
	p += sub_len;
	len += sub_len;

	/* rkcounter */
	item = UMSG_SAC_ITEM_RKCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_r_kcounter);
	*(typeof(msg_p->um_r_kcounter) *)p = msg_p->um_r_kcounter;
	p += sub_len;
	len += sub_len;

	/* recv_sequence */
	item = UMSG_SAC_ITEM_RSEQUENCE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_recv_sequence);
	*(typeof(msg_p->um_recv_sequence) *)p = msg_p->um_recv_sequence;
	p += sub_len;
	len += sub_len;

	/* wait_sequence */
	item = UMSG_SAC_ITEM_WSEQUENCE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_wait_sequence);
	*(typeof(msg_p->um_wait_sequence) *)p = msg_p->um_wait_sequence;
	p += sub_len;
	len += sub_len;

	/* live_time */
	item = UMSG_SAC_ITEM_TIME;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_live_time);
	*(typeof(msg_p->um_live_time) *)p = msg_p->um_live_time;
	p += sub_len;
	len += sub_len;

	/* rtree_segsz */
	item = UMSG_SAC_ITEM_RTREE_SEGSZ;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rtree_segsz);
	*(typeof(msg_p->um_rtree_segsz) *)p = msg_p->um_rtree_segsz;
	p += sub_len;
	len += sub_len;

	/* rtree_nseg */
	item = UMSG_SAC_ITEM_RTREE_NSEG;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rtree_nseg);
	*(typeof(msg_p->um_rtree_nseg) *)p = msg_p->um_rtree_nseg;
	p += sub_len;
	len += sub_len;

	/* rtree_nfree */
	item = UMSG_SAC_ITEM_RTREE_NFREE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rtree_nfree);
	*(typeof(msg_p->um_rtree_nfree) *)p = msg_p->um_rtree_nfree;
	p += sub_len;
	len += sub_len;

	/* rtree_nused */
	item = UMSG_SAC_ITEM_RTREE_NUSED;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rtree_nused);
	*(typeof(msg_p->um_rtree_nused) *)p = msg_p->um_rtree_nused;
	p += sub_len;
	len += sub_len;

	/* btn_segsz */
	item = UMSG_SAC_ITEM_BTN_SEGSZ;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_btn_segsz);
	*(typeof(msg_p->um_btn_segsz) *)p = msg_p->um_btn_segsz;
	p += sub_len;
	len += sub_len;

	/* btn_nseg */
	item = UMSG_SAC_ITEM_BTN_NSEG;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_btn_nseg);
	*(typeof(msg_p->um_btn_nseg) *)p = msg_p->um_btn_nseg;
	p += sub_len;
	len += sub_len;

	/* btn_nfree */
	item = UMSG_SAC_ITEM_BTN_NFREE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_btn_nfree);
	*(typeof(msg_p->um_btn_nfree) *)p = msg_p->um_btn_nfree;
	p += sub_len;
	len += sub_len;

	/* btn_nused */
	item = UMSG_SAC_ITEM_BTN_NUSED;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_btn_nused);
	*(typeof(msg_p->um_btn_nused) *)p = msg_p->um_btn_nused;
	p += sub_len;
	len += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_information_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_information_resp";
	struct umessage_sac_information_resp *msg_p = (struct umessage_sac_information_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_IP:
			msg_p->um_ip_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_ip, p, msg_p->um_ip_len);
			msg_p->um_ip[msg_p->um_ip_len] = '\0';
			p += msg_p->um_ip_len;
			len -= msg_p->um_ip_len;
			break;

		case UMSG_SAC_ITEM_BFE_COUNTER:
			msg_p->um_bfe_page_counter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_BRE_COUNTER:
			msg_p->um_bre_page_counter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_STATE:
			msg_p->um_state = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			break;

		case UMSG_SAC_ITEM_ALEVEL:
			msg_p->um_alevel = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			break;

		case UMSG_SAC_ITEM_RCOUNTER:
			msg_p->um_rcounter = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_RRCOUNTER:
			msg_p->um_r_rcounter = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_RREADYCOUNTER:
			msg_p->um_rreadycounter = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_WCOUNTER:
			msg_p->um_wcounter = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_WREADYCOUNTER:
			msg_p->um_wreadycounter = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_RWCOUNTER:
			msg_p->um_r_wcounter = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_KCOUNTER:
			msg_p->um_kcounter = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_RKCOUNTER:
			msg_p->um_r_kcounter = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_RSEQUENCE:
			msg_p->um_recv_sequence = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_WSEQUENCE:
			msg_p->um_wait_sequence = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
			break;

		case UMSG_SAC_ITEM_TIME:
			msg_p->um_live_time = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_RTREE_SEGSZ:
			msg_p->um_rtree_segsz = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_RTREE_NSEG:
			msg_p->um_rtree_nseg = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_RTREE_NFREE:
			msg_p->um_rtree_nfree = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_RTREE_NUSED:
			msg_p->um_rtree_nused = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_BTN_SEGSZ:
			msg_p->um_btn_segsz = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_BTN_NSEG:
			msg_p->um_btn_nseg = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_BTN_NFREE:
			msg_p->um_btn_nfree = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_BTN_NUSED:
			msg_p->um_btn_nused = *(int32_t *)p;
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
__encode_sac_list_stat_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_list_stat_resp";
	struct umessage_sac_list_stat_resp *msg_p = (struct umessage_sac_list_stat_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

	/* sac channel uuid */
	item = UMSG_SAC_ITEM_CHANNEL;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	item = UMSG_SAC_ITEM_REQ_COUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_req_counter);
	*(typeof(msg_p->um_req_counter) *)p = msg_p->um_req_counter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_TO_REQ_COUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_to_req_counter);
	*(typeof(msg_p->um_to_req_counter) *)p = msg_p->um_to_req_counter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_RCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rcounter);
	*(typeof(msg_p->um_rcounter) *)p = msg_p->um_rcounter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_TO_RCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_to_rcounter);
	*(typeof(msg_p->um_to_rcounter) *)p = msg_p->um_to_rcounter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_WCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_wcounter);
	*(typeof(msg_p->um_wcounter) *)p = msg_p->um_wcounter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_TO_WCOUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_to_wcounter);
	*(typeof(msg_p->um_to_wcounter) *)p = msg_p->um_to_wcounter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_RESP_COUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_resp_counter);
	*(typeof(msg_p->um_resp_counter) *)p = msg_p->um_resp_counter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_TO_RESP_COUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_to_resp_counter);
	*(typeof(msg_p->um_to_resp_counter) *)p = msg_p->um_to_resp_counter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_DRESP_COUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_dresp_counter);
	*(typeof(msg_p->um_dresp_counter) *)p = msg_p->um_dresp_counter;
	p += sub_len;
	len += sub_len;

	item = UMSG_SAC_ITEM_TO_DRESP_COUNTER;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_to_dresp_counter);
	*(typeof(msg_p->um_to_dresp_counter) *)p = msg_p->um_to_dresp_counter;
	p += sub_len;
	len += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_list_stat_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_list_stat_resp";
	struct umessage_sac_list_stat_resp *msg_p = (struct umessage_sac_list_stat_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_REQ_COUNTER:
			msg_p->um_req_counter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_TO_REQ_COUNTER:
			msg_p->um_to_req_counter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_RCOUNTER:
			msg_p->um_rcounter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_TO_RCOUNTER:
			msg_p->um_to_rcounter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_WCOUNTER:
			msg_p->um_wcounter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_TO_WCOUNTER:
			msg_p->um_to_wcounter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_RESP_COUNTER:
			msg_p->um_resp_counter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_TO_RESP_COUNTER:
			msg_p->um_to_resp_counter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_DRESP_COUNTER:
			msg_p->um_dresp_counter = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_TO_DRESP_COUNTER:
			msg_p->um_to_dresp_counter = *(int32_t *)p;
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
__encode_sac_add_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_add_resp";
	struct umessage_sac_add_resp *msg_p = (struct umessage_sac_add_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

	/* sac channel uuid */
	item = UMSG_SAC_ITEM_CHANNEL;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
	item = UMSG_SAC_ITEM_RESP_CODE;
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
__decode_sac_add_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_add_resp";
	struct umessage_sac_add_resp *msg_p = (struct umessage_sac_add_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_RESP_CODE:
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
__encode_sac_delete_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_delete_resp";
	struct umessage_sac_delete_resp *msg_p = (struct umessage_sac_delete_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

	/* sac channel uuid */
	item = UMSG_SAC_ITEM_CHANNEL;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
	item = UMSG_SAC_ITEM_RESP_CODE;
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
__decode_sac_delelte_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_delelte_resp";
	struct umessage_sac_delete_resp *msg_p = (struct umessage_sac_delete_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_RESP_CODE:
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
__encode_sac_switch_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_delete_resp";
	struct umessage_sac_switch_resp *msg_p = (struct umessage_sac_switch_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

	/* sac channel uuid */
	item = UMSG_SAC_ITEM_CHANNEL;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
	item = UMSG_SAC_ITEM_RESP_CODE;
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
__decode_sac_switch_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_delelte_resp";
	struct umessage_sac_switch_resp *msg_p = (struct umessage_sac_switch_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_RESP_CODE:
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
__encode_sac_set_keepalive_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_set_keepalive_resp";
	struct umessage_sac_set_keepalive_resp *msg_p = (struct umessage_sac_set_keepalive_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

	/* channel uuid */
	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* response code */
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
__decode_sac_set_keepalive_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_set_keepalive_resp";
	struct umessage_sac_set_keepalive_resp *msg_p = (struct umessage_sac_set_keepalive_resp *)msghdr_p;
	int rc = 0;


	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	msg_p->um_chan_uuid_len = *(int16_t *)p;
	p += sizeof(int16_t);
	len -= sizeof(int16_t);
	memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
	msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
	p += msg_p->um_chan_uuid_len;
	len -= msg_p->um_chan_uuid_len;
	msg_p->um_resp_code = *(int8_t *)p;
	p += sizeof(int8_t);
	len -= sizeof(int8_t);

	nid_log_warning("%s: uuid_len:%s", log_header, msg_p->um_chan_uuid);

	return rc;
}

static int
__encode_sac_fast_release_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_fast_release_resp";
	struct umessage_sac_fast_release_resp *msg_p = (struct umessage_sac_fast_release_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint32_t len = 0, sub_len;
	uint8_t item;

	nid_log_warning("%s:req:%d, req_code:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code);
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

	/* response code */
	item = UMSG_SAC_ITEM_RESP_CODE;
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
__decode_sac_fast_release_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_fast_release_resp";
	struct umessage_sac_fast_release_resp *msg_p = (struct umessage_sac_fast_release_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_RESP_CODE:
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
__encode_sac_ioinfo_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_ioinfo_resp";
	struct umessage_sac_ioinfo_resp *msg_p = (struct umessage_sac_ioinfo_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

        /* sac channel uuid */
        item = UMSG_SAC_ITEM_CHANNEL;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* sac sync */
        item = UMSG_SAC_ITEM_IOINFO;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_is_running);
	*(typeof(msg_p->um_is_running) *)p = msg_p->um_is_running;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_req_num);
	*(typeof(msg_p->um_req_num) *)p = msg_p->um_req_num;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_req_len);
	*(typeof(msg_p->um_req_len) *)p = msg_p->um_req_len;
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_ioinfo_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_ioinfo_resp";
	struct umessage_sac_ioinfo_resp *msg_p = (struct umessage_sac_ioinfo_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_IOINFO:
			msg_p->um_is_running = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);

			msg_p->um_req_num = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);

			msg_p->um_req_len = *(int64_t *)p;
			p += sizeof(int64_t);
			len -= sizeof(int64_t);
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
__encode_sac_display_resp(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_display_resp";
	struct umessage_sac_display_resp *msg_p = (struct umessage_sac_display_resp *)msghdr_p;
	char *p = out_buf, *len_p;
	uint8_t item;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
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

        /* sac channel uuid */
        item = UMSG_SAC_ITEM_CHANNEL;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_chan_uuid_len);
	*(typeof(msg_p->um_chan_uuid_len) *)p = msg_p->um_chan_uuid_len;
	p += sub_len;
	len += sub_len;

	sub_len = msg_p->um_chan_uuid_len;
	memcpy(p, msg_p->um_chan_uuid, sub_len);
	len += sub_len;
	p += sub_len;

	/* sac sync */
        item = UMSG_SAC_ITEM_SYNC;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_sync);
	*(typeof(msg_p->um_sync) *)p = msg_p->um_sync;
	len += sub_len;
	p += sub_len;

	/* sac direct io */
        item = UMSG_SAC_ITEM_DIRECT_IO;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_direct_io);
	*(typeof(msg_p->um_direct_io) *)p = msg_p->um_direct_io;
	len += sub_len;
	p += sub_len;

	/* sac enable kill myself */
        item = UMSG_SAC_ITEM_ENABLE_KILL_MYSELF;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_enable_kill_myself);
	*(typeof(msg_p->um_enable_kill_myself) *)p = msg_p->um_enable_kill_myself;
	len += sub_len;
	p += sub_len;

	/* sac alignment */
        item = UMSG_SAC_ITEM_ALIGNMENT;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_alignment);
	*(typeof(msg_p->um_alignment) *)p = msg_p->um_alignment;
	len += sub_len;
	p += sub_len;

	/* sac ds name */
        item = UMSG_SAC_ITEM_DS;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_ds_name_len);
	*(typeof(msg_p->um_ds_name_len) *)p = msg_p->um_ds_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_ds_name_len;
	memcpy(p, msg_p->um_ds_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* sac dev name */
        item = UMSG_SAC_ITEM_DEV;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_dev_name_len);
	*(typeof(msg_p->um_dev_name_len) *)p = msg_p->um_dev_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_dev_name_len;
	memcpy(p, msg_p->um_dev_name, sub_len);
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_export_name_len);
	*(typeof(msg_p->um_export_name_len) *)p = msg_p->um_export_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_export_name_len;
	memcpy(p, msg_p->um_export_name, sub_len);
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_dev_size);
	*(typeof(msg_p->um_dev_size) *)p = msg_p->um_dev_size;
	len += sub_len;
	p += sub_len;

	/* sac tp name */
        item = UMSG_SAC_ITEM_TP;
        sub_len = sizeof(item);
        *(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_tp_name_len);
	*(typeof(msg_p->um_tp_name_len) *)p = msg_p->um_tp_name_len;
	len += sub_len;
	p += sub_len;

	sub_len = msg_p->um_tp_name_len;
	memcpy(p, msg_p->um_tp_name, sub_len);
	len += sub_len;
	p += sub_len;

	/* sac wc name */
        item = UMSG_SAC_ITEM_WC;
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

	/* sac rc name */
        item = UMSG_SAC_ITEM_RC;
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

	/* sac io type */
	item = UMSG_SAC_ITEM_IO_TYPE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_io_type);
	*(typeof(msg_p->um_io_type) *)p = msg_p->um_io_type;
	len += sub_len;
	p += sub_len;

	/* sac ds type */
	item = UMSG_SAC_ITEM_DS_TYPE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_ds_type);
	*(typeof(msg_p->um_ds_type) *)p = msg_p->um_ds_type;
	len += sub_len;
	p += sub_len;

	/* sac wc type */
	item = UMSG_SAC_ITEM_WC_TYPE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_wc_type);
	*(typeof(msg_p->um_wc_type) *)p = msg_p->um_wc_type;
	len += sub_len;
	p += sub_len;

	/* sac rc type */
	item = UMSG_SAC_ITEM_RC_TYPE;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_rc_type);
	*(typeof(msg_p->um_rc_type) *)p = msg_p->um_rc_type;
	len += sub_len;
	p += sub_len;

	/* sac ready */
	item = UMSG_SAC_ITEM_READY;
	sub_len = sizeof(item);
	*(typeof(item) *)p = item;
	len += sub_len;
	p += sub_len;

	sub_len = sizeof(msg_p->um_ready);
	*(typeof(msg_p->um_ready) *)p = msg_p->um_ready;
	len += sub_len;
	p += sub_len;

	*(typeof(msghdr_p->um_len) *)len_p = len;	// setup the length in out_buf
	msghdr_p->um_len = len;				// also setup the length in the input header
	*out_len = len;					// pass out the length

	return 0;
}

static int
__decode_sac_display_resp(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_display_resp";
	struct umessage_sac_display_resp *msg_p = (struct umessage_sac_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p;
	p += sizeof(int8_t);
	msghdr_p->um_req_code = *p;
	p += sizeof(int8_t);
	msghdr_p->um_len = len;
	len -= UMSG_SAC_HEADER_LEN;
	p += sizeof(int32_t);	// skip len filed

	while (len) {
		item = *p;
		p += sizeof(item);
		len -= sizeof(item);
		switch (item) {
		case UMSG_SAC_ITEM_CHANNEL:
			msg_p->um_chan_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_chan_uuid, p, msg_p->um_chan_uuid_len);
			msg_p->um_chan_uuid[msg_p->um_chan_uuid_len] = '\0';
			p += msg_p->um_chan_uuid_len;
			len -= msg_p->um_chan_uuid_len;
			break;

		case UMSG_SAC_ITEM_SYNC:
			msg_p->um_sync = *(char *)p;
			p += sizeof(char);
			len -= sizeof(char);
			break;

		case UMSG_SAC_ITEM_DIRECT_IO:
			msg_p->um_direct_io = *(char *)p;
			p += sizeof(char);
			len -= sizeof(char);
			break;

		case UMSG_SAC_ITEM_ENABLE_KILL_MYSELF:
			msg_p->um_enable_kill_myself = *(char *)p;
			p += sizeof(char);
			len -= sizeof(char);
			break;

		case UMSG_SAC_ITEM_ALIGNMENT:
			msg_p->um_alignment = *(int *)p;
			p += sizeof(int);
			len -= sizeof(int);
			break;

		case UMSG_SAC_ITEM_DS:
			msg_p->um_ds_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_ds_name, p, msg_p->um_ds_name_len);
			msg_p->um_ds_name[msg_p->um_ds_name_len] = '\0';
			p += msg_p->um_ds_name_len;
			len -= msg_p->um_ds_name_len;
			break;

		case UMSG_SAC_ITEM_DEV:
			msg_p->um_dev_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_dev_name, p, msg_p->um_dev_name_len);
			msg_p->um_dev_name[msg_p->um_dev_name_len] = '\0';
			p += msg_p->um_dev_name_len;
			len -= msg_p->um_dev_name_len;

			msg_p->um_export_name_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_export_name, p, msg_p->um_export_name_len);
			msg_p->um_export_name[msg_p->um_export_name_len] = '\0';
			p += msg_p->um_export_name_len;
			len -= msg_p->um_export_name_len;

			msg_p->um_dev_size = *(int32_t *)p;
			p += sizeof(int32_t);
			len -= sizeof(int32_t);
			break;

		case UMSG_SAC_ITEM_TP:
			msg_p->um_tp_name_len = *(int8_t *)p;
			p += sizeof(int8_t);
			len -= sizeof(int8_t);
			memcpy(msg_p->um_tp_name, p, msg_p->um_tp_name_len);
			msg_p->um_tp_name[msg_p->um_tp_name_len] = '\0';
			p += msg_p->um_tp_name_len;
			len -= msg_p->um_tp_name_len;
			break;

		case UMSG_SAC_ITEM_WC:
			msg_p->um_wc_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_wc_uuid, p, msg_p->um_wc_uuid_len);
			msg_p->um_wc_uuid[msg_p->um_wc_uuid_len] = '\0';
			p += msg_p->um_wc_uuid_len;
			len -= msg_p->um_wc_uuid_len;
			break;

		case UMSG_SAC_ITEM_RC:
			msg_p->um_rc_uuid_len = *(int16_t *)p;
			p += sizeof(int16_t);
			len -= sizeof(int16_t);
			memcpy(msg_p->um_rc_uuid, p, msg_p->um_rc_uuid_len);
			msg_p->um_rc_uuid[msg_p->um_rc_uuid_len] = '\0';
			p += msg_p->um_rc_uuid_len;
			len -= msg_p->um_rc_uuid_len;
			break;

		case UMSG_SAC_ITEM_IO_TYPE:
			msg_p->um_io_type = *(int *)p;
			p += sizeof(int);
			len -= sizeof(int);
			break;

		case UMSG_SAC_ITEM_DS_TYPE:
			msg_p->um_ds_type = *(int *)p;
			p += sizeof(int);
			len -= sizeof(int);
			break;

		case UMSG_SAC_ITEM_WC_TYPE:
			msg_p->um_wc_type = *(int *)p;
			p += sizeof(int);
			len -= sizeof(int);
			break;

		case UMSG_SAC_ITEM_RC_TYPE:
			msg_p->um_rc_type = *(int *)p;
			p += sizeof(int);
			len -= sizeof(int);
			break;

		case UMSG_SAC_ITEM_READY:
			msg_p->um_ready = *(uint8_t *)p;
			p += sizeof(uint8_t);
			len -= sizeof(uint8_t);
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
__encode_sac_hello(char *out_buf, uint32_t *out_len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__encode_sac_hello";
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
__decode_sac_hello(char *p, int len, struct umessage_sac_hdr *msghdr_p)
{
	char *log_header = "__decode_sac_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_SAC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_SAC_HEADER_LEN;

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
umpk_encode_sac(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "umpk_encode_sac";
	struct umessage_sac_hdr *msghdr_p = (struct umessage_sac_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_SAC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_STAT:
		case UMSG_SAC_CODE_STAT_ALL:
		case UMSG_SAC_CODE_LIST_STAT:
			rc = __encode_sac_information(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_STAT:
		case UMSG_SAC_CODE_RESP_STAT_ALL:
		case UMSG_SAC_CODE_STAT_END:
			rc = __encode_sac_information_resp(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_LIST_STAT:
			rc = __encode_sac_list_stat_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_ADD:
			rc = __encode_sac_add(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_ADD:
			rc = __encode_sac_add_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_DELETE:
			rc = __encode_sac_delete(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_DELETE:
			rc = __encode_sac_delete_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_SWITCH:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_SWITCH:
			rc = __encode_sac_switch(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_SWITCH:
			rc = __encode_sac_switch_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_SET_KEEPALIVE:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_SET_KEEPALIVE:
			rc = __encode_sac_set_keepalive(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_SET_KEEPALIVE:
			rc = __encode_sac_set_keepalive_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_FAST_RELEASE:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_START:
		case UMSG_SAC_CODE_STOP:
			rc = __encode_sac_fast_release(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_START:
		case UMSG_SAC_CODE_RESP_STOP:
			rc = __encode_sac_fast_release_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_IOINFO:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_IOINFO_START:
		case UMSG_SAC_CODE_IOINFO_START_ALL:
		case UMSG_SAC_CODE_IOINFO_STOP:
		case UMSG_SAC_CODE_IOINFO_STOP_ALL:
		case UMSG_SAC_CODE_IOINFO_CHECK:
		case UMSG_SAC_CODE_IOINFO_CHECK_ALL:
			rc = __encode_sac_ioinfo(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_IOINFO_START:
		case UMSG_SAC_CODE_RESP_IOINFO_START_ALL:
		case UMSG_SAC_CODE_RESP_IOINFO_STOP:
		case UMSG_SAC_CODE_RESP_IOINFO_STOP_ALL:
		case UMSG_SAC_CODE_RESP_IOINFO_CHECK:
		case UMSG_SAC_CODE_RESP_IOINFO_CHECK_ALL:
		case UMSG_SAC_CODE_RESP_IOINFO_END:
			rc = __encode_sac_ioinfo_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_S_DISP:
		case UMSG_SAC_CODE_S_DISP_ALL:
		case UMSG_SAC_CODE_W_DISP:
		case UMSG_SAC_CODE_W_DISP_ALL:
			rc = __encode_sac_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_SAC_CODE_S_RESP_DISP:
		case UMSG_SAC_CODE_S_RESP_DISP_ALL:
		case UMSG_SAC_CODE_W_RESP_DISP:
		case UMSG_SAC_CODE_W_RESP_DISP_ALL:
		case UMSG_SAC_CODE_RESP_END:
			rc = __encode_sac_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_HELLO:
		case UMSG_SAC_CODE_RESP_HELLO:
			rc = __encode_sac_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_sac(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_sac";
	struct umessage_sac_hdr *msghdr_p = (struct umessage_sac_hdr *)data;
	int rc = 0;

	switch (msghdr_p->um_req) {
	case UMSG_SAC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_STAT:
		case UMSG_SAC_CODE_STAT_ALL:
		case UMSG_SAC_CODE_LIST_STAT:
			rc = __decode_sac_information(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_STAT:
		case UMSG_SAC_CODE_RESP_STAT_ALL:
		case UMSG_SAC_CODE_RESP_END:
			rc = __decode_sac_information_resp(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_LIST_STAT:
			rc = __decode_sac_list_stat_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd (%d)", log_header, msghdr_p->um_req);
			break;
		}
		break;

	case UMSG_SAC_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_ADD:
			rc = __decode_sac_add(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_ADD:
			rc = __decode_sac_add_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_DELETE:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_DELETE:
			rc = __decode_sac_delete(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_DELETE:
			__decode_sac_delelte_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_SWITCH:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_SWITCH:
			rc = __decode_sac_switch(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_SWITCH:
			__decode_sac_switch_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_SET_KEEPALIVE:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_SET_KEEPALIVE:
			rc = __decode_sac_set_keepalive(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_SET_KEEPALIVE:
			__decode_sac_set_keepalive_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_FAST_RELEASE:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_START:
		case UMSG_SAC_CODE_STOP:
			rc = __decode_sac_fast_release(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_START:
		case UMSG_SAC_CODE_RESP_STOP:
			rc = __decode_sac_fast_release_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_IOINFO:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_IOINFO_START:
		case UMSG_SAC_CODE_IOINFO_START_ALL:
		case UMSG_SAC_CODE_IOINFO_STOP:
		case UMSG_SAC_CODE_IOINFO_STOP_ALL:
		case UMSG_SAC_CODE_IOINFO_CHECK:
		case UMSG_SAC_CODE_IOINFO_CHECK_ALL:
			rc = __decode_sac_ioinfo(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_RESP_IOINFO_START:
		case UMSG_SAC_CODE_RESP_IOINFO_START_ALL:
		case UMSG_SAC_CODE_RESP_IOINFO_STOP:
		case UMSG_SAC_CODE_RESP_IOINFO_STOP_ALL:
		case UMSG_SAC_CODE_RESP_IOINFO_CHECK:
		case UMSG_SAC_CODE_RESP_IOINFO_CHECK_ALL:
		case UMSG_SAC_CODE_RESP_IOINFO_END:
			rc = __decode_sac_ioinfo_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_S_DISP:
		case UMSG_SAC_CODE_S_DISP_ALL:
		case UMSG_SAC_CODE_W_DISP:
		case UMSG_SAC_CODE_W_DISP_ALL:
			rc = __decode_sac_display(p, len, msghdr_p);
			break;

		case UMSG_SAC_CODE_S_RESP_DISP:
		case UMSG_SAC_CODE_S_RESP_DISP_ALL:
		case UMSG_SAC_CODE_W_RESP_DISP:
		case UMSG_SAC_CODE_W_RESP_DISP_ALL:
		case UMSG_SAC_CODE_RESP_END:
			rc = __decode_sac_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_SAC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_SAC_CODE_HELLO:
		case UMSG_SAC_CODE_RESP_HELLO:
			rc = __decode_sac_hello(p, len, msghdr_p);
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
