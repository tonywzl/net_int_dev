/*
 * umpk_rept_if.h
 *	umpk for Replication Target
 */

#ifndef _NID_UMPK_REPT_IF_H
#define _NID_UMPK_REPT_IF_H

#include "nid_shared.h"
#include "umpk_reps_if.h"

/*
 * request cmd
 */
#define UMSG_REPT_CMD_HELLO		1
#define UMSG_REPT_CMD_START		2
#define UMSG_REPT_CMD_DISPLAY		3
#define UMSG_REPT_CMD_INFO		4

/*
 * request code
 */
#define UMSG_REPT_CODE_NO		0

/* for cmd hello */
#define UMSG_REPT_CODE_HELLO		1
#define UMSG_REPT_CODE_RESP_HELLO	2

/* for cmd start */
#define UMSG_REPT_CODE_START		1
#define UMSG_REPT_CODE_RESP_START	2

/* for cmd display */
#define UMSG_REPT_CODE_S_DISP			1
#define UMSG_REPT_CODE_S_RESP_DISP		2
#define UMSG_REPT_CODE_S_DISP_ALL		3
#define UMSG_REPT_CODE_S_RESP_DISP_ALL		4
#define UMSG_REPT_CODE_W_DISP			5
#define UMSG_REPT_CODE_W_RESP_DISP		6
#define UMSG_REPT_CODE_W_DISP_ALL		7
#define UMSG_REPT_CODE_W_RESP_DISP_ALL		8
#define UMSG_REPT_CODE_DISP_END			9

/* for cmd info */
#define UMSG_REPT_CODE_INFO_PRO		1
#define UMSG_REPT_CODE_RESP_INFO_PRO	2

/*
 * items
 */
#define UMSG_REPT_ITEM_REPT		1
#define UMSG_REPT_ITEM_RESP_CODE	2
#define UMSG_REPT_ITEM_DXP		3
#define UMSG_REPT_ITEM_VOLUUID		4
#define UMSG_REPT_ITEM_BITMAP		5
#define UMSG_REPT_ITEM_VOL		6
#define UMSG_REPT_ITEM_PROGRESS		7

#define UMSG_REPT_HEADER_LEN		6

struct umessage_rept_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;
};

struct umessage_rept_hello {
	struct umessage_rept_hdr	um_header;
};

struct umessage_rept_start {
	struct umessage_rept_hdr	um_header;
	uint16_t			um_rt_name_len;
	char				um_rt_name[NID_MAX_UUID];
};

struct umessage_rept_display {
	struct umessage_rept_hdr	um_header;
	uint16_t			um_rt_name_len;
	char				um_rt_name[NID_MAX_UUID];
};

struct umessage_rept_info {
	struct umessage_rept_hdr	um_header;
	uint16_t			um_rt_name_len;
	char				um_rt_name[NID_MAX_UUID];
};

struct umessage_rept_start_resp {
	struct umessage_rept_hdr	um_header;
	uint8_t				um_resp_code;
};

struct umessage_rept_display_resp {
	struct umessage_rept_hdr	um_header;
	uint16_t			um_rt_name_len;
	char				um_rt_name[NID_MAX_UUID];
	uint16_t			um_dxp_name_len;
	char				um_dxp_name[NID_MAX_UUID];
	uint16_t			um_voluuid_len;
	char				um_voluuid[NID_MAX_UUID];
	size_t				um_bitmap_len;
	size_t				um_vol_len;
};

struct umessage_rept_info_pro_resp {
	struct umessage_rept_hdr	um_header;
	float				um_progress;
};

union umessage_rept {
	struct umessage_rept_hello	um_hello;
	struct umessage_rept_start	um_start;
	struct umessage_rept_display	um_display;
	struct umessage_rept_info	um_info;
};

#endif
