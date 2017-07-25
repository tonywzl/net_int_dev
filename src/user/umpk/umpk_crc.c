/*
 * umpk_rc.c
 * 	umpk for write cache
 */

#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_crc_if.h"
static int
__encode_crc_sp_heads_size_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_sp_heads_size_result";
  	assert( out_buf && out_len && msghdr_p );
	struct umessage_crc_information_resp_sp_heads_size *msg_p = (struct umessage_crc_information_resp_sp_heads_size*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;
	int i;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	*p++ = UMSG_CRC_ITEM_SP_HEADS_SIZE;
	len++;
	uint32_t size;
	for (i=0; i<DSMGR_ALL_SP_HEADS; i++) {
		size = msg_p->um_sp_heads_size[i];
		*(uint32_t *)p = size;
		p += 4;
		len += 4;
	}

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

  	return 0;
}

static int
__encode_crc_cse_hit_check_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_cse_hit_check_result";
  	assert( out_buf && out_len && msghdr_p );
	struct umessage_crc_information_resp_cse_hit *msg_p = (struct umessage_crc_information_resp_cse_hit*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	*p++ = UMSG_CRC_ITEM_CSE_HIT;
	len++;
	uint64_t hit_counter = msg_p->um_rc_hit_counter;
	*(uint64_t *)p = hit_counter;
	p += 8;
	len += 8;

	*p++ = UMSG_CRC_ITEM_CSE_UNHIT;
	len++;
	uint64_t unhit_counter = msg_p->um_rc_unhit_counter;
	*(uint64_t *)p = unhit_counter;
	p += 8;
	len += 8;

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

  	return 0;
}

static int
__encode_crc_freespace_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_freespace_result";
  	assert( out_buf && out_len && msghdr_p );
	struct umessage_crc_information_resp_freespace *msg_p = (struct umessage_crc_information_resp_freespace*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	*p++ = UMSG_CRC_ITEM_FREE_SPACE;
	len++;
	uint64_t free_space = msg_p->um_num_blocks_free;
	*(uint64_t *)p = free_space;
	p += 8;
	len += 8;

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

  	return 0;
}

static int
__encode_crc_check_fp_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_check_fp_result";
  	assert( out_buf && out_len && msghdr_p );
	struct umessage_crc_information_resp_check_fp *msg_p = (struct umessage_crc_information_resp_check_fp*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	*p++ = UMSG_CRC_ITEM_CHECK_FP_RC;
	len++;
	unsigned char check_fp_rc = msg_p->um_check_fp_rc;
	*(unsigned char *)p = check_fp_rc;
	p ++;
	len ++;

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

  	return 0;
}

static int
__encode_crc_dsbmp_bsn_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_dsbmp_bsn__result";
  	assert( out_buf && out_len && msghdr_p );
	struct umessage_crc_information_resp_dsbmp_bsn *msg_p = (struct umessage_crc_information_resp_dsbmp_bsn*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	*p++ = UMSG_CRC_ITEM_BSN_OFFSET;
	len++;
	uint64_t offset = msg_p->um_offset;
	*(uint64_t *)p = offset;
	p += 8;
	len += 8;

	*p++ = UMSG_CRC_ITEM_BSN_SIZE;
	len++;
	uint64_t size = msg_p->um_size;
	*(uint64_t *)p = size;
	p += 8;
	len += 8;

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

  	return 0;
}

static int
__encode_crc_dsrec_stat_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_dsrec_stat_result";
  	assert( out_buf && out_len && msghdr_p );
	struct umessage_crc_information_resp_dsrec_stat *msg_p = (struct umessage_crc_information_resp_dsrec_stat*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;
	int i;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	*p++ = UMSG_CRC_ITEM_DSREC_STAT;
	len++;
	for (i=0; i<NID_SIZE_FP_WGT; i++) {
		*(uint64_t *)p = msg_p->um_dsrec_stat.p_lru_nr[i];
		p +=8;
		len +=8;
	}
	*(uint64_t *)p = msg_p->um_dsrec_stat.p_lru_total_nr;
	p += 8;
	len += 8;
	*(uint64_t *)p = msg_p->um_dsrec_stat.p_lru_max;
	p += 8;
	len += 8;
	*(uint64_t *)p= msg_p->um_dsrec_stat.p_rec_nr;
	p += 8;
	len += 8;

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

  	return 0;
}

static int
__encode_crc_dsbmp_rt_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_freespace_result";
  	assert( out_buf && out_len && msghdr_p );
	//	struct umessage_crc_information_resp_dsbmp_rtree_node *msg_p = (struct umessage_crc_information_resp_dsbmp_rtree_node*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

  	return 0;
}

