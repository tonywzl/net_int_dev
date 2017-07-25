/*
 * d2an_if.h
 * 	Interface of two Data (d2) type A Node Module
 */
#ifndef NID_D2AN_IF_H
#define NID_D2AN_IF_H

#include "list.h"
#include "node_if.h"

struct d2a_node {
	struct node_header	d2_header;
	void			*d2_data[2];
	volatile int		d2_acounter;	// atomic counter		
};

struct d2an_interface;
struct d2an_operations {
	struct d2a_node*	(*d2_get_node)(struct d2an_interface *);
	void			(*d2_put_node)(struct d2an_interface *, struct d2a_node *);
	void			(*d2_destroy)(struct d2an_interface *);
};

struct d2an_interface {
	void			*d2_private;
	struct d2an_operations	*d2_op;
};

struct allocator_interface;
struct d2an_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int d2an_initialization(struct d2an_interface *, struct d2an_setup *);
#endif
