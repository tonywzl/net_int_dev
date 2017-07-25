/*
 * brn_if.h
 * 	Interface of Bridge Node Module
 */
#ifndef NID_BRN_IF_H
#define NID_BRN_IF_H

#include "list.h"
#include "node_if.h"

struct bridge_node {
	struct node_header	bn_header;
	uint64_t		bn_index;
	void			*bn_parent;
	void			*bn_data;
};

struct brn_interface;
struct brn_operations {
	struct bridge_node*	(*bn_get_node)(struct brn_interface *);
	void			(*bn_put_node)(struct brn_interface *, struct bridge_node *);
};

struct brn_interface {
	void			*bn_private;
	struct brn_operations	*bn_op;
};

struct allocator_interface;
struct brn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int brn_initialization(struct brn_interface *, struct brn_setup *);
#endif
