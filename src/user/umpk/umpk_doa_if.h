/*
 * umpk_rc_if.h
 * 	umpk for rc
 */

#ifndef _NID_UMPK_DOA_IF_H
#define _NID_UMPK_DOA_IF_H

#include <stdint.h>
#include "nid.h"
#include "nid_shared.h"
#include "list.h"


#define DOA_MAX_VID		1024
#define DOA_MAX_LID		1024
/*
 * request cmd
 */
#define UMSG_DOA_CMD			40
#define UMSG_DOA_CMD_HELLO		2


/*
 * request code
 */


#define UMSG_DOA_CODE_REQUEST			1
#define UMSG_DOA_CODE_CHECK			2
#define UMSG_DOA_CODE_RELEASE			3
#define UMSG_DOA_CODE_OLD_REQUEST		4
#define UMSG_DOA_CODE_OLD_CHECK			5
#define UMSG_DOA_CODE_OLD_RELEASE		6


/* for cmd hello */
#define UMSG_DOA_CODE_HELLO		1
#define UMSG_DOA_CODE_RESP_HELLO	2

/*
 * items
 */
#define UMSG_DOA_ITEM_VID		1
#define UMSG_DOA_ITEM_LID		2
#define UMSG_DOA_ITEM_TIME_OUT		3
#define UMSG_DOA_ITEM_HOLD_TIME		4
#define UMSG_DOA_ITEM_LOCK	 	5


/*
 * Flags
 */
#define UMSG_DOA_HEADER_LEN		6

struct umessage_doa_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_doa_information {
	struct umessage_doa_hdr	um_header;
	char		um_doa_vid[DOA_MAX_VID];
	uint16_t	um_doa_vid_len;
	char		um_doa_lid[DOA_MAX_VID];
	uint16_t	um_doa_lid_len;
	uint32_t	um_doa_hold_time;
	uint32_t 	um_doa_lock;
	uint32_t	um_doa_time_out;

};

struct umessage_doa_hello {
	struct umessage_doa_hdr	um_header;
};

#endif
