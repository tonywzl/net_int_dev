/*
 * umpk_wc_if.h
 * 	umpk for wc
 */

#ifndef _NID_UMPK_WC_IF_H
#define _NID_UMPK_WC_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_WC_CMD_INFORMATION	1
#define UMSG_WC_CMD_HELLO	2

/*
 * request code
 */

/* for cmd info */
#define UMSG_WC_CODE_FLUSHING	1
#define UMSG_WC_CODE_STAT	3
#define UMSG_WC_CODE_RESP_STAT	4

/* for cmd hello */
#define UMSG_WC_CODE_HELLO	1
#define UMSG_WC_CODE_RESP_HELLO	2

/*
 * items
 */
#define UMSG_WC_ITEM_CHANNEL		1
#define UMSG_WC_ITEM_CACHE		2
#define UMSG_WC_ITEM_SEQ_ASSIGNED	3
#define UMSG_WC_ITEM_SEQ_FLUSHED	4
#define UMSG_WC_ITEM_BLOCK_OCCUPIED	5
#define UMSG_WC_ITEM_BLOCK_WRITE	6
#define UMSG_WC_ITEM_BLOCK_FLUSH	7
#define UMSG_WC_ITEM_FF_STATE		8
#define UMSG_WC_ITEM_NBLOCKS		9

#define UMSG_WC_HEADER_LEN		6
struct umessage_wc_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_wc_information {
	struct umessage_wc_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
};

struct umessage_wc_hello {
	struct umessage_wc_hdr	um_header;
};

struct umessage_wc_information_resp_flushing {
	struct umessage_wc_hdr	um_header;
};

struct umessage_wc_information_resp_stat {
	struct umessage_wc_hdr	wc_header;
	uint16_t		wc_uuid_len;
	char			wc_uuid[NID_MAX_UUID];
	uint16_t		wc_block_occupied;
	uint16_t		wc_flush_nblocks;
	uint16_t		wc_cur_write_block;
	uint16_t		wc_cur_flush_block;
	uint64_t		wc_seq_assigned;
	uint64_t		wc_seq_flushed;
};

union umessage_wc {
	struct umessage_wc_information	info;
	struct umessage_wc_hello	hello;
};

union umessage_wc_response {
	struct umessage_wc_information_resp_flushing	info_flushing;
	struct umessage_wc_information_resp_stat	info_stat;
};

#endif
