/*
 * bwc_if.h
 * 	Interface of Writeback Write Cache Module
 */

#include <stdint.h>

#ifndef NID_BWC_IF_H
#define NID_BWC_IF_H

#include <list.h>
#include "nid_shared.h"

struct bwc_throughput_stat;
struct bwc_rw_stat;
struct pp_page_node;
struct io_chan_stat;
struct io_vec_stat;
struct io_stat;
struct bwc_interface;
struct list_head;
struct rw_interface;
struct wc_channel_info;
struct umessage_bwc_information_resp_stat;
struct umessage_bwc_information_list_resp_stat;
struct bwc_req_stat;
struct bwc_delay_stat;

struct bwc_operations {
	void*		(*bw_create_channel)(struct bwc_interface *, void *, struct wc_channel_info *, char *, int *);
	void*		(*bw_prepare_channel)(struct bwc_interface *, struct wc_channel_info *, char *);
	void		(*bw_recover)(struct bwc_interface *);
	uint32_t	(*bw_get_poolsz)(struct bwc_interface *);
	uint32_t	(*bw_get_pagesz)(struct bwc_interface *);
	ssize_t		(*bw_pread)(struct bwc_interface *, void *, void *, size_t, off_t);
	void		(*bw_read_list)(struct bwc_interface *, void *, struct list_head *, int);
	void		(*bw_write_list)(struct bwc_interface *, void *, struct list_head *, int);
	void		(*bw_trim_list)(struct bwc_interface *, void *, struct list_head *, int);
	void		(*bw_flush_update)(struct bwc_interface *, void*, uint64_t);
	int		(*bw_chan_inactive)(struct bwc_interface *, void *);
	int		(*bw_stop)(struct bwc_interface *);
	void		(*bw_start_page)(struct bwc_interface *, void *, void *, int);
	void		(*bw_end_page)(struct bwc_interface *, void *, void *, int);
	void		(*bw_get_chan_stat)(struct bwc_interface *, void *, struct io_chan_stat *);
	void 		(*bw_get_vec_stat)(struct bwc_interface *, void *, struct io_vec_stat *);
	void 		(*bw_get_vec_stat_by_uuid)(struct bwc_interface *, char *, struct io_vec_stat *);
	void* 		(*bw_get_all_vec_stat)(struct bwc_interface *, int *);
	void		(*bw_get_stat)(struct bwc_interface *, struct io_stat *);
	void		(*bw_get_info_stat)(struct bwc_interface *, struct umessage_bwc_information_resp_stat *);
	int		(*bw_fast_flush)(struct bwc_interface *, char *);
	int		(*bw_get_fast_flush)(struct bwc_interface *, char *);
	int		(*bw_stop_fast_flush)(struct bwc_interface *, char *);
	void		(*bw_sync_rec_seq)(struct bwc_interface *);
	void		(*bw_stop_sync_rec_seq)(struct bwc_interface *);
	int		(*bw_vec_start)(struct bwc_interface *, char *);
	int		(*bw_vec_stop)(struct bwc_interface *, char *);
	uint16_t	(*bw_get_block_occupied)(struct bwc_interface *);
	uint32_t	(*bw_get_release_page_counter)(struct bwc_interface *);
	uint32_t	(*bw_get_flush_page_counter)(struct bwc_interface *);
	char*		(*bw_get_uuid)(struct bwc_interface *);
	int             (*bw_freeze_chain_stage1)(struct bwc_interface *, char *);
	int             (*bw_freeze_chain_stage2)(struct bwc_interface *, char *);
	int		(*bw_destroy_chain)(struct bwc_interface *, char *);
	int		(*bw_unfreeze_snapshot)(struct bwc_interface *, char *);
	int		(*bw_dropcache_start)(struct bwc_interface *, char *, int);
	int		(*bw_dropcache_stop)(struct bwc_interface *, char *);
	void            (*bw_get_rw)(struct bwc_interface *,char *, struct bwc_rw_stat *);
	void            (*bw_get_delay)(struct bwc_interface *, struct bwc_delay_stat *);
	void		(*bw_get_throughput)(struct bwc_interface *, struct bwc_throughput_stat *);
	void		(*bw_reset_throughput)(struct bwc_interface *);
	void		(*bw_reset_read_counter)(struct bwc_interface *);
	int64_t		(*bw_get_read_counter)(struct bwc_interface *);
	uint64_t	(*bw_get_coalesce_index)(struct bwc_interface *);
	void		(*bw_reset_coalesce_index)(struct bwc_interface *);
	void		(*bw_merge_prepare)(struct bwc_interface *, char *);
	uint8_t		(*bw_get_cutoff)(struct bwc_interface *, void *);
	int		(*bw_destroy)(struct bwc_interface *);
	int		(*bw_write_response)(struct bwc_interface *);
	void		(*bw_post_initialization)(struct bwc_interface *);
	int		(*bw_update_water_mark)(struct bwc_interface *, int, int);
	int		(*bw_update_delay_level)(struct bwc_interface *, struct bwc_delay_stat *);
	void		(*bw_start_ioinfo)(struct bwc_interface *);
	void		(*bw_stop_ioinfo)(struct bwc_interface *);
	void		(*bw_check_ioinfo)(struct bwc_interface *, struct bwc_req_stat *);
	int		(*bw_get_list_info)(struct bwc_interface *, char *, struct umessage_bwc_information_list_resp_stat *);
};

