/*
 * umpk_drw_if.h
 * 	umpk for drw
 */

#ifndef _NID_UMPK_DRW_IF_H
#define _NID_UMPK_DRW_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_DRW_CMD_ADD			1
#define UMSG_DRW_CMD_DELETE			2
#define UMSG_DRW_CMD_READY			3
#define UMSG_DRW_CMD_DISPLAY			4
#define UMSG_DRW_CMD_HELLO			5

/*
 * request code
 */
#define UMSG_DRW_CODE_NO			0

/* for cmd add */
#define UMSG_DRW_CODE_ADD			1
#define UMSG_DRW_CODE_RESP_ADD			2

/* for cmd delete */
#define UMSG_DRW_CODE_DELETE			1
#define UMSG_DRW_CODE_RESP_DELETE		2

/* for cmd ready */
#define UMSG_DRW_CODE_READY			1
#define UMSG_DRW_CODE_RESP_READY		2

/* for cmd display */
#define UMSG_DRW_CODE_S_DISP			1
#define UMSG_DRW_CODE_S_RESP_DISP		2
#define UMSG_DRW_CODE_S_DISP_ALL		3
#define UMSG_DRW_CODE_S_RESP_DISP_ALL		4
#define UMSG_DRW_CODE_W_DISP			5
#define UMSG_DRW_CODE_W_RESP_DISP		6
#define UMSG_DRW_CODE_W_DISP_ALL		7
#define UMSG_DRW_CODE_W_RESP_DISP_ALL		8
#define UMSG_DRW_CODE_DISP_END			9


/* for cmd hello */
#define UMSG_DRW_CODE_HELLO			1
#define UMSG_DRW_CODE_RESP_HELLO		2

/*
 * items
 */
#define	UMSG_DRW_ITEM_DRW			1
#define	UMSG_DRW_ITEM_RESP_CODE			2
#define UMSG_DRW_ITEM_EXPORTNAME		3
#define UMSG_DRW_ITEM_SIMULATE			4
#define UMSG_DRW_ITEM_PROVISION			5

#define UMSG_DRW_HEADER_LEN			6

struct umessage_drw_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_drw_add {
	struct umessage_drw_hdr	um_header;
	uint8_t			um_drw_name_len;
	char			um_drw_name[NID_MAX_DEVNAME];
	uint16_t		um_exportname_len;
	char			um_exportname[NID_MAX_PATH];
	uint8_t			um_device_provision;
};

struct umessage_drw_delete {
	struct umessage_drw_hdr	um_header;
	uint8_t			um_drw_name_len;
	char			um_drw_name[NID_MAX_DEVNAME];
};

struct umessage_drw_ready {
	struct umessage_drw_hdr	um_header;
	uint8_t			um_drw_name_len;
	char			um_drw_name[NID_MAX_DEVNAME];
};

struct umessage_drw_display {
	struct umessage_drw_hdr	um_header;
	uint8_t			um_drw_name_len;
	char			um_drw_name[NID_MAX_DEVNAME];
};

struct umessage_drw_hello {
	struct umessage_drw_hdr um_header;
};

struct umessage_drw_add_resp {
	struct umessage_drw_hdr	um_header;
	uint8_t			um_drw_name_len;
	char			um_drw_name[NID_MAX_DEVNAME];
	uint8_t			um_resp_code;
};

struct umessage_drw_delete_resp {
	struct umessage_drw_hdr	um_header;
	uint8_t			um_drw_name_len;
	char			um_drw_name[NID_MAX_DEVNAME];
	uint8_t			um_resp_code;
};

struct umessage_drw_ready_resp {
	struct umessage_drw_hdr	um_header;
	uint8_t			um_drw_name_len;
	char			um_drw_name[NID_MAX_DEVNAME];
	uint8_t			um_resp_code;
};

struct umessage_drw_display_resp {
	struct umessage_drw_hdr um_header;
	uint8_t                 um_drw_name_len;
	char                    um_drw_name[NID_MAX_DEVNAME];
	uint16_t		um_exportname_len;
	char                    um_exportname[NID_MAX_PATH];
	uint8_t			um_simulate_async;
	uint8_t			um_simulate_delay;
	uint8_t			um_simulate_delay_min_gap;
	uint8_t			um_simulate_delay_max_gap;
	uint16_t		um_simulate_delay_time_us;
	uint8_t			um_device_provision;
};

union umessage_drw {
	struct umessage_drw_add		um_add;
	struct umessage_drw_delete	um_del;
	struct umessage_drw_ready	um_ready;
	struct umessage_drw_hello	um_hello;
	struct umessage_drw_display	um_display;
};

#endif
