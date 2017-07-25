/*
 * umpk_ini_if.h
 *	umpk for ini
 */

#ifndef _NID_UMPK_INI_IF_H
#define _NID_UMPK_INI_IF_H

#include "nid_shared.h"

#define INI_MAX_DESC 			128
#define INI_MAX_VAL			128

/*
 * request cmd
 */
#define UMSG_INI_CMD_DISPLAY		1
#define UMSG_INI_CMD_HELLO		2

/*
 * request code
 */
#define UMSG_INI_CODE_NO			0

/* for cmd display */
#define UMSG_INI_CODE_T_SECTION			1
#define UMSG_INI_CODE_T_RESP_SECTION		2
#define UMSG_INI_CODE_T_SECTION_DETAIL		3
#define UMSG_INI_CODE_T_RESP_SECTION_DETAIL	4
#define UMSG_INI_CODE_T_ALL			5
#define UMSG_INI_CODE_T_RESP_ALL		6
#define UMSG_INI_CODE_C_SECTION			7
#define UMSG_INI_CODE_C_RESP_SECTION		8
#define UMSG_INI_CODE_C_SECTION_DETAIL		9
#define UMSG_INI_CODE_C_RESP_SECTION_DETAIL	10
#define UMSG_INI_CODE_C_ALL			11
#define UMSG_INI_CODE_C_RESP_ALL		12
#define UMSG_INI_CODE_RESP_END			13

/* for cmd hello */
#define UMSG_INI_CODE_HELLO			1
#define UMSG_INI_CODE_RESP_HELLO		2

/*
 * items
 */
#define UMSG_INI_ITEM_KEY		1
#define UMSG_INI_ITEM_DESC		2
#define UMSG_INI_ITEM_VAL		3

#define UMSG_INI_HEADER_LEN		6

struct umessage_ini_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;
};

struct umessage_ini_display {
	struct umessage_ini_hdr um_header;
	uint16_t		um_key_len;
	char			um_key[NID_MAX_UUID];
};

struct umessage_ini_resp_display {
	struct umessage_ini_hdr	um_header;
	uint16_t		um_key_len;
	char			um_key[NID_MAX_UUID];
	uint16_t		um_desc_len;
	char			um_desc[INI_MAX_DESC];
	uint16_t		um_value_len;
	char			um_value[INI_MAX_VAL];
};

struct umessage_ini_hello {
	struct umessage_ini_hdr um_header;
};

union umessage_ini {
	struct umessage_ini_display	um_display;
	struct umessage_ini_hello	um_hello;
};

#endif
