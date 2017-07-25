/*
 * wc_if.h
 * 	Interface of Write Cache Module
 */
#ifndef NID_WC_IF_H
#define NID_WC_IF_H

#define WC_TYPE_NONE		0
#define WC_TYPE_TWC		1
#define WC_TYPE_NONE_MEMORY	2
#define WC_TYPE_MAX		3

#define WC_RECOVER_NONE		0
#define WC_RECOVER_DOING	1
#define WC_RECOVER_DONE		2

#include <stdint.h>

struct wc_channel_info {
	void	*wc_rc;
	void	*wc_rc_handle;
	void	*wc_rw;
	char	*wc_rw_exportname;
	char	wc_rw_sync;
	char	wc_rw_direct_io;
	int	wc_rw_alignment;
	int	wc_rw_dev_size;
};

struct umessage_wc_information_resp_stat;
struct wc_interface;
struct rw_interface;
struct list_head;
struct io_chan_stat;
struct io_vec_stat;
struct io_stat;
struct wc_channel_info;
struct wc_operations {
	void*		(*wc_create_channel)(struct wc_interface *, void *, struct wc_channel_info *, char *, int *);
	void*		(*wc_prepare_channel)(struct wc_interface *, struct wc_channel_info *, char *);
	void		(*wc_recover)(struct wc_interface *);
	uint32_t	(*wc_get_poolsz)(struct wc_interface *);
	uint32_t	(*wc_get_pagesz)(struct wc_interface *);
	uint32_t	(*wc_get_block_occupied)(struct wc_interface *);
	uint32_t	(*wc_pread)(struct wc_interface *, void *, void *, size_t, off_t);
	void		(*wc_read_list)(struct wc_interface *, void *, struct list_head *, int);
	void		(*wc_write_list)(struct wc_interface *, void *, struct list_head *, int);
	void		(*wc_trim_list)(struct wc_interface *, void *, struct list_head *, int);
	int		(*wc_chan_inactive)(struct wc_interface *, void *);
	int		(*wc_stop)(struct wc_interface *);
	int		(*wc_start_page)(struct wc_interface *, void *, void *, int);
	int		(*wc_end_page)(struct wc_interface *, void *, void *, int);
	int		(*wc_flush_update)(struct wc_interface *, void *, uint64_t);
	void*		(*wc_get_cache_obj)(struct wc_interface *);
	int		(*wc_get_chan_stat)(struct wc_interface *, void *, struct io_chan_stat *);
	int		(*wc_get_vec_stat)(struct wc_interface *, void *, struct io_vec_stat *);
	int		(*wc_get_stat)(struct wc_interface *, struct io_stat *);
	int		(*wc_get_info_stat)(struct wc_interface *, struct umessage_wc_information_resp_stat *);
	int		(*wc_fast_flush)(struct wc_interface *, char *);
	int		(*wc_get_fast_flush)(struct wc_interface *, char *);
	int		(*wc_stop_fast_flush)(struct wc_interface *, char *);
	int		(*wc_vec_start)(struct wc_interface *);
	int		(*wc_vec_stop)(struct wc_interface *);
	uint32_t	(*wc_get_release_page_counter)(struct wc_interface *);
	uint32_t	(*wc_get_flush_page_counter)(struct wc_interface *);
	char*		(*wc_get_uuid)(struct wc_interface *);
	int		(*wc_dropcache_start)(struct wc_interface *, char *, int);
	int		(*wc_dropcache_stop)(struct wc_interface *, char *);
	int		(*wc_get_recover_state)(struct wc_interface *);
	int32_t		(*wc_reset_read_counter)(struct wc_interface *);
	int64_t		(*wc_get_read_counter)(struct wc_interface *);
	uint64_t 	(*wc_get_coalesce_index)(struct wc_interface *);
	void 		(*wc_reset_coalesce_index)(struct wc_interface *);
	uint8_t		(*wc_get_cutoff)(struct wc_interface *, void *);
	int		(*wc_destroy)(struct wc_interface *);
	char*		(*wc_get_name)(struct wc_interface *);
	int		(*wc_freeze_snapshot_stage1)(struct wc_interface *, char *);
	int		(*wc_freeze_snapshot_stage2)(struct wc_interface *, char *);
	int		(*wc_unfreeze_snapshot)(struct wc_interface *, char *);
	int		(*wc_get_wcd_channel_info)(struct wc_interface *, void *, void *);
	int		(*wc_set_ddn_info)(struct wc_interface *, void *, void *);
	int		(*wc_set_rn_info)(struct wc_interface *, void *);
	int		(*wc_write_response)(struct wc_interface *);
	void		(*wc_get_write_list)(struct wc_interface *, struct list_head *);
	int		(*wc_get_type)(struct wc_interface *);
	int		(*wc_destroy_chain)(struct wc_interface *, char *);
	void		(*wc_post_initialization)(struct wc_interface *);
};

struct wc_interface {
	void			*wc_private;
	struct wc_operations	*wc_op;
};

struct pp_interface;
struct tp_interface;
struct srn_interface;
struct allocator_interface;
struct lstn_interface;
struct wc_setup {
	int				type;
	char				*uuid;
	struct pp_interface		*pp;
	struct tp_interface		*tp;
	char				*bufdevice;
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
	struct srn_interface		*srn;
	struct lstn_interface		*lstn;
	struct allocator_interface	*allocator;
	int				bfp_type;
	double				coalesce_ratio;
	double				load_ratio_max;
	double				load_ratio_min;
	double				load_ctrl_level;
	double				flush_delay_ctl;
	double				throttle_ratio;
        int                             low_water_mark;
        int                             high_water_mark;
	struct mqtt_interface		*mqtt;
};

extern int wc_initialization(struct wc_interface *, struct wc_setup *);
#endif
