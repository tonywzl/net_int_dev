/*
 * blksn_if.h
 * 	Interface of Block Node With Variable Size Module
 */
#ifndef NID_BLKSN_IF_H
#define NID_BLKSN_IF_H

#include "list.h"
#include "node_if.h"

struct block_size_node {
	struct node_header	bsn_header;
	void			*bsn_slot;
	struct list_head	bsn_list;
	struct list_head	bsn_olist;	// order list
	uint64_t		bsn_index;
	uint64_t		bsn_size;
};

struct blksn_interface;
struct blksn_operations {
	struct block_size_node *	(*bsn_get_node)(struct blksn_interface *);
	void				(*bsn_put_node)(struct blksn_interface *, struct block_size_node *);
};

struct blksn_interface {
	void			*bsn_private;
	struct blksn_operations	*bsn_op;
};

struct allocator_interface;
struct blksn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int blksn_initialization(struct blksn_interface *, struct blksn_setup *);
#endif
