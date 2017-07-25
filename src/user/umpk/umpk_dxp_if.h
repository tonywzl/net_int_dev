/*
 * umpk_dxp_if.h
 * 	umpk for dxp
 */

#ifndef _NID_UMPK_DXP_IF_H
#define _NID_UMPK_DXP_IF_H

#include "nid_shared.h"
#include "tx_shared.h"

/*
 * request cmd
 */
#define UMSG_DXP_CMD_CREATE_CHANNEL		1
#define UMSG_DXP_CMD_KEEPALIVE			2
#define UMSG_DXP_CMD_DROP_CHANNEL		3
#define UMSG_DXP_CMD_STAT_CHANNEL		4
#define UMSG_DXP_CMD_START_WORK			5
#define UMSG_DXP_CMD_DROP_CHANNEL_RESP		6

/*
 * request code
 */
#define UMSG_DXP_CODE_NO			0

/* for cmd create channel */
#define UMSG_DXP_CODE_UUID			1

/* for cmd keepalive channel resp */
#define UMSG_DXP_KEEPALIVE_RESP			1

/* for cmd drop channel resp */
#define UMSG_DXP_DROP_RESP_SUCCESS		0
#define UMSG_DXP_DROP_RESP_FAILE		1

/* for cmd stat channel resp */
#define UMSG_DXP_CODE_STAT_RESP			1

/* for cmd start work */
#define UMSG_DXP_CODE_DROP_ALL			1
#define UMSG_DXP_START_WORK_RESP		2

/*
 * items
 */
#define UMSG_DXP_ITEM_CHANNEL			1

#define UMSG_DXP_ITEM_DXA_UUID			1
#define UMSG_DXP_ITEM_DXP_UUID			2
#define UMSG_DXP_ITEM_UUID			3

struct umessage_dxp_create_channel {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_dxp_drop_channel {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_dxp_uuid_len;
	char			um_chan_dxp_uuid[NID_MAX_UUID];
};

struct umessage_dxp_drop_channel_resp {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_dxp_uuid_len;
	char			um_chan_dxp_uuid[NID_MAX_UUID];
};

struct umessage_dxp_stat_channel {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_dxp_stat_channel_resp {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint64_t		dsend_seq;	// max seq of send data message
	uint64_t		send_seq;	// max seq of send control message
	uint64_t		ack_seq;	// max seq of ack message send out
	uint64_t		recv_seq;	// max seq of received control message
	uint64_t		recv_dseq;	// max dseq received data message
	uint32_t		cmsg_left;	// un-processed bytes in control message receive buffer
};

struct umessage_dxp_start_work {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_dxp_start_work_resp {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

union umessage_dxp {
	struct umessage_dxp_create_channel	um_create_channel;
	struct umessage_dxp_drop_channel	um_drop_channel;
	struct umessage_dxp_drop_channel_resp	um_drop_channel_resp;
	struct umessage_dxp_stat_channel	um_stat_channel;
	struct umessage_dxp_stat_channel_resp	um_stat_channel_resp;
	struct umessage_dxp_start_work		um_start_work;
	struct umessage_dxp_start_work_resp	um_start_work_resp;
};

#endif
