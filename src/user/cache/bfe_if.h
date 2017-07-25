/*
 * bfe_if.h
 * 	Interface of BIO Flush Engine Module
 */
#ifndef NID_BFE_IF_H
#define NID_BFE_IF_H

#include <stdint.h>

#define BFE_MAX_IOV	512

#define	BFE_PRIV_INDEX	0
#define	BFE_PAGE_INDEX	1

struct bfe_rw_stat {
	uint64_t	flush_page_num;
	uint64_t	overwritten_num;
	uint64_t	overwritten_back_num;
	uint64_t	flush_num;
	uint64_t	flush_back_num;
	uint64_t	coalesce_flush_num;
	uint64_t	coalesce_flush_back_num;
	uint64_t	not_ready_num;
};

struct bfe_interface;
struct io_vec_stat;
struct pp_page_node;
struct data_description_node;
struct list_head;
struct bfe_operations {
	void		(*bf_flush_page)(struct bfe_interface *, struct pp_page_node *);
	int		(*bf_right_extend_node)(struct bfe_interface *, struct data_description_node *, struct data_description_node *);
	void		(*bf_add_node)(struct bfe_interface *, struct data_description_node *);
	void		(*bf_left_cut_node)(struct bfe_interface *, struct data_description_node *, uint32_t, struct data_description_node *);
	void		(*bf_right_cut_node)(struct bfe_interface *, struct data_description_node *, uint32_t, struct data_description_node *);
	int		(*bf_right_cut_node_adjust)(struct bfe_interface *, struct data_description_node *,
			uint32_t, struct data_description_node *, struct data_description_node *);
	void		(*bf_del_node)(struct bfe_interface *, struct data_description_node *, struct data_description_node *);
	void		(*bf_flush_seq)(struct bfe_interface *, uint64_t);
	void		(*bf_close)(struct bfe_interface *);
	void		(*bf_end_page)(struct bfe_interface *, struct pp_page_node *);
	void		(*bf_vec_start)(struct bfe_interface *);
	void		(*bf_vec_stop)(struct bfe_interface *);
	void		(*bf_get_stat)(struct bfe_interface *, struct io_vec_stat *);
	void		(*bf_get_rw_stat)(struct bfe_interface *, struct bfe_rw_stat *);
	uint64_t        (*bf_get_coalesce_index)(struct bfe_interface *);
	void            (*bf_reset_coalesce_index)(struct bfe_interface *);
	int		(*bf_wait_flush_page_counter)(struct bfe_interface *, uint32_t, struct timespec *);
	uint32_t	(*bf_get_page_counter)(struct bfe_interface *);
	void		(*bf_trim_list)(struct bfe_interface *, struct list_head *);
};

struct bfe_interface {
	void			*bf_private;
	struct bfe_operations	*bf_op;
};

struct allocator_interface;
struct pp_interface;
struct bse_interface;
struct bre_interface;
struct bio_interface;
struct wc_interface;
struct rw_interface;
struct lstn_interface;
struct ddn_interface;
struct bfe_setup {
	struct allocator_interface	*allocator;
	struct pp_interface		*pp;
	struct bse_interface		*bse;
	struct bre_interface		*bre;
	struct bio_interface		*bio;
	struct wc_interface		*wc;
	struct rc_interface		*rc;
	struct rw_interface		*rw;
	void				*rw_handle;
	struct lstn_interface		*lstn;
	struct ddn_interface		*ddn;
	void				*bio_handle;
	void				*wc_chain_handle;
	void				*rc_handle;
	int				rw_sync;
	int				target_sync;
	int				do_fp;
	int				max_flush_size;
	char 				*bufdevice;
	int8_t				ssd_mode;
};

extern int bfe_initialization(struct bfe_interface *, struct bfe_setup *);
#endif
