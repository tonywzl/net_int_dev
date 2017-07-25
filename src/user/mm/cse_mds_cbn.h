/*
 * cse_mds_cbn_if.h
 * 	Interface of nse  mds callback node Module
 */
#ifndef NID_cse_mds_cbn_IF_H
#define NID_cse_mds_cbn_IF_H

#include <stdbool.h>
#include "list.h"
#include "node_if.h"

struct cse_crc_cb_node;
struct cse_interface;
struct cse_mds_cb_node {
	struct node_header	cm_header;
	struct cse_interface	*cm_cse_p;
	struct iovec		*cm_vec;
	char			*cm_fp;
	struct list_head	cm_not_found_head;
	struct list_head	cm_not_found_fp_head;
	struct cse_crc_cb_node	*cm_super_arg;
	void (*cm_super_callback)(int errcode, struct cse_crc_cb_node *arg);
};

struct cse_mds_cbn_interface;
struct cse_mds_cbn_operations {
	struct cse_mds_cb_node*	(*cm_get_node)(struct cse_mds_cbn_interface *);
	void			(*cm_put_node)(struct cse_mds_cbn_interface *, struct cse_mds_cb_node *);
};

struct cse_mds_cbn_interface {
	void				*cm_private;
	struct cse_mds_cbn_operations	*cm_op;
};

struct allocator_interface;
struct cse_mds_cbn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int cse_mds_cbn_initialization(struct cse_mds_cbn_interface *, struct cse_mds_cbn_setup *);
#endif
