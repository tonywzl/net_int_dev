/*
 * rc_wcn_if.h
 * 	Interface of rc  wc callback node Module
 */
#ifndef NID_RC_WCN_IF_H
#define NID_RC_WCN_IF_H

#include "list.h"
#include "node_if.h"

struct request_node;
struct sub_request_node;
struct rc_wc_node {
	struct node_header	rm_header;
	volatile int		rm_sub_req_counter;
	char			rm_error;
};

struct rc_wcn_interface;
struct rc_wcn_operations {
	struct rc_wc_node*	(*rm_get_node)(struct rc_wcn_interface *);
	void			(*rm_put_node)(struct rc_wcn_interface *, struct rc_wc_node *);
	void			(*rm_destroy)(struct rc_wcn_interface *);
};

struct rc_wcn_interface {
	void			*rm_private;
	struct rc_wcn_operations	*rm_op;
};

struct allocator_interface;
struct rc_wcn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int rc_wcn_initialization(struct rc_wcn_interface *, struct rc_wcn_setup *);
#endif
