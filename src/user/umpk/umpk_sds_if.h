/*
 * umpk_sds_if.h
 * 	umpk for sds
 */

#ifndef _NID_UMPK_SDS_IF_H
#define _NID_UMPK_SDS_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_SDS_CMD_ADD			1
#define UMSG_SDS_CMD_DELETE			3
#define UMSG_SDS_CMD_DISPLAY			4
#define UMSG_SDS_CMD_HELLO			5

/*
 * request code
 */
#define UMSG_SDS_CODE_NO			0

/* for cmd add */
#define UMSG_SDS_CODE_ADD			1
#define UMSG_SDS_CODE_RESP_ADD			2

/* for cmd delete */
#define UMSG_SDS_CODE_DELETE			1
#define UMSG_SDS_CODE_RESP_DELETE		2

/* for cmd hello */
#define UMSG_SDS_CODE_HELLO			1
#define UMSG_SDS_CODE_RESP_HELLO		2

/* for cmd display */
#define UMSG_SDS_CODE_S_DISP			1
#define UMSG_SDS_CODE_S_RESP_DISP		2
#define UMSG_SDS_CODE_S_DISP_ALL		3
#define UMSG_SDS_CODE_S_RESP_DISP_ALL		4
#define UMSG_SDS_CODE_W_DISP			5
#define UMSG_SDS_CODE_W_RESP_DISP		6
#define UMSG_SDS_CODE_W_DISP_ALL		7
#define UMSG_SDS_CODE_W_RESP_DISP_ALL		8
#define UMSG_SDS_CODE_DISP_END			9

/*
 * items
 */
#define	UMSG_SDS_ITEM_SDS			1
#define	UMSG_SDS_ITEM_PP			2
#define	UMSG_SDS_ITEM_WC			3
#define	UMSG_SDS_ITEM_RC			4
#define	UMSG_SDS_ITEM_RESP_CODE			5

#define UMSG_SDS_HEADER_LEN			6

struct umessage_sds_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_sds_add {
	struct umessage_sds_hdr	um_header;
	uint8_t			um_sds_name_len;
	char			um_sds_name[NID_MAX_DSNAME];
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_PPNAME];
	uint16_t		um_wc_uuid_len;
	char			um_wc_uuid[NID_MAX_UUID];
	uint16_t		um_rc_uuid_len;
	char			um_rc_uuid[NID_MAX_UUID];
};

struct umessage_sds_add_resp {
	struct umessage_sds_hdr	um_header;
	uint8_t			um_sds_name_len;
	char			um_sds_name[NID_MAX_DSNAME];
	uint8_t			um_resp_code;
};

struct umessage_sds_delete {
	struct umessage_sds_hdr	um_header;
	uint8_t			um_sds_name_len;
	char			um_sds_name[NID_MAX_DSNAME];
};

struct umessage_sds_delete_resp {
	struct umessage_sds_hdr	um_header;
	uint8_t			um_sds_name_len;
	char			um_sds_name[NID_MAX_DSNAME];
	uint8_t			um_resp_code;
};

struct umessage_sds_display {
	struct umessage_sds_hdr	um_header;
	uint8_t			um_sds_name_len;
	char			um_sds_name[NID_MAX_DSNAME];
};

struct umessage_sds_display_resp {
	struct umessage_sds_hdr	um_header;
	uint8_t			um_sds_name_len;
	char			um_sds_name[NID_MAX_DSNAME];
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_PPNAME];
	uint16_t		um_wc_uuid_len;
	char			um_wc_uuid[NID_MAX_UUID];
	uint16_t		um_rc_uuid_len;
	char			um_rc_uuid[NID_MAX_UUID];

};

struct umessage_sds_hello {
	struct umessage_sds_hdr	um_header;
};

union umessage_sds {
	struct umessage_sds_add		um_add;
	struct umessage_sds_delete	um_del;
	struct umessage_sds_display	um_display;
	struct umessage_sds_hello	um_hello;
};

#endif
