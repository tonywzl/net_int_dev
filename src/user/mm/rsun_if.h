/*
 * rsun_if.h
 * 	Interface of Resource Unit Node Module
 */
#ifndef NID_RSUN_IF_H
#define NID_RSUN_IF_H

#include "list.h"
#include "node_if.h"

struct rsu_node {
	struct node_header	un_header;
	struct list_head	un_list;
	char			*un_rsm_uuid;
	char			*un_rs_name;
	int			un_index;
	int			un_flags;
	off_t			un_off;
	off_t			un_data_off;
	void			*un_mnp;	// meta node pointer
	void			*un_mblock;	// meta block
};

struct rsun_interface;
struct rsun_operations {
	struct rsu_node*	(*un_get_node)(struct rsun_interface *);
	void			(*un_put_node)(struct rsun_interface *, struct rsu_node *);
};

struct rsun_interface {
	void			*un_private;
	struct rsun_operations	*un_op;
};

struct allocator_interface;
struct rsun_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int rsun_initialization(struct rsun_interface *, struct rsun_setup *);
#endif
