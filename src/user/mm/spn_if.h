/*
 * spn_if.h
 * 	Interface of Space Node Module
 */
#ifndef NID_SPN_IF_H
#define NID_SPN_IF_H

#include <stdint.h>
#include "list.h"

struct spn_header {
	struct list_head	sn_list;
	void			*sn_owner;
	void			*sn_data;
};

struct spn_interface;
struct spn_operations {
	struct spn_header *	(*sn_get_node)(struct spn_interface *);
	void			(*sn_put_node)(struct spn_interface *, struct spn_header *);
	void			(*sn_destroy)(struct spn_interface *);
};

struct spn_interface {
	void			*sn_private;
	struct spn_operations	*sn_op;
};

struct allocator_interface;
struct spn_setup {
	struct allocator_interface	*allocator;
	int				a_set_id;
	uint32_t			spn_size;
	uint32_t			seg_size;
	size_t				alignment;
};

extern int spn_initialization(struct spn_interface *, struct spn_setup *);

#endif

