/*
 * d3an_if.h
 * 	Interface of Three Data (d3) type A Node Module
 */
#ifndef NID_d3AN_IF_H
#define NID_d3AN_IF_H

#include "list.h"
#include "atomic.h"
#include "node_if.h"

struct d3a_node {
	struct node_header	d3_header;
	void			*d3_data[3];
	nid_atomic_t		d3_acounter;	// atomic counter		
};

struct d3an_interface;
struct d3an_operations {
	struct d3a_node*	(*d3_get_node)(struct d3an_interface *);
	void			(*d3_put_node)(struct d3an_interface *, struct d3a_node *);
};

struct d3an_interface {
	void			*d3_private;
	struct d3an_operations	*d3_op;
};

struct allocator_interface;
struct d3an_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int d3an_initialization(struct d3an_interface *, struct d3an_setup *);
#endif
