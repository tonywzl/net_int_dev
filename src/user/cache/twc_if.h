/*
 * twc_if.h
 * 	Interface of Write Through Cache Module
 */

#include <stdint.h>

#ifndef NID_TWC_IF_H
#define NID_TWC_IF_H

#include <list.h>
#include "nid_shared.h"

struct twc_throughput_stat;
struct twc_rw_stat;
struct pp_page_node;
struct io_chan_stat;
struct io_vec_stat;
struct io_stat;
struct twc_interface;
struct list_head;
struct rw_interface;
struct wc_channel_info;
struct umessage_twc_information_resp_stat;
struct twc_operations {
	void*		(*tw_create_channel)(struct twc_interface *, void *, struct wc_channel_info *, char *, int *);
	void*		(*tw_prepare_channel)(struct twc_interface *, struct wc_channel_info *, char *);
	void		(*tw_recover)(struct twc_interface *);
	ssize_t		(*tw_pread)(struct twc_interface *, void *, void *, size_t, off_t);
	void		(*tw_read_list)(struct twc_interface *, void *, struct list_head *);
	void		(*tw_write_list)(struct twc_interface *, void *, struct list_head *, int);
	void		(*tw_trim_list)(struct twc_interface *, void *, struct list_head *, int);
	int		(*tw_chan_inactive)(struct twc_interface *, void *);
	int		(*tw_stop)(struct twc_interface *);
	void		(*tw_get_info_stat)(struct twc_interface *, struct umessage_twc_information_resp_stat *);
	int		(*tw_destroy)(struct twc_interface *);
	char*		(*tw_get_uuid)(struct twc_interface *);
	void		(*tw_get_rw)(struct twc_interface *,char *, struct twc_rw_stat *);
	void		(*tw_get_stat)(struct twc_interface *, struct io_stat *);
	void		(*tw_end_page)(struct twc_interface *, void *, void *, int);
};

struct twc_interface {
	void			*tw_private;
	struct twc_operations	*tw_op;
};

struct allocator_interface;
struct wc_interface;
struct rc_interface;
struct tp_interface;
struct srn_interface;
struct lstn_interface;
struct pp_interface;

struct twc_rw_stat {
	uint64_t        all_write_counts;
	uint64_t        all_read_counts;
	uint64_t        res;
};

struct twc_setup {
	char				uuid[NID_MAX_UUID];
	struct allocator_interface	*allocator;
	struct rc_interface		*rc;
	struct tp_interface		*tp;
	struct pp_interface		*pp;
	struct srn_interface		*srn;
	struct lstn_interface		*lstn;
	char				tp_name[NID_MAX_TPNAME];
	int				do_fp;
};

extern int twc_initialization(struct twc_interface *, struct twc_setup *);
#endif
