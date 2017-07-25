/*
 * nse_mds_cbn.c
 * 	Implementation of two Data (d2) type C Node Module 
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "nse_mds_cbn.h"

struct allocator_interface;
struct nse_mds_cbn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct nse_mds_cb_node *
nse_mds_cbn_get_node(struct nse_mds_cbn_interface *nse_mds_cbn_p)
{
	struct nse_mds_cbn_private *priv_p = (struct nse_mds_cbn_private *)nse_mds_cbn_p->nm_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct nse_mds_cb_node *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct nse_mds_cb_node *)nhp;
	return dnp;
}

static void
nse_mds_cbn_put_node(struct nse_mds_cbn_interface *nse_mds_cbn_p, struct nse_mds_cb_node *dnp)
{
	struct nse_mds_cbn_private *priv_p = (struct nse_mds_cbn_private *)nse_mds_cbn_p->nm_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct nse_mds_cbn_operations nse_mds_cbn_op = {
	.nm_get_node = nse_mds_cbn_get_node,
	.nm_put_node = nse_mds_cbn_put_node,
};

int
nse_mds_cbn_initialization(struct nse_mds_cbn_interface *nse_mds_cbn_p, struct nse_mds_cbn_setup *setup)
{
	char *log_header = "nse_mds_cbn_initialization";
	struct nse_mds_cbn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	nse_mds_cbn_p->nm_op = &nse_mds_cbn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	nse_mds_cbn_p->nm_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct nse_mds_cb_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
