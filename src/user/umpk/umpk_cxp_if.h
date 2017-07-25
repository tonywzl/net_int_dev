/*
 * _UMPK_CXP_if.h
 * 	umpk for cxp
 */

#ifndef _NID_UMPK_CXP_IF_H
#define _NID_UMPK_CXP_IF_H

#include "nid_shared.h"
#include "tx_shared.h"

/*
 * request cmd
 */
#define UMSG_CXP_CMD_CREATE_CHANNEL		1
#define UMSG_CXP_CMD_KEEPALIVE			2
#define UMSG_CXP_CMD_DROP_CHANNEL		3
#define UMSG_CXP_CMD_STAT_CHANNEL		4
#define UMSG_CXP_CMD_START_WORK			5
#define UMSG_CXP_CMD_DROP_CHANNEL_RESP		6

/*
 * request code
 */
#define UMSG_CXP_CODE_NO			0

/* for cmd create channel */
#define UMSG_CXP_CODE_NO			0
#define UMSG_CXP_CODE_UUID			1

/* for cmd keepalive channel resp */
#define UMSG_CXP_KEEPALIVE_RESP		1

/* for cmd drop channel resp */
#define UMSG_CXP_DROP_RESP_SUCCESS		0
#define UMSG_CXP_DROP_RESP_FAILE		1

/* for cmd stat channel resp */
#define UMSG_CXP_CODE_STAT_RESP			1

/* for cmd start work */
#define UMSG_CXP_CODE_DROP_ALL			1
#define UMSG_CXP_START_WORK_RESP		2

/*
 * items
 */
#define UMSG_CXP_ITEM_CHANNEL			1

#define UMSG_CXP_ITEM_CXA_UUID			1
#define UMSG_CXP_ITEM_CXP_UUID			2

struct umessage_cxp_create_channel {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_cxp_drop_channel {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_cxa_uuid_len;
	char			um_chan_cxa_uuid[NID_MAX_UUID];
	uint16_t		um_chan_cxp_uuid_len;
	char			um_chan_cxp_uuid[NID_MAX_UUID];
};

struct umessage_cxp_drop_channel_resp {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_cxp_uuid_len;
	char			um_chan_cxp_uuid[NID_MAX_UUID];
};

struct umessage_cxp_stat_channel {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_cxp_stat_channel_resp {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint64_t		send_seq;	// max seq of send control message
	uint64_t		ack_seq;	// max seq of ack message send out
	uint64_t		recv_seq;	// max seq of received control message
	uint32_t		cmsg_left;	// un-processed bytes in control message receive buffer
};

struct umessage_cxp_start_work {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_cxp_start_work_resp {
	struct umessage_tx_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};


union umessage_cxp {
	struct umessage_cxp_create_channel um_create_channel_uuid;
	struct umessage_cxp_drop_channel	um_drop_channel;
	struct umessage_cxp_drop_channel_resp	um_drop_channel_resp;
	struct umessage_cxp_stat_channel	um_stat_channel;
	struct umessage_cxp_stat_channel_resp	um_stat_channel_resp;
	struct umessage_cxp_start_work		um_start_work;
	struct umessage_cxp_start_work_resp	um_start_work_resp;
};

#endif
