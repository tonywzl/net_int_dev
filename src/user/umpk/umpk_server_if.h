/*
 * umpk_server_if.h
 *	umpk for server
 */

#ifndef _NID_UMPK_SERVER_IF_H
#define _NID_UMPK_SERVER_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_SERVER_CMD_VERSION		1

/*
 * request code
 */
#define UMSG_SERVER_CODE_NO		0

/* for cmd server */
#define UMSG_SERVER_CODE_VERSION	1
#define UMSG_SERVER_CODE_RESP_VERSION	2

/*
 * items
 */
#define UMSG_SERVER_ITEM_VERSION	1

#define UMSG_SERVER_HEADER_LEN		6

struct umessage_server_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;
};

struct umessage_server_version {
	struct umessage_server_hdr	um_header;
};

struct umessage_server_version_resp {
	struct umessage_server_hdr	um_header;
	uint8_t				um_version_len;
	char				um_version[NID_MAX_VERSION];
};

union umessage_server {
	struct umessage_server_version	um_version;
};
#endif
