/*
 * umpk_tp_if.h
 *	umpk for tp
 */

#ifndef _NID_UMPK_TP_IF_H
#define _NID_UMPK_TP_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_TP_CMD_INFORMATION		1
#define UMSG_TP_CMD_ADD			2
#define UMSG_TP_CMD_DELETE		3
#define UMSG_TP_CMD_DISPLAY		4
#define UMSG_TP_CMD_HELLO		5

/*
 * request code
 */
#define UMSG_TP_CODE_NO			0

/* for cmd info */
#define UMSG_TP_CODE_STAT		1
#define UMSG_TP_CODE_RESP_STAT		2
#define UMSG_TP_CODE_STAT_ALL		3
#define UMSG_TP_CODE_RESP_STAT_ALL	4
#define UMSG_TP_CODE_RESP_END		5
#define UMSG_TP_CODE_RESP_NOT_FOUND	6

/* for cmd add */
#define UMSG_TP_CODE_ADD		1
#define UMSG_TP_CODE_RESP_ADD		2

/* for cmd delete */
#define UMSG_TP_CODE_DELETE		1
#define UMSG_TP_CODE_RESP_DELETE	2

/* for cmd display */
#define UMSG_TP_CODE_S_DISP		1
#define UMSG_TP_CODE_S_RESP_DISP	2
#define UMSG_TP_CODE_S_DISP_ALL		3
#define UMSG_TP_CODE_S_RESP_DISP_ALL	4
#define UMSG_TP_CODE_W_DISP		5
#define UMSG_TP_CODE_W_RESP_DISP	6
#define UMSG_TP_CODE_W_DISP_ALL		7
#define UMSG_TP_CODE_W_RESP_DISP_ALL	8
#define UMSG_TP_CODE_DISP_END		9

/* for cmd hello */
#define UMSG_TP_CODE_HELLO		1
#define UMSG_TP_CODE_RESP_HELLO		2

/*
 * items
 */
#define UMSG_TP_ITEM_TP			1
#define UMSG_TP_ITEM_STAT		2
#define UMSG_TP_ITEM_SETUP		3
#define UMSG_TP_ITEM_WORKERS		4
#define UMSG_TP_ITEM_RESP_CODE		5

#define UMSG_TP_HEADER_LEN		6

struct umessage_tp_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;
};

struct umessage_tp_information {
	struct umessage_tp_hdr 	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
};

struct umessage_tp_information_resp_stat {
	struct umessage_tp_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
	uint16_t		um_nused;
	uint16_t		um_nfree;
	uint16_t		um_workers;
	uint16_t		um_max_workers;
	uint32_t		um_no_free;
};

struct umessage_tp_add {
	struct umessage_tp_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
	uint16_t		um_workers;
};

struct umessage_tp_delete {
	struct umessage_tp_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
};

struct umessage_tp_delete_resp {
	struct umessage_tp_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_tp_add_resp {
	struct umessage_tp_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_tp_display {
	struct umessage_tp_hdr	um_header;
	uint16_t		um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
};

struct umessage_tp_display_resp {
	struct umessage_tp_hdr	um_header;
	uint16_t                um_uuid_len;
	char			um_uuid[NID_MAX_UUID];
	uint16_t		um_min_workers;
	uint16_t		um_max_workers;
	uint16_t		um_extend;
	uint16_t		um_delay;
};


struct umessage_tp_hello {
	struct umessage_tp_hdr um_header;
};

union umessage_tp {
	struct umessage_tp_information	um_information;
	struct umessage_tp_add		um_add;
	struct umessage_tp_delete	um_del;
	struct umessage_tp_display	um_display;
	struct umessage_tp_hello	um_hello;
};
#endif
