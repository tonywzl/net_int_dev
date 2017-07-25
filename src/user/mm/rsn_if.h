/*
 * rsn_if.h
 * 	Interface of Resource Node Module
 */
#ifndef NID_RSN_IF_H
#define NID_RSN_IF_H

#include "list.h"
#include "nid_shared.h"
#include "node_if.h"

struct rs_node {
	struct node_header	rn_header;
	struct list_head	rn_list;
	struct list_head	rn_free_unit_head;
	char			rn_exportname[NID_MAX_PATH];
	off_t			rn_size;
	void			*rn_data;
};

struct rsn_interface;
struct rsn_operations {
	struct rs_node*		(*rn_get_node)(struct rsn_interface *);
	void			(*rn_put_node)(struct rsn_interface *, struct rs_node *);
};

struct rsn_interface {
	void			*rn_private;
	struct rsn_operations	*rn_op;
};

struct allocator_interface;
struct rsn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int rsn_initialization(struct rsn_interface *, struct rsn_setup *);
#endif
