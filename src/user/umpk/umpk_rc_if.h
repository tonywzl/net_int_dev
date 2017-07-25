/*
 * umpk_rc_if.h
 * 	umpk for rc
 */

#ifndef _NID_UMPK_RC_IF_H
#define _NID_UMPK_RC_IF_H

#include <stdint.h>
#include "nid.h"
#include "nid_shared.h"
#include "list.h"

/*
 * request cmd
 */
#define UMSG_CRC_CMD_DROPCACHE			1
#define UMSG_CRC_CMD_INFORMATION			2

/*
 * request code
 */

/* for cmd dc */
#define UMSG_CRC_CODE_START			1
#define UMSG_CRC_CODE_START_SYNC			2
#define UMSG_CRC_CODE_STOP			3
#define UMSG_CRC_CODE_INFO			4

/* for cmd info */
#define UMSG_CRC_CODE_FLUSHING			1
#define UMSG_CRC_CODE_FREE_SPACE			4
#define UMSG_CRC_CODE_FREE_SPACE_DIST		5
#define UMSG_CRC_CODE_FREE_SPACE_RSLT		6
#define UMSG_CRC_CODE_FREE_SPACE_DIST_RSLT	7
#define UMSG_CRC_CODE_NSE_COUNTER		8
#define UMSG_CRC_CODE_RESP_NSE_COUNTER		9
#define UMSG_CRC_CODE_NSE_DETAIL			10
#define UMSG_CRC_CODE_RESP_NSE_DETAIL		11
#define UMSG_CRC_CODE_DSBMP_RTREE		12
#define UMSG_CRC_CODE_DSBMP_RTREE_RSLT		13
#define UMSG_CRC_CODE_BSN_RSLT			14
#define UMSG_CRC_CODE_DSBMP_RTREE_END_RSLT	15
#define UMSG_CRC_CODE_CHECK_FP			16
#define UMSG_CRC_CODE_CHECK_FP_RSLT		17

/*
 * items
 */
#define UMSG_CRC_ITEM_CHANNEL		1
#define UMSG_CRC_ITEM_CACHE		2
#define UMSG_CRC_ITEM_NUM_SIZES		3
#define UMSG_CRC_ITEM_SIZE		4
#define UMSG_CRC_ITEM_SIZE_NUMBER 	5
#define UMSG_CRC_ITEM_FREE_SPACE		6
#define UMSG_CRC_ITEM_COUNTER		7
#define UMSG_CRC_ITEM_BSN_OFFSET		8
#define UMSG_CRC_ITEM_BSN_SIZE		9
#define UMSG_CRC_ITEM_BLOCK_INDEX	10
#define UMSG_CRC_ITEM_FP			11
#define UMSG_CRC_ITEM_FLAG		12
#define UMSG_CRC_ITEM_CHECK_FP_RC	13

/*
 * Flags
 */
#define UMSG_CRC_FLAG_START		0x01
#define UMSG_CRC_FLAG_END		0x02

#define UMSG_CRC_HEADER_LEN		6

struct umessage_crc_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_crc_dropcache {
	struct umessage_crc_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_crc_information {
	struct umessage_crc_hdr	um_header;
	uint16_t		um_rc_uuid_len;
	char			um_rc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_crc_information_resp_freespace {
	struct umessage_crc_hdr	um_header;
	uint64_t		um_num_blocks_free;
};

struct space_size_number {
  	uint64_t 		um_size;
  	uint64_t 		um_number;
  	struct list_head	um_ss_list;

};

struct umessage_crc_information_resp_freespace_dist {
	struct umessage_crc_hdr	um_header;
  	uint64_t 		um_num_sizes;
  	struct list_head 	um_size_list_head;
  	struct list_head 	large_size_list_head;

};

struct umessage_crc_information_resp_nse_stae{
	struct umessage_crc_hdr	um_header;
	uint16_t		um_rc_uuid_len;
	char			um_rc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	struct nse_stat		um_nse_stat;
};

struct umessage_crc_information_resp_nse_detail {
	struct umessage_crc_hdr	um_header;
	char			um_fp[NID_SIZE_FP];
	uint64_t		um_block_index;
	uint16_t		um_rc_uuid_len;
	char			um_rc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	struct nse_stat		um_nse_stat;
	uint64_t		um_flags;
};

struct umessage_crc_information_resp_dsbmp_bsn {
	struct umessage_crc_hdr	um_header;
  	uint64_t 		um_offset;
	uint64_t		um_size;
};

struct umessage_crc_information_resp_dsbmp_rtree_node {
	struct umessage_crc_hdr	um_header;
  	uint64_t 		um_num_nodes;
};

struct umessage_crc_information_resp_dsbmp_rtree_node_finish {
	struct umessage_crc_hdr	um_header;
};

struct umessage_crc_information_resp_check_fp {
  	struct umessage_crc_hdr	um_header;
  	unsigned char		um_check_fp_rc;
};

union umessage_crc {
	struct umessage_crc_dropcache	dc;
	struct umessage_crc_information	info;
};

#endif
