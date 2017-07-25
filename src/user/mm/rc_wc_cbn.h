/*
 * rc_wc_cbn_if.h
 * 	Interface of rc wc callback node Module
 */
#ifndef NID_RC_WC_CBN_IF_H
#define NID_RC_WC_CBN_IF_H

#include "list.h"
#include "node_if.h"

struct request_node;
struct sub_request_node;
struct rc_wc_cb_node {
	struct node_header	rm_header;
	struct request_node 	*rm_rn_p;
	struct sub_request_node *rm_sreq_p;
	struct rc_wc_node	*rm_wc_node;
	void			*rm_rw_handle;
};

struct rc_wc_cbn_interface;
struct rc_wc_cbn_operations {
	struct rc_wc_cb_node*	(*rm_get_node)(struct rc_wc_cbn_interface *);
	void			(*rm_put_node)(struct rc_wc_cbn_interface *, struct rc_wc_cb_node *);
	void			(*rm_destroy)(struct rc_wc_cbn_interface *);
};

struct rc_wc_cbn_interface {
	void			*rm_private;
	struct rc_wc_cbn_operations	*rm_op;
};

struct allocator_interface;
struct rc_wc_cbn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int rc_wc_cbn_initialization(struct rc_wc_cbn_interface *, struct rc_wc_cbn_setup *);
#endif
