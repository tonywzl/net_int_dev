/*
 * nse_mds_cbn_if.h
 * 	Interface of nse  mds callback node Module
 */
#ifndef NID_nse_mds_cbn_IF_H
#define NID_nse_mds_cbn_IF_H

#include <stdbool.h>
#include "list.h"
#include "node_if.h"

struct sub_request_node;
struct nse_mds_node;
struct nse_mds_cb_node {
	struct node_header	nm_header;
	struct sub_request_node *nm_sreq_p;
	struct nse_mds_node 	*nm_nse_mds;
	bool			*nm_fp_bitmap;
	void			*nm_fp;
	bool			nm_isalignment;
	off_t			nm_off;
	uint32_t		nm_len;
};

struct nse_mds_cbn_interface;
struct nse_mds_cbn_operations {
	struct nse_mds_cb_node*	(*nm_get_node)(struct nse_mds_cbn_interface *);
	void			(*nm_put_node)(struct nse_mds_cbn_interface *, struct nse_mds_cb_node *);
};

struct nse_mds_cbn_interface {
	void			*nm_private;
	struct nse_mds_cbn_operations	*nm_op;
};

struct allocator_interface;
struct nse_mds_cbn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int nse_mds_cbn_initialization(struct nse_mds_cbn_interface *, struct nse_mds_cbn_setup *);
#endif
