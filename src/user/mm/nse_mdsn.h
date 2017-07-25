/*
 * nse_mdsn_if.h
 * 	Interface of nse  mds node Module
 */
#ifndef NID_nse_mdsn_IF_H
#define NID_nse_mdsn_IF_H

#include "list.h"
#include "node_if.h"

struct nse_crc_cb_node;
struct nse_mds_node {
	struct node_header	nm_header;
	volatile int		nm_sub_request_cnt;
	struct nse_crc_cb_node 	*nm_super_arg;
	struct nse_interface 	*nm_nse_p;
	int 			nm_errcode;
	void (*nm_super_callback)(int errcode, struct nse_crc_cb_node *arg);
};

struct nse_mdsn_interface;
struct nse_mdsn_operations {
	struct nse_mds_node*	(*nm_get_node)(struct nse_mdsn_interface *);
	void			(*nm_put_node)(struct nse_mdsn_interface *, struct nse_mds_node *);
};

struct nse_mdsn_interface {
	void				*nm_private;
	struct nse_mdsn_operations	*nm_op;
};

struct allocator_interface;
struct nse_mdsn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int nse_mdsn_initialization(struct nse_mdsn_interface *, struct nse_mdsn_setup *);
#endif
