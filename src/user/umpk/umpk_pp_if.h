/*
 * umpk_pp_if.h
 * 	umpk for pp
 */

#ifndef _NID_UMPK_PP_IF_H
#define _NID_UMPK_PP_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_PP_CMD_ADD			1
#define UMSG_PP_CMD_DELETE		2
#define	UMSG_PP_CMD_STAT		3
#define UMSG_PP_CMD_DISPLAY		4	
#define UMSG_PP_CMD_HELLO		5

/*
 * request code
 */
#define UMSG_PP_CODE_NO			0

/* for cmd add */
#define UMSG_PP_CODE_ADD		1
#define UMSG_PP_CODE_RESP_ADD		2

/* for cmd delete */
#define UMSG_PP_CODE_DELETE		1
#define UMSG_PP_CODE_RESP_DELETE	2

/* for cmd stat */
#define UMSG_PP_CODE_STAT		1
#define UMSG_PP_CODE_RESP_STAT		2
#define UMSG_PP_CODE_STAT_ALL		3
#define UMSG_PP_CODE_RESP_STAT_ALL	4
#define UMSG_PP_CODE_RESP_END		5

/* for cmd display */
#define UMSG_PP_CODE_S_DISP		1
#define UMSG_PP_CODE_S_RESP_DISP	2
#define UMSG_PP_CODE_S_DISP_ALL		3
#define UMSG_PP_CODE_S_RESP_DISP_ALL	4
#define UMSG_PP_CODE_W_DISP		5
#define UMSG_PP_CODE_W_RESP_DISP	6
#define UMSG_PP_CODE_W_DISP_ALL		7
#define UMSG_PP_CODE_W_RESP_DISP_ALL	8
#define UMSG_PP_CODE_DISP_END		9

/* for cmd hello */
#define UMSG_PP_CODE_HELLO		1
#define UMSG_PP_CODE_RESP_HELLO		2

/*
 * items
 */
#define	UMSG_PP_ITEM_NAME			1
#define	UMSG_PP_ITEM_SET			2
#define	UMSG_PP_ITEM_POOL_SZ			3
#define	UMSG_PP_ITEM_PAGE_SZ			4
#define	UMSG_PP_ITEM_RESP_CODE			5
#define UMSG_PP_ITEM_STAT			6

#define UMSG_PP_HEADER_LEN			6

struct umessage_pp_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_pp_add {
	struct umessage_pp_hdr	um_header;
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_PPNAME];
	uint8_t			um_set_id;
	uint32_t		um_pool_size;
	uint32_t		um_page_size;
};

struct umessage_pp_add_resp {
	struct umessage_pp_hdr	um_header;
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_DSNAME];
	uint8_t			um_resp_code;
};

struct umessage_pp_delete {
	struct umessage_pp_hdr	um_header;
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_PPNAME];
};

struct umessage_pp_delete_resp {
	struct umessage_pp_hdr	um_header;
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_DSNAME];
	uint8_t			um_resp_code;
};

struct umessage_pp_stat {
	struct umessage_pp_hdr	um_header;
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_PPNAME];
};

struct umessage_pp_stat_resp {
	struct umessage_pp_hdr 	um_header;
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_PPNAME];
	uint32_t		um_pp_poolsz;
	uint32_t		um_pp_pagesz;
	uint32_t		um_pp_poollen;
	uint32_t		um_pp_nfree;
};

struct umessage_pp_display {
	struct umessage_pp_hdr	um_header;
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_PPNAME];
};

struct umessage_pp_display_resp {
	struct umessage_pp_hdr	um_header;
	uint8_t			um_pp_name_len;
	char			um_pp_name[NID_MAX_PPNAME];
	uint8_t			um_set_id;
	uint32_t		um_pool_size;
	uint32_t		um_page_size;
};

struct umessage_pp_hello {
	struct umessage_pp_hdr	um_header;
};

union umessage_pp {
	struct umessage_pp_add		um_add;
	struct umessage_pp_delete	um_delete;
	struct umessage_pp_stat		um_stat;
	struct umessage_pp_display	um_disp;
	struct umessage_pp_hello	um_hello;
};

#endif
