/*
 * umpk_drc_if.h
 * 	umpk for drc
 */

#ifndef _NID_UMPK_DRC_IF_H
#define _NID_UMPK_DRC_IF_H

#include "nid_shared.h"

#define UMSG_DCK_HEADER_LEN            6

/*
 * request cmd
 */
#define UMSG_DRC_CMD_INFORMATION	1
#define	UMSG_DRC_CMD_DROPCACHE		2
#define UMSG_DRC_CMD_HELLO		3

/*
 * request code
 */

/* for cmd dc */
#define UMSG_DRC_CODE_START		1
#define UMSG_DRC_CODE_STOP		2

/* for cmd info */
#define UMSG_DRC_CODE_STAT		1
#define UMSG_DRC_CODE_RESP_STAT		2

#define UMSG_DRC_CODE_HELLO		1
#define UMSG_DRC_CODE_RESP_HELLO	2

/*
 * items
 */
#define UMSG_DRC_ITEM_CHANNEL		1
#define UMSG_DRC_ITEM_CACHE		2
#define UMSG_DRC_ITEM_BLOCK_OCCUPIED	3

#define UMSG_DRC_HEADER_LEN		6
struct umessage_drc_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_drc_dropcache {
	struct umessage_drc_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
};

struct umessage_drc_information {
	struct umessage_drc_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
};

struct umessage_drc_information_resp_flushing {
	struct umessage_drc_hdr	um_header;
};

struct umessage_drc_information_resp_stat {
	struct umessage_drc_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
	uint16_t		block_occupied;
	uint16_t		um_drc_state;
};

struct umessage_drc_hello {
	struct umessage_drc_hdr	um_header;
};

union umessage_drc {
	struct umessage_drc_dropcache	dc;
	struct umessage_drc_information	info;
	struct umessage_drc_hello	hello;
};

union umessage_drc_response {
	struct umessage_drc_information_resp_flushing	info_flushing;
	struct umessage_drc_information_resp_stat	info_stat;
};


#endif
