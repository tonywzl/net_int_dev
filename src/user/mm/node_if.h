/*
 * node_if.h
 * 	Interface of Node Module
 */
#ifndef NID_NODE_IF_H
#define NID_NODE_IF_H

#include <stdint.h>
#include "list.h"

struct node_header {
	struct list_head	n_list;	
	void			*n_owner;
};

struct node_interface;
struct node_operations {
	struct node_header *	(*n_get_node)(struct node_interface *);
	void			(*n_put_node)(struct node_interface *, struct node_header *);
	void			(*n_destroy)(struct node_interface *);
};

struct node_interface {
	void			*n_private;
	struct node_operations	*n_op;
};

struct allocator_interface;
struct node_setup {
	struct allocator_interface	*allocator;
	int				a_set_id;
	uint32_t			node_size;
	uint32_t			seg_size;
	size_t				alignment;
};

extern int node_initialization(struct node_interface *, struct node_setup *);

#endif
