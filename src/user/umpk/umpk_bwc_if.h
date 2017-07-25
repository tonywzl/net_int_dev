/*
 * umpk_bwc_if.h
 * 	umpk for bwc
 */

#ifndef _NID_UMPK_BWC_IF_H
#define _NID_UMPK_BWC_IF_H

#include "nid_shared.h"

/*
 * request cmd
 */
#define UMSG_BWC_CMD_DROPCACHE			1
#define UMSG_BWC_CMD_INFORMATION		2
#define UMSG_BWC_CMD_THROUGHPUT			3
#define UMSG_BWC_CMD_ADD			4
#define UMSG_BWC_CMD_REMOVE			5
#define UMSG_BWC_CMD_FFLUSH			6
#define UMSG_BWC_CMD_RECOVER			7
#define UMSG_BWC_CMD_SNAPSHOT			8
#define UMSG_BWC_CMD_UPDATE_WATER_MARK		9
#define UMSG_BWC_CMD_IOINFO			10
#define UMSG_BWC_CMD_UPDATE_DELAY		11
#define UMSG_BWC_CMD_FLUSH_EMPTY		12
#define UMSG_BWC_CMD_DISPLAY			13
#define UMSG_BWC_CMD_HELLO			14

/*
 * request code
 */
#define UMSG_BWC_CODE_NO			0

/* for cmd dc */
#define UMSG_BWC_CODE_START			1
#define UMSG_BWC_CODE_START_SYNC		2
#define UMSG_BWC_CODE_STOP			3

/* for cmd info */
#define UMSG_BWC_CODE_FLUSHING			1
#define UMSG_BWC_CODE_STAT			3
#define UMSG_BWC_CODE_RESP_STAT			4
#define UMSG_BWC_CODE_THROUGHPUT_STAT		5
#define UMSG_BWC_CODE_THROUGHPUT_STAT_RSLT	6
#define UMSG_BWC_CODE_RW_STAT			7
#define UMSG_BWC_CODE_RW_STAT_RSLT		8
#define UMSG_BWC_CODE_DELAY_STAT		9
#define UMSG_BWC_CODE_RESP_DELAY_STAT		10
#define UMSG_BWC_CODE_LIST_STAT			11
#define UMSG_BWC_CODE_LIST_RESP_STAT		12

/* for cmd throughput */
#define UMSG_BWC_CODE_THROUGHPUT_RESET		1

/* for cmd add */
#define UMSG_BWC_CODE_ADD			1
#define UMSG_BWC_CODE_RESP_ADD			2

/* for cmd update water mark*/
#define UMSG_BWC_CODE_UPDATE_WATER_MARK		1
#define UMSG_BWC_CODE_RESP_UPDATE_WATER_MARK	2

/* for cmd remove */
#define UMSG_BWC_CODE_REMOVE			1
#define UMSG_BWC_CODE_RESP_REMOVE		2

/* for cmd fflush */
#define UMSG_BWC_CODE_FF_START			1
#define UMSG_BWC_CODE_FF_GET			2
#define UMSG_BWC_CODE_FF_STOP			3
#define UMSG_BWC_CODE_RESP_FF_GET		4

/* for cmd recover */
#define UMSG_BWC_CODE_RCV_START			1
#define UMSG_BWC_CODE_RCV_GET			2
#define UMSG_BWC_CODE_RESP_RCV_GET		3

/* for cmd snapshot */
#define UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_1		1
#define UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_2		2
#define UMSG_BWC_CODE_SNAPSHOT_UNFREEZE				3
#define UMSG_BWC_CODE_SNAPSHOT_FREEZE_RESP			4
#define UMSG_BWC_CODE_SNAPSHOT_UNFREEZE_RESP		5

