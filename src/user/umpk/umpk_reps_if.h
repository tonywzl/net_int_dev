/*
 * umpk_reps_if.h
 *	umpk for Replication Source
 */

#ifndef _NID_UMPK_REPS_IF_H
#define _NID_UMPK_REPS_IF_H

#include "nid_shared.h"
#include "umpk_rept_if.h"

/*
 * request cmd
 */
#define UMSG_REPS_CMD_HELLO		1
#define UMSG_REPS_CMD_START		2
#define UMSG_REPS_CMD_DISPLAY		3
#define UMSG_REPS_CMD_INFO		4
#define UMSG_REPS_CMD_PAUSE		5
#define UMSG_REPS_CMD_CONTINUE		6

/*
 * request code
 */
#define UMSG_REPS_CODE_NO		0

/* for cmd hello */
#define UMSG_REPS_CODE_HELLO		1
#define UMSG_REPS_CODE_RESP_HELLO	2

/* for cmd start */
#define UMSG_REPS_CODE_START		1
#define UMSG_REPS_CODE_START_SNAP	2
#define UMSG_REPS_CODE_START_SNAP_DIFF	3
#define UMSG_REPS_CODE_RESP_START	4

/* for cmd display */
#define UMSG_REPS_CODE_S_DISP			1
#define UMSG_REPS_CODE_S_RESP_DISP		2
#define UMSG_REPS_CODE_S_DISP_ALL		3
#define UMSG_REPS_CODE_S_RESP_DISP_ALL		4
#define UMSG_REPS_CODE_W_DISP			5
#define UMSG_REPS_CODE_W_RESP_DISP		6
#define UMSG_REPS_CODE_W_DISP_ALL		7
#define UMSG_REPS_CODE_W_RESP_DISP_ALL		8
#define UMSG_REPS_CODE_DISP_END			9

/* for cmd info */
#define UMSG_REPS_CODE_INFO_PRO		1
#define UMSG_REPS_CODE_RESP_INFO_PRO	2

/* for cmd pause */
#define UMSG_REPS_CODE_PAUSE		1
#define UMSG_REPS_CODE_RESP_PAUSE	2

/* for cmd continue */
#define UMSG_REPS_CODE_CONTINUE		1
#define UMSG_REPS_CODE_RESP_CONTINUE	2

/*
 * items
 */
#define UMSG_REPS_ITEM_REPS		1
#define UMSG_REPS_ITEM_RESP_CODE	2
#define UMSG_REPS_ITEM_REPT		3
#define UMSG_REPS_ITEM_DXA		4
#define UMSG_REPS_ITEM_VOLUUID		5
#define UMSG_REPS_ITEM_SNAP		6
#define UMSG_REPS_ITEM_BITMAP		7
#define UMSG_REPS_ITEM_VOL		8
#define UMSG_REPS_ITEM_SNAP2		9
#define UMSG_REPS_ITEM_PROGRESS		10

#define UMSG_REPS_HEADER_LEN		6

struct umessage_reps_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;
};

struct umessage_reps_hello {
	struct umessage_reps_hdr	um_header;
};

struct umessage_reps_start {
	struct umessage_reps_hdr	um_header;
	uint16_t			um_rs_name_len;
	char				um_rs_name[NID_MAX_UUID];
};

struct umessage_reps_start_snap {
	struct umessage_reps_hdr	um_header;
	uint16_t			um_rs_name_len;
	char				um_rs_name[NID_MAX_UUID];
	uint16_t			um_sp_name_len;
	char				um_sp_name[NID_MAX_UUID];

};

struct umessage_reps_start_snap_diff {
	struct umessage_reps_hdr	um_header;
	uint16_t			um_rs_name_len;
	char				um_rs_name[NID_MAX_UUID];
	uint16_t			um_sp_name_len;
	char				um_sp_name[NID_MAX_UUID];
	uint16_t			um_sp_name2_len;
	char				um_sp_name2[NID_MAX_UUID];
};

struct umessage_reps_pause {
	struct umessage_reps_hdr	um_header;
	uint16_t			um_rs_name_len;
	char				um_rs_name[NID_MAX_UUID];
};

struct umessage_reps_continue {
	struct umessage_reps_hdr	um_header;
	uint16_t			um_rs_name_len;
	char				um_rs_name[NID_MAX_UUID];
};

struct umessage_reps_display {
	struct umessage_reps_hdr	um_header;
	uint16_t			um_rs_name_len;
	char				um_rs_name[NID_MAX_UUID];
};

struct umessage_reps_info {
	struct umessage_reps_hdr	um_header;
	uint16_t			um_rs_name_len;
	char				um_rs_name[NID_MAX_UUID];
};

struct umessage_reps_start_resp {
	struct umessage_reps_hdr	um_header;
	uint8_t				um_resp_code;
};

struct umessage_reps_pause_resp {
	struct umessage_reps_hdr	um_header;
	uint8_t				um_resp_code;
};

struct umessage_reps_continue_resp {
	struct umessage_reps_hdr	um_header;
	uint8_t				um_resp_code;
};

struct umessage_reps_display_resp {
	struct umessage_reps_hdr	um_header;
	uint16_t			um_rs_name_len;
	char				um_rs_name[NID_MAX_UUID];
	uint16_t			um_rt_name_len;
	char				um_rt_name[NID_MAX_UUID];
	uint16_t			um_dxa_name_len;
	char				um_dxa_name[NID_MAX_UUID];
	uint16_t			um_voluuid_len;
	char				um_voluuid[NID_MAX_UUID];
	uint16_t			um_snapuuid_len;
	char				um_snapuuid[NID_MAX_UUID];
	uint16_t			um_snapuuid2_len;
	char				um_snapuuid2[NID_MAX_UUID];
	size_t				um_bitmap_len;
	size_t				um_vol_len;
};

struct umessage_reps_info_pro_resp {
	struct umessage_reps_hdr	um_header;
	float				um_progress;
};

union umessage_reps {
	struct umessage_reps_hello		um_hello;
	struct umessage_reps_start		um_start;
	struct umessage_reps_start_snap		um_start_snap;
	struct umessage_reps_start_snap_diff	um_start_snap_diff;
	struct umessage_reps_pause		um_pause;
	struct umessage_reps_continue		um_continue;
	struct umessage_reps_display		um_disp;
	struct umessage_reps_info		um_info;
};

#endif
