/*
 * umpk_sac_if.h
 * 	umpk for sac
 */

#ifndef _NID_UMPK_SAC_IF_H
#define _NID_UMPK_SAC_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_SAC_CMD_INFORMATION		1
#define UMSG_SAC_CMD_ADD			2
#define UMSG_SAC_CMD_DELETE			3
#define UMSG_SAC_CMD_SWITCH			4
#define UMSG_SAC_CMD_SET_KEEPALIVE		5
#define UMSG_SAC_CMD_FAST_RELEASE		6
#define UMSG_SAC_CMD_IOINFO			7
#define UMSG_SAC_CMD_DISPLAY			8
#define	UMSG_SAC_CMD_HELLO			9

/*
 * request code
 */
#define UMSG_SAC_CODE_NO			0

/* for cmd info */
#define UMSG_SAC_CODE_STAT			1
#define UMSG_SAC_CODE_RESP_STAT			2
#define UMSG_SAC_CODE_STAT_ALL			3
#define UMSG_SAC_CODE_RESP_STAT_ALL		4
#define UMSG_SAC_CODE_STAT_END			5
#define UMSG_SAC_CODE_LIST_STAT			6
#define UMSG_SAC_CODE_RESP_LIST_STAT		7


/* for cmd add */
#define UMSG_SAC_CODE_ADD			1
#define UMSG_SAC_CODE_RESP_ADD			2

/* for cmd del */
#define UMSG_SAC_CODE_DELETE			1
#define UMSG_SAC_CODE_RESP_DELETE		2

/* for cmd swm */
#define UMSG_SAC_CODE_SWITCH			1
#define UMSG_SAC_CODE_RESP_SWITCH		2

/* for cmd set keepalive */
#define UMSG_SAC_CODE_SET_KEEPALIVE		1
#define UMSG_SAC_CODE_RESP_SET_KEEPALIVE	2

/* for cmd fast flush */
#define UMSG_SAC_CODE_START			1
#define UMSG_SAC_CODE_RESP_START		2
#define UMSG_SAC_CODE_STOP			3
#define UMSG_SAC_CODE_RESP_STOP			4

/* for cmd ioinfo */
#define UMSG_SAC_CODE_IOINFO_START		1
#define UMSG_SAC_CODE_RESP_IOINFO_START		2
#define UMSG_SAC_CODE_IOINFO_START_ALL		3
#define UMSG_SAC_CODE_RESP_IOINFO_START_ALL	4
#define UMSG_SAC_CODE_IOINFO_STOP		5
#define UMSG_SAC_CODE_RESP_IOINFO_STOP		6
#define UMSG_SAC_CODE_IOINFO_STOP_ALL		7
#define UMSG_SAC_CODE_RESP_IOINFO_STOP_ALL	8
#define UMSG_SAC_CODE_IOINFO_CHECK		9
#define UMSG_SAC_CODE_RESP_IOINFO_CHECK		10
#define UMSG_SAC_CODE_IOINFO_CHECK_ALL		11
#define UMSG_SAC_CODE_RESP_IOINFO_CHECK_ALL	12
#define UMSG_SAC_CODE_RESP_IOINFO_END		13

/* for cmd display */
#define UMSG_SAC_CODE_S_DISP			1
#define UMSG_SAC_CODE_S_RESP_DISP		2
#define UMSG_SAC_CODE_S_DISP_ALL		3
#define UMSG_SAC_CODE_S_RESP_DISP_ALL		4
#define UMSG_SAC_CODE_W_DISP			5
#define UMSG_SAC_CODE_W_RESP_DISP		6
#define UMSG_SAC_CODE_W_DISP_ALL		7
#define UMSG_SAC_CODE_W_RESP_DISP_ALL		8
#define UMSG_SAC_CODE_RESP_END			9

/* for cmd hello */
#define UMSG_SAC_CODE_HELLO			1
#define UMSG_SAC_CODE_RESP_HELLO		2

/*
 * items
 */
