/*
 * umpk_dxa_if.h
 * 	umpk for dxa
 */

#ifndef _NID_UMPK_DXA_IF_H
#define _NID_UMPK_DXA_IF_H

#include <stdint.h>
#include <netinet/tcp.h>

#include "nid_shared.h"
#include "tx_shared.h"

/*
 * request cmd
 */
#define UMSG_DXA_CMD_CREATE_CHANNEL		1
#define UMSG_DXA_CMD_KEEPALIVE			2
#define UMSG_DXA_CMD_DROP_CHANNEL		3
#define UMSG_DXA_CMD_DISPLAY			4
#define UMSG_DXA_CMD_HELLO			5

/*
 * request code
 */
#define UMSG_DXA_CODE_NO			0

/* for cmd create channel */
#define UMSG_DXA_CODE_DPORT			1

/* for cmd keepalive channel resp */
#define UMSG_DXA_KEEPALIVE_RESP		1

/* for cmd drop channel resp */
#define UMSG_DXA_DROP_RESP_SUCCESS		0
#define UMSG_DXA_DROP_RESP_FAILE		1

/* for cmd display */
#define UMSG_DXA_CODE_S_DISP			1
#define UMSG_DXA_CODE_S_RESP_DISP		2
#define UMSG_DXA_CODE_S_DISP_ALL		3
#define UMSG_DXA_CODE_S_RESP_DISP_ALL		4
#define UMSG_DXA_CODE_W_DISP			5
#define UMSG_DXA_CODE_W_RESP_DISP		6
#define UMSG_DXA_CODE_W_DISP_ALL		7
#define UMSG_DXA_CODE_W_RESP_DISP_ALL		8
#define UMSG_DXA_CODE_DISP_END			9

/* for cmd hello */
#define UMSG_DXA_CODE_HELLO			1
#define UMSG_DXA_CODE_RESP_HELLO		2

/*
 * items
 */
#define UMSG_DXA_ITEM_DXA_UUID			1
#define UMSG_DXA_ITEM_DXP_UUID			2
#define UMSG_DXA_ITEM_UUID			3
#define UMSG_DXA_ITEM_DPORT			4
#define UMSG_DXA_ITEM_PEER_UUID			5
#define UMSG_DXA_ITEM_IP			6
#define UMSG_DXA_ITEM_DXT_NAME			7

struct umessage_dxa_create_channel_uuid {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_dxa_uuid_len;
	char			um_chan_dxa_uuid[NID_MAX_UUID];
	uint16_t		um_chan_dxp_uuid_len;
	char			um_chan_dxp_uuid[NID_MAX_UUID];
};

struct umessage_dxa_create_channel_dport {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	u_short			um_dport;
};

struct umessage_dxa_drop_channel {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_dxa_uuid_len;
	char			um_chan_dxa_uuid[NID_MAX_UUID];
	uint16_t		um_chan_dxp_uuid_len;
	char			um_chan_dxp_uuid[NID_MAX_UUID];
};

struct umessage_dxa_display {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_dxa_uuid_len;
	char			um_chan_dxa_uuid[NID_MAX_UUID];
};

struct umessage_dxa_display_resp {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_dxa_uuid_len;
	char			um_chan_dxa_uuid[NID_MAX_UUID];
	uint16_t		um_peer_uuid_len;
	char			um_peer_uuid[NID_MAX_UUID];
	uint8_t			um_ip_len;
	char			um_ip[NID_MAX_IP];
	uint16_t		um_dxt_name_len;
	char			um_dxt_name[NID_MAX_UUID];
};

struct umessage_dxa_hello {
	struct umessage_tx_hdr	um_header;
};

union umessage_dxa {
	struct umessage_dxa_create_channel_uuid		um_create_channel_uuid;
	struct umessage_dxa_create_channel_dport	um_create_channel_dport;
	struct umessage_dxa_drop_channel		um_drop_channel;
	struct umessage_dxa_display			um_display;
	struct umessage_dxa_hello			um_hello;
};

#endif
