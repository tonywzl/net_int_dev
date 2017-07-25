/*
 * umpk_agent_if.h
 *	umpk for agent
 */

#ifndef _NID_UMPK_AGENT_IF_H
#define _NID_UMPK_AGENT_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_AGENT_CMD_VERSION		1

/*
 * request code
 */
#define UMSG_AGENT_CODE_NO		0

/* for cmd agent */
#define UMSG_AGENT_CODE_VERSION	1
#define UMSG_AGENT_CODE_RESP_VERSION	2

/*
 * items
 */
#define UMSG_AGENT_ITEM_VERSION	1

#define UMSG_AGENT_HEADER_LEN		6

struct umessage_agent_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;
};

struct umessage_agent_version {
	struct umessage_agent_hdr	um_header;
};

struct umessage_agent_version_resp {
	struct umessage_agent_hdr	um_header;
	uint8_t				um_version_len;
	char				um_version[NID_MAX_VERSION];
};

union umessage_agent {
	struct umessage_agent_version	um_version;
};
#endif
