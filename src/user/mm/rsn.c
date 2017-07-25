/*
 * rsn.c
 * 	Implementation of Resource Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "rsn_if.h"

struct allocator_interface;
struct rsn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct rs_node *
rsn_get_node(struct rsn_interface *rsn_p)
{
	struct rsn_private *priv_p = (struct rsn_private *)rsn_p->rn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct rs_node *rnp;

	nhp = node_p->n_op->n_get_node(node_p);
	rnp = (struct rs_node *)nhp;
	return rnp;
}

static void
rsn_put_node(struct rsn_interface *rsn_p, struct rs_node *rnp)
{
	struct rsn_private *priv_p = (struct rsn_private *)rsn_p->rn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)rnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct rsn_operations rsn_op = {
	.rn_get_node = rsn_get_node,
	.rn_put_node = rsn_put_node,
};

int
rsn_initialization(struct rsn_interface *rsn_p, struct rsn_setup *setup)
{
	char *log_header = "rsn_initialization";
	struct rsn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	rsn_p->rn_op = &rsn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	rsn_p->rn_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct rs_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
