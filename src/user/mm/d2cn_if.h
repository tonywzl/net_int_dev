/*
 * d2cn_if.h
 * 	Interface of two Data (d2) type C Node Module
 */
#ifndef NID_D2CN_IF_H
#define NID_D2CN_IF_H

#include "list.h"
#include "node_if.h"

struct d2c_node {
	struct node_header	d2_header;
	void			*d2_data[2];
	volatile int		d2_acounter;	// atomic counter
	uint8_t			d2_partial_flush;
	int			d2_fl_counter;	// flushing counter
	uint64_t		d2_fl_seq;	// flushing sequence
	int			d2_nr_counter;	// not ready counter
	int			d2_ow_counter;	// overwritten counter
	uint64_t		d2_seq;
	struct list_head	d2_head;	
};

struct d2cn_interface;
struct d2cn_operations {
	struct d2c_node*	(*d2_get_node)(struct d2cn_interface *);
	void			(*d2_put_node)(struct d2cn_interface *, struct d2c_node *);
	void			(*d2_destroy)(struct d2cn_interface *);
};

struct d2cn_interface {
	void			*d2_private;
	struct d2cn_operations	*d2_op;
};

struct allocator_interface;
struct d2cn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int d2cn_initialization(struct d2cn_interface *, struct d2cn_setup *);
#endif