/* for cmd ioinfo */
#define UMSG_BWC_CODE_IOINFO_START			1
#define UMSG_BWC_CODE_RESP_IOINFO_START			2
#define UMSG_BWC_CODE_IOINFO_STOP			3
#define UMSG_BWC_CODE_RESP_IOINFO_STOP			4
#define UMSG_BWC_CODE_IOINFO_CHECK			5
#define UMSG_BWC_CODE_RESP_IOINFO_CHECK			6
#define UMSG_BWC_CODE_IOINFO_START_ALL			7
#define UMSG_BWC_CODE_RESP_IOINFO_START_ALL		8
#define UMSG_BWC_CODE_IOINFO_STOP_ALL			9
#define UMSG_BWC_CODE_RESP_IOINFO_STOP_ALL		10
#define UMSG_BWC_CODE_IOINFO_CHECK_ALL			11
#define UMSG_BWC_CODE_RESP_IOINFO_CHECK_ALL		12
#define UMSG_BWC_CODE_IOINFO_BFE_START			13
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_START		14
#define UMSG_BWC_CODE_IOINFO_BFE_STOP			15
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP		16
#define UMSG_BWC_CODE_IOINFO_BFE_CHECK			17
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK		18
#define UMSG_BWC_CODE_IOINFO_BFE_CHAIN_START_ALL	19
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_START_ALL	20
#define UMSG_BWC_CODE_IOINFO_BFE_CHAIN_STOP_ALL		21
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_STOP_ALL	22
#define UMSG_BWC_CODE_IOINFO_BFE_CHAIN_CHECK_ALL	23
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_CHAIN_CHECK_ALL	24
#define UMSG_BWC_CODE_IOINFO_BFE_START_ALL		25
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_START_ALL		26
#define UMSG_BWC_CODE_IOINFO_BFE_STOP_ALL		27
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_STOP_ALL		28
#define UMSG_BWC_CODE_IOINFO_BFE_CHECK_ALL		29
#define UMSG_BWC_CODE_RESP_IOINFO_BFE_CHECK_ALL		30
#define UMSG_BWC_CODE_RESP_IOINFO_END			31

/* for cmd update delay */
#define UMSG_BWC_CODE_UPDATE_DELAY		1
#define UMSG_BWC_CODE_RESP_UPDATE_DELAY		2

/* for cmd flush empty */
#define UMSG_BWC_CODE_FLUSH_EMPTY_START		1
#define UMSG_BWC_CODE_RESP_FLUSH_EMPTY_START	2
#define UMSG_BWC_CODE_FLUSH_EMPTY_STOP		3
#define UMSG_BWC_CODE_RESP_FLUSH_EMPTY_STOP	4

/* for cmd display */
#define UMSG_BWC_CODE_S_DISP			1
#define UMSG_BWC_CODE_S_RESP_DISP		2
#define UMSG_BWC_CODE_S_DISP_ALL		3
#define UMSG_BWC_CODE_S_RESP_DISP_ALL		4
#define UMSG_BWC_CODE_W_DISP			5
#define UMSG_BWC_CODE_W_RESP_DISP		6
#define UMSG_BWC_CODE_W_DISP_ALL		7
#define UMSG_BWC_CODE_W_RESP_DISP_ALL		8
#define UMSG_BWC_CODE_DISP_END			9

/* for cmd hello */
#define UMSG_BWC_CODE_HELLO			1
#define UMSG_BWC_CODE_RESP_HELLO		2

/*
 * items
 */