#define UMSG_SAC_ITEM_CHANNEL			1
#define UMSG_SAC_ITEM_SYNC			2
#define UMSG_SAC_ITEM_DIRECT_IO			3
#define	UMSG_SAC_ITEM_ALIGNMENT			4
#define	UMSG_SAC_ITEM_DS			5
#define	UMSG_SAC_ITEM_DEV			6
#define	UMSG_SAC_ITEM_BWC			7
#define	UMSG_SAC_ITEM_TP			8
#define	UMSG_SAC_ITEM_RESP_CODE			9
#define UMSG_SAC_ITEM_WC			10
#define UMSG_SAC_ITEM_RC			11
#define UMSG_SAC_ITEM_IO_TYPE			12
#define UMSG_SAC_ITEM_DS_TYPE			13
#define UMSG_SAC_ITEM_WC_TYPE			14
#define UMSG_SAC_ITEM_RC_TYPE			15
#define UMSG_SAC_ITEM_READY			16
#define UMSG_SAC_ITEM_BFE_COUNTER		17
#define UMSG_SAC_ITEM_BRE_COUNTER		18
#define UMSG_SAC_ITEM_IOINFO			19
#define UMSG_SAC_ITEM_IP			20
#define UMSG_SAC_ITEM_ALEVEL			21
#define UMSG_SAC_ITEM_RCOUNTER			22
#define UMSG_SAC_ITEM_RREADYCOUNTER		23
#define UMSG_SAC_ITEM_WCOUNTER			24
#define UMSG_SAC_ITEM_KCOUNTER			25	// keepalive counter
#define UMSG_SAC_ITEM_RRCOUNTER			26	// resp read counter
#define UMSG_SAC_ITEM_RWCOUNTER			27	// resp write counter
#define UMSG_SAC_ITEM_RKCOUNTER			28	// resp keepalive counter
#define UMSG_SAC_ITEM_STATE			29
#define UMSG_SAC_ITEM_TIME			30	//
#define UMSG_SAC_ITEM_RSEQUENCE			31	// recved sequence
#define UMSG_SAC_ITEM_WSEQUENCE			32	// waiting for sequence
#define UMSG_SAC_ITEM_WREADYCOUNTER		33
#define UMSG_SAC_ITEM_SRVSTATUS			34
#define UMSG_SAC_ITEM_RTREE_SEGSZ		35
#define UMSG_SAC_ITEM_RTREE_NSEG		36
#define UMSG_SAC_ITEM_RTREE_NFREE		37
#define UMSG_SAC_ITEM_RTREE_NUSED		38
#define UMSG_SAC_ITEM_BTN_SEGSZ			39
#define UMSG_SAC_ITEM_BTN_NSEG			40
#define UMSG_SAC_ITEM_BTN_NFREE			41
#define UMSG_SAC_ITEM_BTN_NUSED			42
#define UMSG_SAC_ITEM_ENABLE_KILL_MYSELF	43
#define UMSG_SAC_ITEM_REQ_COUNTER		44
#define UMSG_SAC_ITEM_TO_REQ_COUNTER		45
#define UMSG_SAC_ITEM_TO_RCOUNTER		46
#define UMSG_SAC_ITEM_TO_WCOUNTER		47
#define UMSG_SAC_ITEM_RESP_COUNTER		48
#define UMSG_SAC_ITEM_TO_RESP_COUNTER		49
#define UMSG_SAC_ITEM_DRESP_COUNTER		50
#define UMSG_SAC_ITEM_TO_DRESP_COUNTER		51

#define UMSG_SAC_HEADER_LEN			6

