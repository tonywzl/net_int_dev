/*
 * rsugn_if.h
 * 	Interface of Resource Unit Group Node Module
 */
#ifndef NID_RSUGN_IF_H
#define NID_RSUGN_IF_H

#include "list.h"
#include "node_if.h"

struct rsug_node {
	struct node_header	gn_header;
	struct list_head	gn_list;
	char			*gn_owner_uuid;
	uint8_t			gn_rsu_number;
	struct list_head	gn_rsu_head;
};

struct rsugn_interface;
struct rsugn_operations {
	struct rsug_node*	(*gn_get_node)(struct rsugn_interface *);
	void			(*gn_put_node)(struct rsugn_interface *, struct rsug_node *);
	void			(*gn_destroy)(struct rsugn_interface *);
};

struct rsugn_interface {
	void			*gn_private;
	struct rsugn_operations	*gn_op;
};

struct allocator_interface;
struct rsugn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int rsugn_initialization(struct rsugn_interface *, struct rsugn_setup *);
#endif
