/*
 * rdwn.c
 * 	Implementation of Resource For Distributed Write Cache Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "rdwn_if.h"

struct allocator_interface;
struct rdwn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct rs_dwc_node *
rdwn_get_node(struct rdwn_interface *rdwn_p)
{
	struct rdwn_private *priv_p = (struct rdwn_private *)rdwn_p->dw_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct rs_dwc_node *lnp;

	nhp = node_p->n_op->n_get_node(node_p);
	lnp = (struct rs_dwc_node *)nhp;
	return lnp;
}

static void
rdwn_put_node(struct rdwn_interface *rdwn_p, struct rs_dwc_node *lnp)
{
	struct rdwn_private *priv_p = (struct rdwn_private *)rdwn_p->dw_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)lnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct rdwn_operations rdwn_op = {
	.dw_get_node = rdwn_get_node,
	.dw_put_node = rdwn_put_node,
};

int
rdwn_initialization(struct rdwn_interface *rdwn_p, struct rdwn_setup *setup)
{
	char *log_header = "rdwn_initialization";
	struct rdwn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	rdwn_p->dw_op = &rdwn_op;
	priv_p = calloc(1, sizeof(*priv_p));
	rdwn_p->dw_private = priv_p;

	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct rs_dwc_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 

	return 0;
}