struct umessage_sac_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_sac_information {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_sac_add {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t			um_sync;
	uint8_t			um_direct_io;
	uint8_t			um_enable_kill_myself;
	uint32_t		um_alignment;
	uint8_t			um_ds_name_len;
	char			um_ds_name[NID_MAX_DSNAME];
	uint8_t			um_dev_name_len;
	char			um_dev_name[NID_MAX_DEVNAME];
	uint16_t		um_exportname_len;
	char			um_exportname[NID_MAX_UUID];
	uint32_t		um_dev_size;
	uint8_t			um_tp_name_len;
	char			um_tp_name[NID_MAX_TPNAME];
};

struct umessage_sac_delete {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_sac_switch {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
};

struct umessage_sac_set_keepalive {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint16_t		um_enable_keepalive;
	uint16_t		um_max_keepalive;
};

struct umessage_sac_fast_release {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_sac_ioinfo {
	struct umessage_sac_hdr	um_header;
	uint16_t                um_chan_uuid_len;
	char                    um_chan_uuid[NID_MAX_UUID];
};

struct umessage_sac_display {
	struct umessage_sac_hdr	um_header;
	uint16_t                um_chan_uuid_len;
	char                    um_chan_uuid[NID_MAX_UUID];
};

struct umessage_sac_information_resp {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t			um_ip_len;
	char			um_ip[NID_MAX_IP];
	uint32_t		um_bfe_page_counter;
	uint32_t		um_bre_page_counter;
	uint8_t			um_state;
	uint8_t			um_alevel;
	uint64_t		um_rcounter;
	uint64_t		um_r_rcounter;
	uint64_t		um_rreadycounter;
	uint64_t		um_wcounter;
	uint64_t		um_wreadycounter;
	uint64_t		um_r_wcounter;
	uint64_t		um_kcounter;
	uint64_t		um_r_kcounter;
	uint64_t		um_recv_sequence;
	uint64_t		um_wait_sequence;
	uint32_t		um_live_time;
	uint32_t		um_rtree_segsz;
	uint32_t		um_rtree_nseg;
	uint32_t		um_rtree_nfree;
	uint32_t		um_rtree_nused;
	uint32_t		um_btn_segsz;
	uint32_t		um_btn_nseg;
	uint32_t		um_btn_nfree;
	uint32_t		um_btn_nused;
};

struct umessage_sac_list_stat_resp {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint32_t		um_req_counter;
	uint32_t		um_to_req_counter;
	uint32_t		um_rcounter;
	uint32_t		um_to_rcounter;
	uint32_t		um_wcounter;
	uint32_t		um_to_wcounter;
	uint32_t		um_resp_counter;
	uint32_t		um_to_resp_counter;
	uint32_t		um_dresp_counter;
	uint32_t		um_to_dresp_counter;
};

struct umessage_sac_add_resp {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_sac_delete_resp {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_sac_switch_resp {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_sac_set_keepalive_resp {
	struct umessage_sac_hdr	um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_sac_ioinfo_resp {
	struct umessage_sac_hdr um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t			um_is_running;
	uint64_t		um_req_num;
	uint64_t		um_req_len;
};

struct umessage_sac_display_resp {
	struct umessage_sac_hdr um_header;
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	char			um_sync;
	char			um_direct_io;
	char			um_enable_kill_myself;
	int			um_alignment;
	uint8_t			um_ds_name_len;
	char			um_ds_name[NID_MAX_DSNAME];
	uint8_t			um_dev_name_len;
	char			um_dev_name[NID_MAX_DEVNAME];
	uint16_t		um_export_name_len;
	char			um_export_name[NID_MAX_PATH];
	uint32_t		um_dev_size;
	uint8_t			um_tp_name_len;
	char			um_tp_name[NID_MAX_TPNAME];
	uint16_t		um_wc_uuid_len;
	char			um_wc_uuid[NID_MAX_UUID];
	uint16_t		um_rc_uuid_len;
	char			um_rc_uuid[NID_MAX_UUID];
	int			um_io_type;
	int			um_ds_type;
	int			um_wc_type;
	int			um_rc_type;
	uint8_t			um_ready;
};

struct umessage_sac_fast_release_resp {
	struct umessage_sac_hdr	um_header;
	uint8_t			um_resp_code;
};

struct umessage_sac_hello {
	struct umessage_sac_hdr um_header;
};

union umessage_sac {
	struct umessage_sac_information		um_info;
	struct umessage_sac_add			um_add;
	struct umessage_sac_delete		um_del;
	struct umessage_sac_switch		um_swm;
	struct umessage_sac_set_keepalive	um_kepp;
	struct umessage_sac_fast_release	um_fr;
	struct umessage_sac_ioinfo		um_ioinfo;
	struct umessage_sac_display		um_display;
	struct umessage_sac_hello 		um_hello;
};

#endif
