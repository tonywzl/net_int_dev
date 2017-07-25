/*
 * umpk_driver_if.h
 *	umpk for driver
 */

#ifndef _NID_UMPK_DRIVER_IF_H
#define _NID_UMPK_DRIVER_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_DRIVER_CMD_VERSION		1

/*
 * request code
 */
#define UMSG_DRIVER_CODE_NO		0

/* for cmd driver */
#define UMSG_DRIVER_CODE_VERSION	1
#define UMSG_DRIVER_CODE_RESP_VERSION	2

/*
 * items
 */
#define UMSG_DRIVER_ITEM_VERSION	1

#define UMSG_DRIVER_HEADER_LEN		6

struct umessage_driver_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;
};

struct umessage_driver_version {
	struct umessage_driver_hdr	um_header;
};

struct umessage_driver_version_resp {
	struct umessage_driver_hdr	um_header;
	uint8_t				um_version_len;
	char				um_version[NID_MAX_VERSION];
};

union umessage_driver {
	struct umessage_driver_version	um_version;
};
#endif
