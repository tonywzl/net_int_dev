/*
 * bwc_rbn_if.h
 * 	Interface of BWC Recover Buffer Node Module
 */
#ifndef NID_BWC_RBN_IF_H
#define NID_BWC_RBN_IF_H

#include "list.h"
#include "node_if.h"

#define BWC_RBN_BUFSZ (32 << 10)	// big enough for seg header and all req headers of one segment
struct bwc_rb_node {
	union {
		struct {
			struct node_header	nn_header;
			struct list_head	nn_list;
		};

		struct {
			char	space[512];	// alignment padding
		};
	};

	char	buf[BWC_RBN_BUFSZ];
};

struct bwc_rbn_interface;
struct bwc_rbn_operations {
	struct bwc_rb_node*	(*nn_get_node)(struct bwc_rbn_interface *);
	void			(*nn_put_node)(struct bwc_rbn_interface *, struct bwc_rb_node *);
	void			(*nn_destroy)(struct bwc_rbn_interface *);
};

struct bwc_rbn_interface {
	void				*nn_private;
	struct bwc_rbn_operations	*nn_op;
};

struct allocator_interface;
struct bwc_rbn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
	size_t				alignment;
};

extern int bwc_rbn_initialization(struct bwc_rbn_interface *, struct bwc_rbn_setup *);
#endif
