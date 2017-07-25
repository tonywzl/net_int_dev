/*
 * umpk_cxa_if.h
 * 	umpk for cxa
 */

#ifndef _NID_UMPK_CXA_IF_H
#define _NID_UMPK_CXA_IF_H

#include <stdint.h>
#include <netinet/tcp.h>

#include "nid_shared.h"
#include "tx_shared.h"

/*
 * request cmd
 */
#define UMSG_CXA_CMD_KEEPALIVE			1
#define UMSG_CXA_CMD_DROP_CHANNEL		2
#define UMSG_CXA_CMD_DISPLAY			3
#define UMSG_CXA_CMD_HELLO			4

/*
 * request code
 */
#define UMSG_CXA_CODE_NO			0

/* for cmd keepalive channel resp */
#define UMSG_CXA_KEEPALIVE_RESP		1

/* for cmd drop channel resp */
#define UMSG_CXA_DROP_RESP_SUCCESS		0
#define UMSG_CXA_DROP_RESP_FAILE		1

/* for cmd display */
#define UMSG_CXA_CODE_S_DISP			1
#define UMSG_CXA_CODE_S_RESP_DISP		2
#define UMSG_CXA_CODE_S_DISP_ALL		3
#define UMSG_CXA_CODE_S_RESP_DISP_ALL		4
#define UMSG_CXA_CODE_W_DISP			5
#define UMSG_CXA_CODE_W_RESP_DISP		6
#define UMSG_CXA_CODE_W_DISP_ALL		7
#define UMSG_CXA_CODE_W_RESP_DISP_ALL		8
#define UMSG_CXA_CODE_DISP_END			9

/* for cmd hello */
#define UMSG_CXA_CODE_HELLO			1
#define UMSG_CXA_CODE_RESP_HELLO		2	

/*
 * items
 */
#define UMSG_CXA_ITEM_CXA_UUID			1
#define UMSG_CXA_ITEM_CXP_UUID			2
#define UMSG_CXA_ITEM_PEER_UUID			3
#define UMSG_CXA_ITEM_IP			4
#define UMSG_CXA_ITEM_CXT_NAME			5

struct umessage_cxa_drop_channel {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_cxa_uuid_len;
	char			um_chan_cxa_uuid[NID_MAX_UUID];
	uint16_t		um_chan_cxp_uuid_len;
	char			um_chan_cxp_uuid[NID_MAX_UUID];
};

struct umessage_cxa_display {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_cxa_uuid_len;
	char			um_chan_cxa_uuid[NID_MAX_UUID];
};

struct umessage_cxa_display_resp {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_cxa_uuid_len;
	char			um_chan_cxa_uuid[NID_MAX_UUID];
	uint16_t		um_peer_uuid_len;
	char			um_peer_uuid[NID_MAX_UUID];
	uint8_t			um_ip_len;
	char			um_ip[NID_MAX_IP];
	uint16_t		um_cxt_name_len;
	char			um_cxt_name[NID_MAX_UUID];
};

struct umessage_cxa_hello {
	struct umessage_tx_hdr um_header;
};

union umessage_cxa {
	struct umessage_cxa_drop_channel		um_drop_channel;
	struct umessage_cxa_display			um_display;
	struct umessage_cxa_hello			um_hello;
};


#endif
