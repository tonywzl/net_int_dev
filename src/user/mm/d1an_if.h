/*
 * d1an_if.h
 * 	Interface of One Data (d1) type A Node Module
 */
#ifndef NID_d1aN_IF_H
#define NID_d1aN_IF_H

#include <sys/uio.h>

#include "list.h"
#include "rw_if.h"
#include "node_if.h"

struct d1a_node {
	struct rw_callback_arg	d1_arg;		// always be the first item
	void			*d1_data;
	struct list_head	d1_list;
	int			d1_counter;
	off_t			d1_offset;
	int			d1_flag;
	int 			d1_ecode;
	int			d1_retry_time;
	struct iovec		d1_iov[];
};

struct d1an_interface;
struct d1an_operations {
	struct d1a_node*	(*d1_get_node)(struct d1an_interface *);
	void			(*d1_put_node)(struct d1an_interface *, struct d1a_node *);
	void			(*d1_destroy)(struct d1an_interface *);
};

struct d1an_interface {
	void			*d1_private;
	struct d1an_operations	*d1_op;
};

struct allocator_interface;
struct d1an_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
	int				node_size;
};

extern int d1an_initialization(struct d1an_interface *, struct d1an_setup *);
#endif
