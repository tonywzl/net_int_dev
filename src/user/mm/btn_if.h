/*
 * btn_if.h
 * 	Interface of Bse Tree Slot Node Module
 */
#ifndef NID_BTN_IF_H
#define NID_BTN_IF_H

#include <stdint.h>
#include <list.h>
#include <node_if.h>

struct btn_stat {
	uint32_t	s_nseg;
	uint32_t	s_segsz;
	uint32_t	s_nfree;
	uint32_t	s_nused;
};

struct btn_segment;
struct btn_node {
	struct node_header	n_header;
	struct list_head	n_ddn_head;
	void			*n_parent;
};

struct btn_interface;
struct btn_operations {
	struct btn_node*	(*bt_get_node)(struct btn_interface *);
	void			(*bt_put_node)(struct btn_interface *, struct btn_node *);
	void			(*bt_get_stat)(struct btn_interface *, struct btn_stat *);
	void			(*bt_destroy)(struct btn_interface *);
};

struct btn_interface {
	void			*bt_private;
	struct btn_operations	*bt_op;
};

struct allocator_interface;
struct btn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int btn_initialization(struct btn_interface *, struct btn_setup *);

#endif
