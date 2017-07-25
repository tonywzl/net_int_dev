/*
 * nse_crc_cbn_if.h
 * 	Interface of nse  crc callback node Module
 */
#ifndef NID_nse_crc_cbn_IF_H
#define NID_nse_crc_cbn_IF_H

#include "list.h"
#include "node_if.h"

struct crc_rc_cb_node;
struct crc_interface;
struct nse_crc_cb_node {
	struct node_header	nc_header;
	struct list_head	nc_fp_found_head;
	struct list_head	nc_found_head;
	struct crc_interface	*nc_crc_p;
	struct crc_rc_cb_node 	*nc_super_arg;
	struct rw_interface	*nc_rw;
	void			*nc_rw_handle;
	void 			(*nc_super_callback)(int, struct crc_rc_cb_node *);
};

struct nse_crc_cbn_interface;
struct nse_crc_cbn_operations {
	struct nse_crc_cb_node*	(*nc_get_node)(struct nse_crc_cbn_interface *);
	void			(*nc_put_node)(struct nse_crc_cbn_interface *, struct nse_crc_cb_node *);
};

struct nse_crc_cbn_interface {
	void			*nc_private;
	struct nse_crc_cbn_operations	*nc_op;
};

struct allocator_interface;
struct nse_crc_cbn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int nse_crc_cbn_initialization(struct nse_crc_cbn_interface *, struct nse_crc_cbn_setup *);
#endif
