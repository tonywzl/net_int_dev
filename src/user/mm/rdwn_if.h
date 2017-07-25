/*
 * rdwn_if.h
 * 	Interface of Resource For Distributed Write Cache Node Module
 */
#ifndef NID_RDWN_IF_H
#define NID_RDWN_IF_H

#include "list.h"
#include "node_if.h"

struct rs_dwc_node {
	struct node_header	dw_header;
};

struct rdwn_interface;
struct rdwn_operations {
	struct rs_dwc_node*	(*dw_get_node)(struct rdwn_interface *);
	void			(*dw_put_node)(struct rdwn_interface *, struct rs_dwc_node *);
};

struct rdwn_interface {
	void			*dw_private;
	struct rdwn_operations	*dw_op;
};

struct allocator_interface;
struct rdwn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int rdwn_initialization(struct rdwn_interface *, struct rdwn_setup *);
#endif
