/*
 * fpn_if.h
 * 	Interface of Finger Print Node Module
 */
#ifndef NID_FPN_IF_H
#define NID_FPN_IF_H

#include "list.h"
#include "node_if.h"

struct fp_node {
	struct node_header	fp_header;
	void			*fp_parent;
	struct list_head	fp_list;
	struct list_head	fp_lru_list;
	uint32_t		fp_ref;
	char			fp[0];
};

struct fpn_interface;
struct fpn_operations {
	struct fp_node *	(*fp_get_node)(struct fpn_interface *);
	void			(*fp_put_node)(struct fpn_interface *, struct fp_node *);
	int			(*fp_get_fp_size)(struct fpn_interface *);
};

struct fpn_interface {
	void			*fp_private;
	struct fpn_operations	*fp_op;
};

struct allocator_interface;
struct fpn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
	int				fp_size;
};

extern int fpn_initialization(struct fpn_interface *, struct fpn_setup *);
#endif
