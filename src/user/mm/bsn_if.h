/*
 * bsn_if.h
 * 	Interface of BIO Search Node Module
 */
#ifndef NID_bsn_IF_H
#define NID_bsn_IF_H

#include "node_if.h"
#include "list.h"
#include "rtree_if.h"

struct rtree_interface;
struct list_head;
struct bsn_node {
	struct node_header	bn_header;
	struct list_head	bn_list;
	struct rtree_interface	bn_rtree;
	struct list_head	bn_ddn_head;
};

struct bsn_interface;
struct bsn_operations {
	struct bsn_node*	(*bn_get_node)(struct bsn_interface *);
	void			(*bn_put_node)(struct bsn_interface *, struct bsn_node *);
};

struct bsn_interface {
	void				*bn_private;
	struct bsn_operations	*bn_op;
};

struct allocator_interface;
struct bsn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int bsn_initialization(struct bsn_interface *, struct bsn_setup *);
#endif
