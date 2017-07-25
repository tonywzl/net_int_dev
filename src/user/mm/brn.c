/*
 * brn.c
 * 	Implementation of Brigde Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "brn_if.h"

struct allocator_interface;
struct brn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct bridge_node *
brn_get_node(struct brn_interface *brn_p)
{
	struct brn_private *priv_p = (struct brn_private *)brn_p->bn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct bridge_node *bnp;

	nhp = node_p->n_op->n_get_node(node_p);
	bnp = (struct bridge_node *)nhp;
	return bnp;
}

static void
brn_put_node(struct brn_interface *brn_p, struct bridge_node *bnp)
{
	struct brn_private *priv_p = (struct brn_private *)brn_p->bn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)bnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct brn_operations brn_op = {
	.bn_get_node = brn_get_node,
	.bn_put_node = brn_put_node,
};

int
brn_initialization(struct brn_interface *brn_p, struct brn_setup *setup)
{
	char *log_header = "brn_initialization";
	struct brn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	brn_p->bn_op = &brn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	brn_p->bn_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct bridge_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
