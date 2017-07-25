/*
 * cse_crc_cbn_if.h
 * 	Interface of rc  bwc callback node Module
 */
#ifndef NID_cse_crc_cbn_IF_H
#define NID_cse_crc_cbn_IF_H

#include "list.h"
#include "node_if.h"

struct crc_rc_cb_node;

struct cse_crc_cb_node {
	struct node_header	cc_header;
	struct crc_interface	*cc_crc_p;
	struct rw_interface	*cc_rw;
	void			*cc_rw_handle;
	struct crc_rc_cb_node 	*cc_super_arg;
	void 			(*cc_super_callback)(int, struct crc_rc_cb_node *);
};

struct cse_crc_cbn_interface;
struct cse_crc_cbn_operations {
	struct cse_crc_cb_node*	(*cc_get_node)(struct cse_crc_cbn_interface *);
	void			(*cc_put_node)(struct cse_crc_cbn_interface *, struct cse_crc_cb_node *);
};

struct cse_crc_cbn_interface {
	void			*cc_private;
	struct cse_crc_cbn_operations	*cc_op;
};

struct allocator_interface;
struct cse_crc_cbn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int cse_crc_cbn_initialization(struct cse_crc_cbn_interface *, struct cse_crc_cbn_setup *);
#endif