#define UMSG_BWC_ITEM_CHANNEL			1
#define UMSG_BWC_ITEM_CACHE			2
#define UMSG_BWC_ITEM_SEQ_ASSIGNED		3
#define UMSG_BWC_ITEM_SEQ_FLUSHED		4
#define UMSG_BWC_ITEM_BLOCK_OCCUPIED		5
#define UMSG_BWC_ITEM_BLOCK_WRITE		6
#define UMSG_BWC_ITEM_BLOCK_FLUSH		7
#define UMSG_BWC_ITEM_DATA_FLUSHED		8
#define UMSG_BWC_ITEM_DATA_PKG_FLUSHED		9
#define UMSG_BWC_ITEM_NONDATA_PKG_FLUSHED	10
#define UMSG_BWC_ITEM_OVERWRITTEN_NUM		11
#define UMSG_BWC_ITEM_OVERWRITTEN_BACK_NUM	12
#define UMSG_BWC_ITEM_COALESCE_NUM		13
#define UMSG_BWC_ITEM_COALESCE_BACK_NUM		14
#define UMSG_BWC_ITEM_FLUSH_NUM			15
#define UMSG_BWC_ITEM_FLUSH_BACK_NUM		16
#define UMSG_BWC_ITEM_FLUSH_PAGE_NUM		17
#define UMSG_BWC_ITEM_NOT_READY_NUM		18
#define UMSG_BWC_ITEM_RES_NUM			19
#define UMSG_BWC_ITEM_ADD_RSLT			20
#define UMSG_BWC_ITEM_REMOVE_RSLT		21
#define UMSG_BWC_ITEM_FF_STATE			22
#define UMSG_BWC_ITEM_RCV_STATE			23
#define UMSG_BWC_ITEM_SNAPSHOT			24
#define	UMSG_BWC_ITEM_TP			25
#define UMSG_BWC_ITEM_RESP_CONTER		26
#define UMSG_BWC_ITEM_BUFDEV			27
#define UMSG_BWC_ITEM_SETUP			28
#define UMSG_BWC_ITEM_NBLOCKS			29
#define UMSG_BWC_ITEM_RECFLUSHED		30
#define UMSG_BWC_ITEM_HIGH_WATER_MARK		31
#define UMSG_BWC_ITEM_LOW_WATER_MARK		32
#define UMSG_BWC_ITEM_WATER_MARK_RSLT		33
#define UMSG_BWC_ITEM_IOINFO			34
#define UMSG_BWC_ITEM_STATE			35
#define UMSG_BWC_ITEM_BUF_AVAIL			36
#define UMSG_BWC_ITEM_DELAY_FIRST_LEVEL		37
#define UMSG_BWC_ITEM_DELAY_SECOND_LEVEL	38
#define UMSG_BWC_ITEM_DELAY_FIRST_LEVEL_MAX	39
#define UMSG_BWC_ITEM_DELAY_SECOND_LEVEL_MAX	40
#define UMSG_BWC_ITEM_RESP_CODE			41
#define UMSG_BWC_ITEM_DELAY_FIRST_INTERVAL	42
#define UMSG_BWC_ITEM_DELAY_SECOND_INTERVAL	43
#define UMSG_BWC_ITEM_DELAY_SECOND_START	44
#define UMSG_BWC_ITEM_DELAY_TIME		45
#define UMSG_BWC_ITEM_TO_WRITE_COUNTER		46
#define UMSG_BWC_ITEM_TO_READ_COUNTER		47
#define UMSG_BWC_ITEM_WRITE_COUNTER		48
#define UMSG_BWC_ITEM_WRITE_VEC_COUNTER		49
#define UMSG_BWC_ITEM_WRITE_BUSY		50
#define UMSG_BWC_ITEM_READ_BUSY			51
#define UMSG_BWC_ITEM_FOUND_IT			52

#define UMSG_BWC_HEADER_LEN			6

struct umessage_bwc_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint32_t	um_len;		// the whole message len, including the header
};

