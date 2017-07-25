/*
 * umpk_mrw_if.h
 * 	umpk for rw
 */

#ifndef _NID_UMPK_MRW_IF_H
#define _NID_UMPK_MRW_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */

#define UMSG_MRW_CMD_INFORMATION	1
#define UMSG_MRW_CMD_ADD		2
#define UMSG_MRW_CMD_DISPLAY		3
#define UMSG_MRW_CMD_HELLO		4

/*
 * request code
 */
#define UMSG_MRW_CODE_NO		0

/* for cmd info */
#define UMSG_MRW_CODE_STAT		1
#define UMSG_MRW_CODE_RESP_STAT		2

/* for cmd add */
#define UMSG_MRW_CODE_ADD		1
#define UMSG_MRW_CODE_RESP_ADD		2

/* for cmd display */
#define UMSG_MRW_CODE_S_DISP		1
#define UMSG_MRW_CODE_S_RESP_DISP	2
#define UMSG_MRW_CODE_S_DISP_ALL	3
#define UMSG_MRW_CODE_S_RESP_DISP_ALL	4
#define UMSG_MRW_CODE_W_DISP		5
#define UMSG_MRW_CODE_W_RESP_DISP	6
#define UMSG_MRW_CODE_W_DISP_ALL	7
#define UMSG_MRW_CODE_W_RESP_DISP_ALL	8
#define UMSG_MRW_CODE_DISP_END		9

/* for cmd hello */
#define UMSG_MRW_CODE_HELLO		1
#define UMSG_MRW_CODE_RESP_HELLO	2

/*
 * items
 */
#define UMSG_MRW_ITEM_CHANNEL		1
#define UMSG_MRW_ITEM_CACHE		2
#define UMSG_MRW_ITEM_WOP_NUM		3
#define UMSG_MRW_ITEM_WFP_NUM		4
#define UMSG_MRW_ITEM_MRW		5
#define UMSG_MRW_ITEM_RESP_CODE		6

#define UMSG_MRW_HEADER_LEN		6

struct umessage_mrw_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_mrw_information {
	struct umessage_mrw_hdr	um_header;
	uint8_t			um_mrw_name_len;
	char			um_mrw_name[NID_MAX_DEVNAME];
};

struct umessage_mrw_add {
	struct umessage_mrw_hdr	um_header;
	uint8_t			um_mrw_name_len;
	char			um_mrw_name[NID_MAX_DEVNAME];
};

struct umessage_mrw_display {
	struct umessage_mrw_hdr	um_header;
	uint8_t			um_mrw_name_len;
	char			um_mrw_name[NID_MAX_DEVNAME];
};

struct umessage_mrw_hello {
	struct umessage_mrw_hdr	um_header;
};

struct umessage_mrw_information_stat_resp {
	struct umessage_mrw_hdr	um_header;
	uint64_t		um_seq_wop_num;
	uint64_t		um_seq_wfp_num;
	uint16_t		um_seq_mrw;
};

struct umessage_mrw_add_resp {
	struct umessage_mrw_hdr	um_header;
	uint8_t			um_mrw_name_len;
	char			um_mrw_name[NID_MAX_DEVNAME];
	uint8_t			um_resp_code;
};

union umessage_mrw {
	struct umessage_mrw_information	info;
	struct umessage_mrw_add		add;
	struct umessage_mrw_display	display;
	struct umessage_mrw_hello	hello;
};

#if 0
union umessage_mrw_response {
	struct umessage_mrw_information_stat_resp	info_stat;
};
#endif
#endif
