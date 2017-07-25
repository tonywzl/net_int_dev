/*
 * srn_if.h
 * 	Interface of Sub Request Node Module
 */
#ifndef NID_SRN_IF_H
#define NID_SRN_IF_H

#include <stdint.h>
#include <list.h>
#include "node_if.h"

struct srn_segment;
struct sub_request_node {
	struct node_header	sr_header;
	struct list_head	sr_list;
	char			*sr_buf;
	off_t			sr_offset;
	uint32_t		sr_len;
	struct sub_request_node	*sr_real;	// For two step read. If offset or len is not alignment we need make a alignment node, and put real non-alignment node here.
};

struct srn_interface;
struct srn_operations {
	struct sub_request_node*	(*sr_get_node)(struct srn_interface *);
	void				(*sr_put_node)(struct srn_interface *, struct sub_request_node *);
	void				(*sr_close)(struct srn_interface *);
};

struct srn_interface {
	void			*sr_private;
	struct srn_operations	*sr_op;
};

struct allocator_interface;
struct srn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int srn_initialization(struct srn_interface *, struct srn_setup *);
#endif