static int
__encode_crc_dsbmp_rt_end_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_dsbmp_rt_end_result";
  	assert( out_buf && out_len && msghdr_p );
	//struct umessage_crc_information_resp_dsbmp_rtree_node_finish *msg_p = (struct umessage_crc_information_resp_dsbmp_rtree_node_finish*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;

  	return 0;
}

static int
__decode_crc_freespace_result(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
  	assert( p && len && msghdr_p );
	char *log_header = "__decode_crc_freespace_result";

	struct umessage_crc_information_resp_freespace *res_p = (struct umessage_crc_information_resp_freespace *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_FREE_SPACE:
			res_p->um_num_blocks_free = *(uint64_t *)p;
			p += 8;
			len -= 8;
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
__decode_crc_sp_heads_size_result(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
  	assert( p && len && msghdr_p );
	char *log_header = "__decode_crc_sp_heads_size_result";

	struct umessage_crc_information_resp_sp_heads_size *res_p = (struct umessage_crc_information_resp_sp_heads_size *)msghdr_p;
	int8_t item;
	int rc = 0;
	int i;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_SP_HEADS_SIZE:
			for (i=0; i<DSMGR_ALL_SP_HEADS; i++) {
				res_p->um_sp_heads_size[i] = *(uint32_t *)p;
				p += 4;
				len -= 4;
			}
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
__decode_crc_cse_hit_check_result(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
  	assert( p && len && msghdr_p );
	char *log_header = "__decode_crc_cse_hit_check_result";

	struct umessage_crc_information_resp_cse_hit *res_p = (struct umessage_crc_information_resp_cse_hit *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CSE_HIT:
			res_p->um_rc_hit_counter = *(uint64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_CRC_ITEM_CSE_UNHIT:
			res_p->um_rc_unhit_counter = *(uint64_t *)p;
			p += 8;
			len -= 8;
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
__decode_crc_check_fp_result(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
  	assert( p && len && msghdr_p );
	char *log_header = "__decode_crc_check_fp_result";

	struct umessage_crc_information_resp_check_fp *res_p = (struct umessage_crc_information_resp_check_fp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CHECK_FP_RC:
			res_p->um_check_fp_rc = *(unsigned char *)p;
			p ++;
			len --;
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
__decode_crc_dsbmp_bsn_result(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
  	assert( p && len && msghdr_p );
	char *log_header = "__decode_crc_bsn_resultt";

	struct umessage_crc_information_resp_dsbmp_bsn  *res_p = (struct umessage_crc_information_resp_dsbmp_bsn  *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	item = *p++;
	if ( item == UMSG_CRC_ITEM_BSN_OFFSET ) {
		res_p->um_offset = *(uint64_t *)p;
		p += 8;
		len -= 8;
        	item = *p++;
		//		uint64_t size = 0;
		if ( UMSG_CRC_ITEM_BSN_SIZE == item ) {
			res_p->um_size = *(uint64_t *)p;
			p += 8;
			len -= 8;
		} else {
	  		nid_log_error("%s: got wrong item (%d) - expecting NUM_SIZES", log_header, item);
			rc = -1;
			goto out;
		}
	}
	nid_log_debug("%s:  offset = %lu, size = %lu", log_header, res_p->um_offset, res_p->um_size );

out:
  	return rc;
}


static int
__decode_crc_dsrec_stat_result(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
  	assert( p && len && msghdr_p );
	char *log_header = "__decode_crc_dsrec_stat_result";

	struct umessage_crc_information_resp_dsrec_stat  *res_p = (struct umessage_crc_information_resp_dsrec_stat  *)msghdr_p;
	int8_t item;
	int rc = 0, i;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	item = *p++;
	if ( item == UMSG_CRC_ITEM_DSREC_STAT ) {
		for (i=0; i<NID_SIZE_FP_WGT; i++) {
			res_p->um_dsrec_stat.p_lru_nr[i] = *(uint64_t *)p;
			p += 8;
			len -= 8;
		}
		res_p->um_dsrec_stat.p_lru_total_nr = *(uint64_t *)p;
		p += 8;
		len -= 8;
		res_p->um_dsrec_stat.p_lru_max = *(uint64_t *)p;
		p += 8;
		len -= 8;
		res_p->um_dsrec_stat.p_rec_nr = *(uint64_t *)p;
		p += 8;
		len -= 8;
	}
	else {
		nid_log_error("%s: got wrong item (%d) - expecting DSREC_STAT", log_header, item);
		rc = -1;
	}

  	return rc;
}

static int
__encode_crc_freespace_dist_result(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
  	assert( out_buf && out_len && msghdr_p );
	char *log_header = "__encode_crc_freespace_dist_result";
  	assert( out_buf && out_len && msghdr_p );
	struct umessage_crc_information_resp_freespace_dist *msg_p = (struct umessage_crc_information_resp_freespace_dist*)msghdr_p;
	char* p = out_buf;
	uint32_t len = 0;

	nid_log_warning( "starting...%s", log_header );

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
	p += 4;
	len += 4;

	*p++ = UMSG_CRC_ITEM_NUM_SIZES;
	len++;
	uint64_t num_sizes = msg_p->um_num_sizes;
	*(uint64_t *)p = (uint64_t)num_sizes;
	p += 8;
	len += 8;
	struct space_size_number* np;
	list_for_each_entry( np, struct space_size_number, &msg_p->um_size_list_head, um_ss_list ) {
	  	*p++ = UMSG_CRC_ITEM_SIZE;
		uint64_t space_size = np->um_size;
		*(uint64_t*)p = space_size;
		p += 8;
		len += 8;

		*p++ = UMSG_CRC_ITEM_SIZE_NUMBER;
		uint64_t size_number = np->um_number;
		*(uint64_t*)p = size_number;
		p += 8;
		len += 8;
	}

	p = out_buf + 2;
	*(uint32_t* )p = len;
	msghdr_p->um_len = len;
	*out_len = len;


  	return 0;
}

static int
__decode_crc_freespace_dist_result(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
  	assert( p && len && msghdr_p );
	char *log_header = "__decode_crc_freespace_dist_result";

	struct umessage_crc_information_resp_freespace_dist *res_p = (struct umessage_crc_information_resp_freespace_dist *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed
	len -= 4;
	// struct list_head* space_list_head = x_calloc( 1, sizeof( struct list_head ) );
        item = *p++;
	if ( UMSG_CRC_ITEM_NUM_SIZES == item ) {
	  	res_p->um_num_sizes = *((uint64_t*)p);
	  	nid_log_debug("%s:  NUM_SIZES = %d", log_header, (int)(res_p->um_num_sizes));
		p += 8;
		len -= 8;
	} else {
	  	nid_log_error("%s: got wrong item (%d) - expecting NUM_SIZES", log_header, item);
		rc = -1;
		goto out;
	}
	int num_entries = 0;
	while (len > 0) {
		len -= 1;
		item = *p;
		INIT_LIST_HEAD( &res_p->um_size_list_head );
		switch (*p++) {
		case UMSG_CRC_ITEM_SIZE:
		  	num_entries ++;
			uint64_t size_val = *(uint64_t *)p;
			p += 8;
			len -= 8;
        		item = *p++;
			uint64_t size_number = 0;
			if ( UMSG_CRC_ITEM_SIZE_NUMBER == item ) {
				size_number = *(uint64_t *)p;
				p += 8;
				len -= 8;
			} else {
	  			nid_log_error("%s: got wrong item (%d) - expecting NUM_SIZES", log_header, item);
				rc = -1;
				goto out;
			}
	  		nid_log_debug("%s:  size_val = %lu, size_num = %lu", log_header, size_val, size_number);
			struct space_size_number* ssn = x_calloc( 1, sizeof( struct space_size_number ) );
			ssn->um_size = size_val;
			ssn->um_number = size_number;
			list_add_tail( &(ssn->um_ss_list), &res_p->um_size_list_head );
			nid_log_error("%s: adding new space_size_node %lu, len left = %d", log_header, (unsigned long)ssn, (int)len);
			break;

		default:
			nid_log_error("%s: got wrong item (%d)", log_header, item);
			rc = -1;
			goto out;
		}
	}
	res_p->um_num_sizes = num_entries;

out:
  	return rc;
}



static int
__encode_crc_information(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_information";
	struct umessage_crc_information *msg_p = (struct umessage_crc_information *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: rc_uuid_len:%d", log_header, msg_p->um_rc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	if (msg_p->um_rc_uuid_len) {
		*p++ = UMSG_CRC_ITEM_CACHE;
		len++;
		sub_len = msg_p->um_rc_uuid_len;
		*(uint16_t *)p = (uint16_t)sub_len;
		p += 2;
		len += 2;
		memcpy(p, msg_p->um_rc_uuid, sub_len);
		p += sub_len;
		len += sub_len;
	}

	if (msg_p->um_chan_uuid_len) {
		*p++ = UMSG_CRC_ITEM_CHANNEL;
		len++;
		sub_len = msg_p->um_chan_uuid_len;
		*(uint16_t *)p = (uint16_t)sub_len;
		p += 2;
		len += 2;
		memcpy(p, msg_p->um_chan_uuid, sub_len);
		p += sub_len;
		len += sub_len;
	}

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_crc_information_resp_nse_stat(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_information_resp_nse_counter";
	struct umessage_crc_information_resp_nse_stat *msg_p
		= (struct umessage_crc_information_resp_nse_stat *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: rc_uuid_len:%d", log_header, msg_p->um_rc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	if (msg_p->um_rc_uuid_len) {
		*p++ = UMSG_CRC_ITEM_CACHE;
		len++;
		sub_len = msg_p->um_rc_uuid_len;
		*(uint16_t *)p = (uint16_t)sub_len;
		p += 2;
		len += 2;
		memcpy(p, msg_p->um_rc_uuid, sub_len);
		p += sub_len;
		len += sub_len;
	}

	if (msg_p->um_chan_uuid_len) {
		*p++ = UMSG_CRC_ITEM_CHANNEL;
		len++;
		sub_len = msg_p->um_chan_uuid_len;
		*(uint16_t *)p = (uint16_t)sub_len;
		p += 2;
		len += 2;
		memcpy(p, msg_p->um_chan_uuid, sub_len);
		p += sub_len;
		len += sub_len;
	}


	*p++ = UMSG_CRC_ITEM_NSE_STAT;
	len++;
	*(uint64_t *)p = msg_p->um_nse_stat.fp_max_num;
	p += 8;
	len += 8;
	*(uint64_t *)p = msg_p->um_nse_stat.fp_min_num;
	p += 8;
	len += 8;
	*(uint64_t *)p = msg_p->um_nse_stat.fp_num;
	p += 8;
	len += 8;
	*(uint64_t *)p = msg_p->um_nse_stat.fp_rec_num;
	p += 8;
	len += 8;
	*(uint64_t *)p = msg_p->um_nse_stat.hit_num;
	p += 8;
	len += 8;
	*(uint64_t *)p = msg_p->um_nse_stat.unhit_num;
	p += 8;
	len += 8;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__encode_crc_information_resp_nse_detail(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	struct umessage_crc_information_resp_nse_detail *msg_p
		= (struct umessage_crc_information_resp_nse_detail *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_CRC_ITEM_FLAG;
	len++;
	*(uint64_t *)p = msg_p->um_flags;
	p += 8;
	len += 8;

	if (msg_p->um_flags & UMSG_CRC_FLAG_START) {
		if (msg_p->um_rc_uuid_len) {
			*p++ = UMSG_CRC_ITEM_CACHE;
			len++;
			sub_len = msg_p->um_rc_uuid_len;
			*(uint16_t *)p = (uint16_t)sub_len;
			p += 2;
			len += 2;
			memcpy(p, msg_p->um_rc_uuid, sub_len);
			p += sub_len;
			len += sub_len;
		}

		if (msg_p->um_chan_uuid_len) {
			*p++ = UMSG_CRC_ITEM_CHANNEL;
			len++;
			sub_len = msg_p->um_chan_uuid_len;
			*(uint16_t *)p = (uint16_t)sub_len;
			p += 2;
			len += 2;
			memcpy(p, msg_p->um_chan_uuid, sub_len);
			p += sub_len;
			len += sub_len;
		}

		*p++ = UMSG_CRC_ITEM_NSE_STAT;
		len++;
		*(uint64_t *)p = msg_p->um_nse_stat.fp_max_num;
		p += 8;
		len += 8;
		*(uint64_t *)p = msg_p->um_nse_stat.fp_min_num;
		p += 8;
		len += 8;
		*(uint64_t *)p = msg_p->um_nse_stat.fp_num;
		p += 8;
		len += 8;
		*(uint64_t *)p = msg_p->um_nse_stat.fp_rec_num;
		p += 8;
		len += 8;
		*(uint64_t *)p = msg_p->um_nse_stat.hit_num;
		p += 8;
		len += 8;
		*(uint64_t *)p = msg_p->um_nse_stat.unhit_num;
		p += 8;
		len += 8;

	} else if (msg_p->um_flags & UMSG_CRC_FLAG_END) {
		*p++ = UMSG_CRC_ITEM_FLAG;
		len++;
		*(uint64_t *)p = msg_p->um_flags;
		p += 8;
		len += 8;

	} else {
		*p++ = UMSG_CRC_ITEM_BLOCK_INDEX;
		len++;
		*(uint64_t *)p = msg_p->um_block_index;
		p += 8;
		len += 8;

		*p++ = UMSG_CRC_ITEM_FP;
		len++;
		memcpy(p, msg_p->um_fp, NID_SIZE_FP);
		p += NID_SIZE_FP;
		len += NID_SIZE_FP;
	}

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_crc_information(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_information";
	struct umessage_crc_information *res_p = (struct umessage_crc_information *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
			break;

		case UMSG_CRC_ITEM_CHANNEL:
			res_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_chan_uuid, p, res_p->um_chan_uuid_len);
			res_p->um_chan_uuid[res_p->um_chan_uuid_len] = 0;
			p += res_p->um_chan_uuid_len;
			len -= res_p->um_chan_uuid_len;
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
__decode_crc_information_resp_nse_stat(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_information_resp_nes_stat";
	struct umessage_crc_information_resp_nse_stat *res_p =
		(struct umessage_crc_information_resp_nse_stat *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
			break;

		case UMSG_CRC_ITEM_CHANNEL:
			res_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_chan_uuid, p, res_p->um_chan_uuid_len);
			res_p->um_chan_uuid[res_p->um_chan_uuid_len] = 0;
			p += res_p->um_chan_uuid_len;
			len -= res_p->um_chan_uuid_len;
			break;

		case UMSG_CRC_ITEM_NSE_STAT:
			res_p->um_nse_stat.fp_max_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.fp_min_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.fp_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.fp_rec_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.hit_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.unhit_num = *(int64_t *)p;
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
__decode_crc_information_resp_nes_detail(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_information_resp_nes_detail";
	struct umessage_crc_information_resp_nse_detail *res_p =
		(struct umessage_crc_information_resp_nse_detail*)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_BLOCK_INDEX:
			res_p->um_block_index = *(uint64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_CRC_ITEM_FP:
			memcpy(res_p->um_fp, p, NID_SIZE_FP);
			p += NID_SIZE_FP;
			len -= NID_SIZE_FP;
			break;

		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
			break;

		case UMSG_CRC_ITEM_CHANNEL:
			res_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_chan_uuid, p, res_p->um_chan_uuid_len);
			res_p->um_chan_uuid[res_p->um_chan_uuid_len] = 0;
			p += res_p->um_chan_uuid_len;
			len -= res_p->um_chan_uuid_len;
			break;

		case UMSG_CRC_ITEM_NSE_STAT:
			res_p->um_nse_stat.fp_max_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.fp_min_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.fp_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.fp_rec_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.hit_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			res_p->um_nse_stat.unhit_num = *(int64_t *)p;
			p += 8;
			len -= 8;
			break;

		case UMSG_CRC_ITEM_FLAG:
			res_p->um_flags =  *(uint64_t *)p;
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
__encode_crc_dropcache(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_dropcache";
	struct umessage_crc_dropcache *msg_p = (struct umessage_crc_dropcache *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: chan_uuid_len:%d", log_header, msg_p->um_chan_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_CRC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_rc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_CHANNEL;
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
__decode_crc_dropcache(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_dropcache";
	struct umessage_crc_dropcache *res_p = (struct umessage_crc_dropcache *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
			break;

		case UMSG_CRC_ITEM_CHANNEL:
			res_p->um_chan_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_chan_uuid, p, res_p->um_chan_uuid_len);
			res_p->um_chan_uuid[res_p->um_chan_uuid_len] = 0;
			p += res_p->um_chan_uuid_len;
			len -= res_p->um_chan_uuid_len;
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
__encode_crc_setwgt(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_setwgt";
	struct umessage_crc_setwgt *msg_p = (struct umessage_crc_setwgt *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: um_rc_uuid_len:%d", log_header, msg_p->um_rc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_CRC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_rc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_SET_WGT;
	len++;
	*(uint8_t *)p = (uint8_t)msg_p->um_wgt;
	p++;
	len++;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_crc_setwgt(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_setwgt";
	struct umessage_crc_setwgt *res_p = (struct umessage_crc_setwgt *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
			break;

		case UMSG_CRC_ITEM_SET_WGT:
			res_p->um_wgt = *(int8_t *)p;
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
__encode_crc_add(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_add";
	struct umessage_crc_add *msg_p = (struct umessage_crc_add *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: um_rc_uuid_len:%d", log_header, msg_p->um_rc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_CRC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_rc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_PP;
	len++;
	sub_len = msg_p->um_pp_name_len;
	*(uint8_t *)p = (uint8_t)sub_len;
	p++;
	len++;
	memcpy(p, msg_p->um_pp_name, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_DEVICE;
	len++;
	sub_len = msg_p->um_cache_device_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_cache_device, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_SIZE;
	len++;
	*(uint32_t *)p = (uint32_t)msg_p->um_cache_size;
	p += 4;
	len += 4;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_crc_add(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_add";
	struct umessage_crc_add *res_p = (struct umessage_crc_add *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
			break;

		case UMSG_CRC_ITEM_PP:
			res_p->um_pp_name_len = *(int8_t *)p;
			p ++;
			len --;
			memcpy(res_p->um_pp_name, p, res_p->um_pp_name_len);
			res_p->um_pp_name[res_p->um_pp_name_len] = 0;
			p += res_p->um_pp_name_len;
			len -= res_p->um_pp_name_len;
			break;

		case UMSG_CRC_ITEM_DEVICE:
			res_p->um_cache_device_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_cache_device, p, res_p->um_cache_device_len);
			res_p->um_cache_device[res_p->um_cache_device_len] = 0;
			p += res_p->um_cache_device_len;
			len -= res_p->um_cache_device_len;
			break;

		case UMSG_CRC_ITEM_SIZE:
			res_p->um_cache_size = *(int32_t *)p;
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
__encode_crc_add_resp(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_add_resp";
	struct umessage_crc_add_resp *msg_p = (struct umessage_crc_add_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: um_rc_uuid_len:%d", log_header, msg_p->um_rc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_CRC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_rc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_ADD_RSLT;
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
__decode_crc_add_resp(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_add_resp";
	struct umessage_crc_add_resp *res_p = (struct umessage_crc_add_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
			break;

		case UMSG_CRC_ITEM_ADD_RSLT:
			res_p->um_resp_code = *(int8_t *)p;
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
__encode_crc_display(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_display";
	struct umessage_crc_display *msg_p = (struct umessage_crc_display *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: rc_uuid_len:%d", log_header, msg_p->um_rc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_CRC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_rc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_crc_display(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_display";
	struct umessage_crc_display *res_p = (struct umessage_crc_display *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
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
__encode_crc_display_resp(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_display_resp";
	struct umessage_crc_display_resp *msg_p = (struct umessage_crc_display_resp *)msghdr_p;
	char *p = out_buf;
	uint32_t len = 0, sub_len;

	nid_log_warning("%s: um_rc_uuid_len:%d", log_header, msg_p->um_rc_uuid_len);
	*p++ = msghdr_p->um_req;
	len++;
	*p++ = msghdr_p->um_req_code;
	len++;
        p += 4; // skip the length field
	len += 4;

	*p++ = UMSG_CRC_ITEM_CACHE;
	len++;
	sub_len = msg_p->um_rc_uuid_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_rc_uuid, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_PP;
	len++;
	sub_len = msg_p->um_pp_name_len;
	*(uint8_t *)p = (uint8_t)sub_len;
	p++;
	len++;
	memcpy(p, msg_p->um_pp_name, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_DEVICE;
	len++;
	sub_len = msg_p->um_cache_device_len;
	*(uint16_t *)p = (uint16_t)sub_len;
	p += 2;
	len += 2;
	memcpy(p, msg_p->um_cache_device, sub_len);
	p += sub_len;
	len += sub_len;

	*p++ = UMSG_CRC_ITEM_SIZE;
	len++;
	*(uint32_t *)p = (uint32_t)msg_p->um_cache_size;
	p += 4;
	len += 4;

	*p++ = UMSG_CRC_ITEM_BLOCK_SIZE;
	len++;
	*(uint64_t *)p = (uint32_t)msg_p->um_block_size;
	p += 8;
	len += 8;

	p = out_buf + 2;	// go back to the length field
	*(uint32_t *)p = len;	// setup the length
	msghdr_p->um_len = len;	// also setup the length in the input header
	*out_len = len;

	return 0;
}

static int
__decode_crc_display_resp(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_display_resp";
	struct umessage_crc_display_resp *res_p = (struct umessage_crc_display_resp *)msghdr_p;
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	len -= UMSG_CRC_HEADER_LEN;
	p += 4;	// skip len filed

	while (len) {
		len -= 1;
		item = *p;
		switch (*p++) {
		case UMSG_CRC_ITEM_CACHE:
			res_p->um_rc_uuid_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_rc_uuid, p, res_p->um_rc_uuid_len);
			res_p->um_rc_uuid[res_p->um_rc_uuid_len] = 0;
			p += res_p->um_rc_uuid_len;
			len -= res_p->um_rc_uuid_len;
			break;

		case UMSG_CRC_ITEM_PP:
			res_p->um_pp_name_len = *(int8_t *)p;
			p ++;
			len --;
			memcpy(res_p->um_pp_name, p, res_p->um_pp_name_len);
			res_p->um_pp_name[res_p->um_pp_name_len] = 0;
			p += res_p->um_pp_name_len;
			len -= res_p->um_pp_name_len;
			break;

		case UMSG_CRC_ITEM_DEVICE:
			res_p->um_cache_device_len = *(int16_t *)p;
			p += 2;
			len -= 2;
			memcpy(res_p->um_cache_device, p, res_p->um_cache_device_len);
			res_p->um_cache_device[res_p->um_cache_device_len] = 0;
			p += res_p->um_cache_device_len;
			len -= res_p->um_cache_device_len;
			break;

		case UMSG_CRC_ITEM_SIZE:
			res_p->um_cache_size = *(int32_t *)p;
			p += 4;
			len -= 4;
			break;

		case UMSG_CRC_ITEM_BLOCK_SIZE:
			res_p->um_block_size = *(int64_t *)p;
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
__encode_crc_hello(char *out_buf, uint32_t *out_len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__encode_crc_hello";
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
__decode_crc_hello(char *p, int len, struct umessage_crc_hdr *msghdr_p)
{
	char *log_header = "__decode_crc_hello";
	int8_t item;
	int rc = 0;

	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = len;
	p += 4;
	len -= UMSG_CRC_HEADER_LEN;

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
umpk_encode_crc(char *out_buf, uint32_t *out_len, void *data)
{
	char *log_header = "encode_type_crc";
	struct umessage_crc_hdr *msghdr_p = (struct umessage_crc_hdr *)data;
	int rc;

	switch (msghdr_p->um_req) {
	case UMSG_CRC_CMD_DROPCACHE:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_START:
		case UMSG_CRC_CODE_START_SYNC:
		case UMSG_CRC_CODE_STOP:
			rc = __encode_crc_dropcache(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_FREE_SPACE:
		case UMSG_CRC_CODE_FREE_SPACE_DIST:
		case UMSG_CRC_CODE_NSE_STAT:
		case UMSG_CRC_CODE_NSE_DETAIL:
		case UMSG_CRC_CODE_DSBMP_RTREE:
		case UMSG_CRC_CODE_CSE_HIT_CHECK:
		case UMSG_CRC_CODE_CHECK_FP:
		case UMSG_CRC_CODE_SP_HEADS_SIZE:
			rc = __encode_crc_information(out_buf, out_len, msghdr_p);
			break;

		case UMSG_CRC_CODE_SP_HEADS_SIZE_RSLT:
			rc = __encode_crc_sp_heads_size_result(out_buf, out_len, msghdr_p);
			break;

		case UMSG_CRC_CODE_CSE_HIT_CHECK_RSLT:
			rc = __encode_crc_cse_hit_check_result( out_buf, out_len, msghdr_p );
			break;

		case UMSG_CRC_CODE_FREE_SPACE_RSLT:
			rc = __encode_crc_freespace_result( out_buf, out_len, msghdr_p );
			break;

		case UMSG_CRC_CODE_CHECK_FP_RSLT:
			rc = __encode_crc_check_fp_result( out_buf, out_len, msghdr_p );
			break;

		case UMSG_CRC_CODE_FREE_SPACE_DIST_RSLT:
			rc = __encode_crc_freespace_dist_result( out_buf, out_len, msghdr_p );
			break;

		case UMSG_CRC_CODE_RESP_NSE_STAT:
			rc = __encode_crc_information_resp_nse_stat(out_buf, out_len, msghdr_p);
			break;

		case UMSG_CRC_CODE_RESP_NSE_DETAIL:
			rc = __encode_crc_information_resp_nse_detail(out_buf, out_len, msghdr_p);
			break;

		case UMSG_CRC_CODE_DSBMP_RTREE_RSLT:
			rc = __encode_crc_dsbmp_rt_result( out_buf, out_len, msghdr_p );
			break;
		case UMSG_CRC_CODE_DSBMP_RTREE_END_RSLT:
			rc = __encode_crc_dsbmp_rt_end_result( out_buf, out_len, msghdr_p );
			break;
		case UMSG_CRC_CODE_BSN_RSLT:
			rc = __encode_crc_dsbmp_bsn_result( out_buf, out_len, msghdr_p );
			break;
		case UMSG_CRC_CODE_DSREC_STAT:
			rc = __encode_crc_dsrec_stat_result( out_buf, out_len, msghdr_p );
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_SETWGT:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_WGT:
			rc = __encode_crc_setwgt(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_ADD:
			rc = __encode_crc_add(out_buf, out_len, msghdr_p);
			break;

		case UMSG_CRC_CODE_RESP_ADD:
			rc = __encode_crc_add_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_S_DISP:
		case UMSG_CRC_CODE_S_DISP_ALL:
		case UMSG_CRC_CODE_W_DISP:
		case UMSG_CRC_CODE_W_DISP_ALL:
			rc = __encode_crc_display(out_buf, out_len, msghdr_p);
			break;

		case UMSG_CRC_CODE_S_RESP_DISP:
		case UMSG_CRC_CODE_S_RESP_DISP_ALL:
		case UMSG_CRC_CODE_W_RESP_DISP:
		case UMSG_CRC_CODE_W_RESP_DISP_ALL:
		case UMSG_CRC_CODE_DISP_END:
			rc = __encode_crc_display_resp(out_buf, out_len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd cdoe (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_HELLO:
		case UMSG_CRC_CODE_RESP_HELLO:
			rc = __encode_crc_hello(out_buf, out_len, msghdr_p);
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
umpk_decode_crc(char *p, uint32_t len, void *data)
{
	char *log_header = "umpk_decode_crc";
	struct umessage_crc_hdr *msghdr_p = (struct umessage_crc_hdr *)data;
	int rc = 0;

	char* header_index = p;
	assert(len >= UMSG_CRC_HEADER_LEN);
	msghdr_p->um_req = *header_index++;
	msghdr_p->um_req_code = *header_index++;

	switch (msghdr_p->um_req) {
	case UMSG_CRC_CMD_DROPCACHE:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_START:
		case UMSG_CRC_CODE_START_SYNC:
		case UMSG_CRC_CODE_STOP:
			rc = __decode_crc_dropcache(p, len, msghdr_p);
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_INFORMATION:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_FREE_SPACE:
		case UMSG_CRC_CODE_FREE_SPACE_DIST:
		case UMSG_CRC_CODE_DSBMP_RTREE:
		case UMSG_CRC_CODE_NSE_STAT:
		case UMSG_CRC_CODE_NSE_DETAIL:
		case UMSG_CRC_CODE_CSE_HIT_CHECK:
		case UMSG_CRC_CODE_CHECK_FP:
		case UMSG_CRC_CODE_SP_HEADS_SIZE:
			rc = __decode_crc_information(p, len, msghdr_p);
			break;

		case UMSG_CRC_CODE_FREE_SPACE_RSLT:
		  	rc = __decode_crc_freespace_result(p, len, msghdr_p );
		  	break;

		case UMSG_CRC_CODE_SP_HEADS_SIZE_RSLT:
		  	rc = __decode_crc_sp_heads_size_result(p, len, msghdr_p );
		  	break;

		case UMSG_CRC_CODE_CSE_HIT_CHECK_RSLT:
			rc = __decode_crc_cse_hit_check_result( p, len, msghdr_p );
			break;

		case UMSG_CRC_CODE_CHECK_FP_RSLT:
		  	rc = __decode_crc_check_fp_result(p, len, msghdr_p );
		  	break;

		case UMSG_CRC_CODE_FREE_SPACE_DIST_RSLT:
		  	rc = __decode_crc_freespace_dist_result(p, len, msghdr_p);
		  	break;

		case UMSG_CRC_CODE_RESP_NSE_STAT:
			rc = __decode_crc_information_resp_nse_stat(p, len, msghdr_p);
			break;

		case UMSG_CRC_CODE_RESP_NSE_DETAIL:
			rc = __decode_crc_information_resp_nes_detail(p, len, msghdr_p);
			break;

		case UMSG_CRC_CODE_DSBMP_RTREE_RSLT:
		case UMSG_CRC_CODE_DSBMP_RTREE_END_RSLT:
		  	// no need to decode these messages as they are markers
		  	// the req and req_code are already decoded so it is fine
		  	rc = 0;
		  	break;
		case UMSG_CRC_CODE_BSN_RSLT:
		  	rc = __decode_crc_dsbmp_bsn_result( p, len, msghdr_p );
		  	break;
		case UMSG_CRC_CODE_DSREC_STAT:
		  	rc = __decode_crc_dsrec_stat_result( p, len, msghdr_p );
		  	break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_SETWGT:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_WGT:
			rc = __decode_crc_setwgt(p, len, msghdr_p);
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_ADD:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_ADD:
			rc = __decode_crc_add(p, len, msghdr_p);
			break;

		case UMSG_CRC_CODE_RESP_ADD:
			rc = __decode_crc_add_resp(p, len, msghdr_p);
			break;
		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_DISPLAY:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_S_DISP:
		case UMSG_CRC_CODE_S_DISP_ALL:
		case UMSG_CRC_CODE_W_DISP:
		case UMSG_CRC_CODE_W_DISP_ALL:
			rc = __decode_crc_display(p, len, msghdr_p);
			break;

		case UMSG_CRC_CODE_S_RESP_DISP:
		case UMSG_CRC_CODE_S_RESP_DISP_ALL:
		case UMSG_CRC_CODE_W_RESP_DISP:
		case UMSG_CRC_CODE_W_RESP_DISP_ALL:
		case UMSG_CRC_CODE_DISP_END:
			rc = __decode_crc_display_resp(p, len, msghdr_p);
			break;

		default:
			rc = -1;
			nid_log_error("%s: unknown cmd code (%d)", log_header, msghdr_p->um_req_code);
			break;
		}
		break;

	case UMSG_CRC_CMD_HELLO:
		switch (msghdr_p->um_req_code) {
		case UMSG_CRC_CODE_HELLO:
		case UMSG_CRC_CODE_RESP_HELLO:
			rc = __decode_crc_hello(p, len, msghdr_p);
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
