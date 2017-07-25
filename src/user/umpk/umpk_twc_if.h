/*
 * umpk_twc_if.h
 * 	umpk for twc
 */

#ifndef _NID_UMPK_TWC_IF_H
#define _NID_UMPK_TWC_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_TWC_CMD_INFORMATION	1
#define UMSG_TWC_CMD_THROUGHPUT		2
#define UMSG_TWC_CMD_DISPLAY		3
#define UMSG_TWC_CMD_HELLO		4

/*
 * request code
 */

/* for cmd info */
#define UMSG_TWC_CODE_FLUSHING	1
#define UMSG_TWC_CODE_STAT	3
#define UMSG_TWC_CODE_RESP_STAT	4
#define UMSG_TWC_CODE_THROUGHPUT_STAT	5
#define UMSG_TWC_CODE_THROUGHPUT_STAT_RSLT	6
#define UMSG_TWC_CODE_RW_STAT	7
#define UMSG_TWC_CODE_RW_STAT_RSLT	8

/* for cmd throughput */
#define UMSG_TWC_CODE_THROUGHPUT_RESET	1

/* for cmd display */
#define UMSG_TWC_CODE_S_DISP			1
#define UMSG_TWC_CODE_S_RESP_DISP		2
#define UMSG_TWC_CODE_S_DISP_ALL		3
#define UMSG_TWC_CODE_S_RESP_DISP_ALL		4
#define UMSG_TWC_CODE_W_DISP			5
#define UMSG_TWC_CODE_W_RESP_DISP		6
#define UMSG_TWC_CODE_W_DISP_ALL		7
#define UMSG_TWC_CODE_W_RESP_DISP_ALL		8
#define UMSG_TWC_CODE_DISP_END			9

/* for cmd hello */
#define UMSG_TWC_CODE_HELLO		1
#define UMSG_TWC_CODE_RESP_HELLO	2

/*
 * items
 */
#define UMSG_TWC_ITEM_CHANNEL		1
#define UMSG_TWC_ITEM_CACHE		2
#define UMSG_TWC_ITEM_SEQ_ASSIGNED	3
#define UMSG_TWC_ITEM_SEQ_FLUSHED	4
#define UMSG_TWC_ITEM_BLOCK_OCCUPIED	5
#define UMSG_TWC_ITEM_BLOCK_WRITE	6
#define UMSG_TWC_ITEM_BLOCK_FLUSH	7
#define UMSG_TWC_ITEM_DATA_FLUSHED	8
#define UMSG_TWC_ITEM_RES_NUM		9
#define UMSG_TWC_ITEM_TP_NAME		10
#define UMSG_TWC_ITEM_DO_FP		11

#define UMSG_TWC_HEADER_LEN		6

struct umessage_twc_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_twc_information {
	struct umessage_twc_hdr	um_header;
	uint16_t		um_twc_uuid_len;
	char			um_twc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_twc_throughput {
	struct umessage_twc_hdr	um_header;
	uint16_t		um_twc_uuid_len;
	char			um_twc_uuid[NID_MAX_UUID];

};

struct umessage_twc_information_throughput_stat {
	struct umessage_twc_hdr	um_header;
	uint16_t		um_twc_uuid_len;
	char			um_twc_uuid[NID_MAX_UUID];
	uint64_t		um_seq_nondata_pkg_flushed;
	uint64_t		um_seq_data_flushed;
	uint64_t		um_seq_data_pkg_flushed;
};

struct umessage_twc_information_rw_stat {
	struct umessage_twc_hdr	um_header;
	uint16_t		um_twc_uuid_len;
	char			um_twc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint64_t		um_chan_data_write;
	uint64_t		um_chan_data_read;
	uint64_t		res;
};

struct umessage_twc_display {
	struct umessage_twc_hdr	um_header;
	uint16_t		um_twc_uuid_len;
	char			um_twc_uuid[NID_MAX_UUID];
};

struct umessage_twc_information_resp_flushing {
	struct umessage_twc_hdr	um_header;
};

struct umessage_twc_information_resp_stat {
	struct umessage_twc_hdr	um_header;
	uint16_t		um_twc_uuid_len;
	char			um_twc_uuid[NID_MAX_UUID];
	uint64_t		um_seq_assigned;
	uint64_t		um_seq_flushed;
};

struct umessage_twc_display_resp {
	struct umessage_twc_hdr	um_header;
	uint16_t		um_twc_uuid_len;
	char			um_twc_uuid[NID_MAX_UUID];
	uint8_t			um_tp_name_len;
	char			um_tp_name[NID_MAX_TPNAME];
	uint8_t			um_do_fp;
};

struct umessage_twc_hello {
	struct umessage_twc_hdr um_header;
};

union umessage_twc {
	struct umessage_twc_information	info;
	struct umessage_twc_display	disp;
	struct umessage_twc_hello	hello;
};

union umessage_twc_response {
	struct umessage_twc_information_resp_flushing	info_flushing;
	struct umessage_twc_information_resp_stat	info_stat;
};

#endif