struct umessage_bwc_dropcache {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_bwc_information {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_bwc_information_list_stat {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_bwc_throughput {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
};


struct umessage_bwc_add {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t 		um_bufdev_len;
	char			um_bufdev[NID_MAX_PATH];
	uint32_t		um_bufdev_sz;
	uint8_t			um_rw_sync;
	uint8_t			um_two_step_read;
	uint8_t			um_do_fp;
	uint8_t			um_bfp_type;
	double			um_coalesce_ratio;
	double  		um_load_ratio_max;
	double  		um_load_ratio_min;
	double  		um_load_ctrl_level;
	double  		um_flush_delay_ctl;
	double			um_throttle_ratio;
	uint8_t			um_tp_name_len;
	char			um_tp_name[NID_MAX_TPNAME];
	uint8_t			um_max_flush_size;
	uint8_t			um_ssd_mode;
	uint16_t		um_write_delay_first_level;
	uint16_t		um_write_delay_second_level;
	uint16_t		um_high_water_mark;
	uint16_t		um_low_water_mark;
};


struct umessage_bwc_water_mark {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_high_water_mark;
	uint16_t		um_low_water_mark;
};

struct umessage_bwc_remove {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
};

struct umessage_bwc_fflush {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_bwc_snapshot {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_bwc_recover {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
};

struct umessage_bwc_delay {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_write_delay_first_level;
	uint16_t		um_write_delay_second_level;
	uint32_t		um_write_delay_first_level_max;
	uint32_t		um_write_delay_second_level_max;
};

struct umessage_bwc_ioinfo {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
};

struct umessage_bwc_flush_empty {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
};

struct umessage_bwc_display {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
};

struct umessage_bwc_hello {
	struct umessage_bwc_hdr	um_header;
};

struct umessage_bwc_information_resp_throughput_stat {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint64_t		um_seq_nondata_pkg_flushed;
	uint64_t		um_seq_data_flushed;
	uint64_t		um_seq_data_pkg_flushed;
};

struct umessage_bwc_information_resp_rw_stat {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint64_t		um_seq_overwritten_counter;
	uint64_t		um_seq_overwritten_back_counter;
	uint64_t		um_seq_coalesce_flush_num;
	uint64_t		um_seq_flush_num;
	uint64_t		um_seq_coalesce_flush_back_num;
	uint64_t		um_seq_flush_back_num;
	uint64_t		res;
	uint64_t		um_seq_flush_page_num;
	uint64_t		um_seq_not_ready_num;
};

struct umessage_bwc_information_resp_flushing {
	struct umessage_bwc_hdr	um_header;
};

struct umessage_bwc_information_resp_stat {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_block_occupied;
	uint16_t		um_flush_nblocks;
	uint16_t		um_cur_write_block;
	uint16_t		um_cur_flush_block;
	uint64_t		um_seq_assigned;
	uint64_t		um_seq_flushed;
	uint64_t		um_resp_counter;
	uint64_t		um_rec_seq_flushed;
	uint8_t			um_state;
	uint32_t		um_buf_avail;
};

struct umessage_bwc_information_resp_delay_stat {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_write_delay_first_level;
	uint16_t		um_write_delay_second_level;
	uint32_t		um_write_delay_first_level_max;
	uint32_t		um_write_delay_second_level_max;
	uint32_t		um_write_delay_first_level_increase_interval;
	uint32_t		um_write_delay_second_level_increase_interval;
	uint32_t		um_write_delay_second_level_start;
	uint32_t		um_write_delay_time;
};

struct umessage_bwc_add_resp {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_bwc_water_mark_resp {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_bwc_remove_resp {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_bwc_snapshot_resp {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	int8_t			um_resp_code;
};

struct umessage_bwc_fflush_resp_get {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t			um_ff_state;
};

struct umessage_bwc_recover_resp_get {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint8_t			um_rcv_state;
};

struct umessage_bwc_ioinfo_resp {
	struct umessage_bwc_hdr um_header;
	uint16_t                um_bwc_uuid_len;
	char                    um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint8_t                 um_is_running;
	uint64_t                um_req_num;
	uint64_t                um_req_len;
};

struct umessage_bwc_delay_resp {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_bwc_flush_empty_resp {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint8_t			um_resp_code;
};

struct umessage_bwc_display_resp {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t 		um_bufdev_len;
	char			um_bufdev[NID_MAX_PATH];
	uint8_t			um_tp_name_len;
	char			um_tp_name[NID_MAX_TPNAME];
	uint32_t		um_bufdev_sz;
	uint8_t			um_rw_sync;
	uint8_t			um_two_step_read;
	uint8_t			um_do_fp;
	uint8_t			um_bfp_type;
	double			um_coalesce_ratio;
	double  		um_load_ratio_max;
	double  		um_load_ratio_min;
	double  		um_load_ctrl_level;
	double  		um_flush_delay_ctl;
	double			um_throttle_ratio;
	uint8_t			um_ssd_mode;
	uint8_t			um_max_flush_size;
	uint16_t		um_write_delay_first_level;
	uint16_t		um_write_delay_second_level;
	uint32_t		um_write_delay_first_level_max;
	uint32_t		um_write_delay_second_level_max;
	uint16_t		um_low_water_mark;
	uint16_t		um_high_water_mark;
};
struct umessage_bwc_information_list_resp_stat {
	struct umessage_bwc_hdr	um_header;
	uint16_t		um_bwc_uuid_len;
	char			um_bwc_uuid[NID_MAX_UUID];
	uint16_t		um_chan_uuid_len;
	char			um_chan_uuid[NID_MAX_UUID];
	uint32_t		um_to_write_list_counter;
	uint32_t		um_to_read_list_counter;
	uint32_t		um_write_list_counter;
	uint32_t		um_write_vec_list_counter;
	uint8_t			um_write_busy;
	uint8_t			um_read_busy;
	uint8_t			um_found_it;

};

union umessage_bwc {
	struct umessage_bwc_dropcache	um_dc;
	struct umessage_bwc_information	um_info;
	struct umessage_bwc_throughput	um_tgt;
	struct umessage_bwc_add		um_add;
	struct umessage_bwc_remove	um_rm;
	struct umessage_bwc_fflush	um_ff;
	struct umessage_bwc_recover	um_rcv;
	struct umessage_bwc_water_mark	um_wm;
	struct umessage_bwc_ioinfo	um_ioinfo;
	struct umessage_bwc_delay	um_delay;
	struct umessage_bwc_flush_empty	um_fe;
	struct umessage_bwc_display	um_display;
	struct umessage_bwc_hello	um_hello;
};

#if 0
union umessage_bwc_response {
	struct umessage_bwc_information_resp_flushing	info_flushing;
	struct umessage_bwc_information_resp_stat	info_stat;
	struct umessage_bwc_fflush_resp_get		ffush_get;
	struct umessage_bwc_recover_resp_get		recover_get;
};
#endif
#endif
