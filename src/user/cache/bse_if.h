/*
 * bse_if.h
 * 	Interface of Buffer Search Engine Module
 */
#ifndef NID_bse_IF_H
#define NID_bse_IF_H

#include <stdint.h>

struct request_node;
struct list_head;
struct io_chan_stat;
struct bse_interface;
struct data_description_node;
struct bse_operations {
	struct data_description_node*
		(*bs_contiguous_low_bound)(struct bse_interface *, struct data_description_node *, int max, int);
	struct data_description_node*
		(*bs_contiguous_low_bound_async)(struct bse_interface *, struct data_description_node *, int max);
	struct data_description_node*
		(*bs_traverse_next)(struct bse_interface *);
	struct data_description_node*
		(*bs_traverse_next_async)(struct bse_interface *);
	void		(*bs_traverse_start)(struct bse_interface *, struct data_description_node *);
	void		(*bs_traverse_set)(struct bse_interface *, struct data_description_node *);
	void		(*bs_traverse_end)(struct bse_interface *);
	void		(*bs_add_list)(struct bse_interface *, struct list_head *);
	void		(*bs_del_node)(struct bse_interface *, struct data_description_node *);
	void		(*bs_search)(struct bse_interface *, struct request_node *);
	void		(*bs_close)(struct bse_interface *);
	void		(*bs_get_chan_stat)(struct bse_interface *, struct io_chan_stat *);
	void		(*bs_create_snapshot)(struct bse_interface *);
};

struct bse_interface {
	void			*bs_private;
	struct bse_operations	*bs_op;
};

struct allocator_interface;
struct pp_interface;
struct ddn_interface;
struct srn_interface;
struct bse_interface;
struct bse_setup {
	struct allocator_interface	*allocator;
	struct pp_interface		*pp;
	struct ddn_interface		*ddn;
	struct srn_interface		*srn;
	struct bfe_interface		*bfe;
	char				*bufdevice;
	uint32_t			devsz;		// unit: G, must be power of 2
	uint32_t			cachesz;	// must be power of 2
};

extern int bse_initialization(struct bse_interface *, struct bse_setup *);
#endif
