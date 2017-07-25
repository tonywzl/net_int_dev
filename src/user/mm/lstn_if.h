/*
 * lstn_if.h
 * 	Interface of Lst Node Module
 */
#ifndef NID_LSTN_IF_H
#define NID_LSTN_IF_H

#include "list.h"
#include "node_if.h"

struct list_node {
	struct node_header	ln_header;
	struct list_head	ln_list;
	void			*ln_data;
};

struct lstn_interface;
struct lstn_operations {
	struct list_node*	(*ln_get_node)(struct lstn_interface *);
	void			(*ln_put_node)(struct lstn_interface *, struct list_node *);
};

struct lstn_interface {
	void			*ln_private;
	struct lstn_operations	*ln_op;
};

struct allocator_interface;
struct lstn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int lstn_initialization(struct lstn_interface *, struct lstn_setup *);
#endif
