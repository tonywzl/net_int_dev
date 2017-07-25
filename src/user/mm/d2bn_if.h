/*
 * d2bn_if.h
 * 	Interface of two Data (d2) type B Node Module
 */
#ifndef NID_D2BN_IF_H
#define NID_D2BN_IF_H

#include "list.h"
#include "rw_if.h"
#include "node_if.h"

struct d2b_node {
	struct rw_callback_arg	d2_arg;
	void			*d2_data[2];
	struct list_head	d2_list;
	int			d2_flag;
	int			d2_ret;
	int			d2_retry_time;
};

struct d2bn_interface;
struct d2bn_operations {
	struct d2b_node*	(*d2_get_node)(struct d2bn_interface *);
	void			(*d2_put_node)(struct d2bn_interface *, struct d2b_node *);
	void			(*d2_destroy)(struct d2bn_interface *);
};

struct d2bn_interface {
	void			*d2_private;
	struct d2bn_operations	*d2_op;
};

struct allocator_interface;
struct d2bn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int d2bn_initialization(struct d2bn_interface *, struct d2bn_setup *);
#endif