struct bwc_interface {
	void			*bw_private;
	struct bwc_operations	*bw_op;
};

struct allocator_interface;
struct wc_interface;
struct rc_interface;
struct pp_interface;
struct bfe_interface;
struct tp_interface;
struct bfp_interface;
struct srn_interface;
struct lstn_interface;

struct bwc_throughput_stat {
	uint64_t	data_flushed;
	uint64_t	data_pkg_flushed;
	uint64_t	nondata_pkg_flushed;
};

struct bwc_rw_stat {
	uint64_t        overwritten_num;
	uint64_t        overwritten_back_num;
	uint64_t        coalesce_num;
	uint64_t        coalesce_back_num;
	uint64_t        flush_num;
	uint64_t        flush_back_num;
	uint64_t        flush_page;
	uint64_t        not_ready_num;
	uint64_t        res;
};

struct bwc_req_stat {
	uint8_t		is_running;
	uint64_t	req_num;
	uint64_t	req_len;
};

struct bwc_delay_stat {
	int				write_delay_first_level;
	int				write_delay_first_level_max_us;
	int				write_delay_first_level_increase_interval;
	int				write_delay_second_level;
	int				write_delay_second_level_max_us;
	int				write_delay_second_level_increase_interval;
	int				write_delay_second_level_start_ms;
	__useconds_t			write_delay_time;
};

struct bwc_setup {
	char				uuid[NID_MAX_UUID];
	struct allocator_interface	*allocator;
	struct wc_interface		*wc;
	struct rc_interface		*rc;
	struct bfp_interface		*bfp;
	struct pp_interface		*pp;
	struct tp_interface		*tp;
	struct srn_interface		*srn;
	struct lstn_interface		*lstn;
	struct mqtt_interface		*mqtt;
	char				bufdevice[NID_MAX_PATH];
	char				tp_name[NID_MAX_TPNAME];
	int				bufdevicesz;
	int				rw_sync;
	int				two_step_read;
	int				do_fp;
	int				ssd_mode;
	int				max_flush_size;
	int				write_delay_first_level;
	int				write_delay_first_level_max_us;
	int				write_delay_second_level;
	int				write_delay_second_level_max_us;
	int				bfp_type;
//	uint32_t			pagesz;
//	ssize_t 			buffersz;
	double				coalesce_ratio;
	double  			load_ratio_max;
	double  			load_ratio_min;
	double  			load_ctrl_level;
	double  			flush_delay_ctl;
	double				throttle_ratio;
	int				low_water_mark;
	int				high_water_mark;
};

extern int bwc_initialization(struct bwc_interface *, struct bwc_setup *);
#endif
