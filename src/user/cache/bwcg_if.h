/*
 * bwcg_if.h
 * 	Interface of None-Memory Write Cache Guardian Module
 */
#ifndef NID_BWCG_IF_H
#define NID_BWCG_IF_H

#define BWCG_MAX_BWC	16

#include "list.h"

struct umessage_bwc_information_resp_stat;
struct umessage_bwc_information_list_resp_stat;
struct bwc_throughput_stat;
struct bwc_rw_stat;
struct bwc_delay_stat;
struct bwcg_interface;
struct bwcg_operations {
	struct wc_interface*	(*wg_search_bwc)(struct bwcg_interface *, char *);
	struct wc_interface*	(*wg_search_and_create_bwc)(struct bwcg_interface *, char *);
	int			(*wg_add_bwc)(struct bwcg_interface *, char *, char *, int, int, int, int, int,
						double, double, double, double, double, double, char *, int, int,
						int, int, int, int);
	int			(*wg_recover_bwc)(struct bwcg_interface *, char *);
	void			(*wg_recover_all_bwc)(struct bwcg_interface *);
	int			(*wg_get_recover_bwc)(struct bwcg_interface *, char *);
	int			(*wg_remove_bwc)(struct bwcg_interface *, char *);
	void			(*wg_get_info_stat)(struct bwcg_interface *, char *, struct umessage_bwc_information_resp_stat *);
	void			(*wg_get_throughput)(struct bwcg_interface *, char *, struct bwc_throughput_stat *);
	void			(*wg_get_rw)(struct bwcg_interface *, char *, char *, struct bwc_rw_stat *);
	void			(*wg_get_delay)(struct bwcg_interface *, char *, struct bwc_delay_stat *);
	void			(*wg_reset_throughput)(struct bwcg_interface *, char *);
	void			(*wg_dropcache_start)(struct bwcg_interface *, char *, char *, int);
	void			(*wg_dropcache_stop)(struct bwcg_interface *, char *, char *);
	void			(*wg_fflush_start)(struct bwcg_interface *, char *, char *);
	void			(*wg_fflush_start_all)(struct bwcg_interface *);
	int			(*wg_fflush_get)(struct bwcg_interface *, char *, char *);
	void			(*wg_fflush_stop)(struct bwcg_interface *, char *, char *);
	void			(*wg_fflush_stop_all)(struct bwcg_interface *);
	int			(*wg_freeze_snapshot_stage1)(struct bwcg_interface *, char *, char *);
	int                     (*wg_freeze_snapshot_stage2)(struct bwcg_interface *, char *, char *);
	int			(*wg_unfreeze_snapshot)(struct bwcg_interface *, char *, char *);
	struct bwc_setup*	(*wg_get_all_bwc_setup)(struct bwcg_interface *, int *);
	int*			(*wg_get_working_bwc_index)(struct bwcg_interface *, int *);
	int			(*wg_update_water_mark)(struct bwcg_interface *, char *, int, int);
	int			(*wg_update_delay_level)(struct bwcg_interface *, char *, struct bwc_delay_stat *);
	struct bwc_interface**	(*wg_get_all_bwc)(struct bwcg_interface *, int *);
	void			(*wg_get_all_bwc_stat)(struct bwcg_interface *, struct list_head *out_head);
	void			(*wg_info_list_stat)(struct bwcg_interface *, char *, char *, struct umessage_bwc_information_list_resp_stat *);
};

struct bwcg_interface {
	void			*wg_private;
	struct bwcg_operations	*wg_op;
};

struct ini_interface;
struct sdsg_interface;
struct tpg_interface;
struct srn_interface;
struct allocator_interface;
struct lstn_interface;
struct io_interface;
struct bwcg_setup {
	struct ini_interface		**ini;
	struct sdsg_interface		*sdsg;
	struct tp_interface		*wtp;
	struct srn_interface		*srn;
	struct allocator_interface	*allocator;
	struct lstn_interface		*lstn;
	struct io_interface		*io;
	struct tpg_interface		*tpg;
	struct mqtt_interface		*mqtt;
};

extern int bwcg_initialization(struct bwcg_interface *, struct bwcg_setup *);
#endif
