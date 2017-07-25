/*
 * rsugn.c
 * 	Implementation of Resource Unit Group Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "rsugn_if.h"

struct allocator_interface;
struct rsugn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct rsug_node *
rsugn_get_node(struct rsugn_interface *rsugn_p)
{
	struct rsugn_private *priv_p = (struct rsugn_private *)rsugn_p->gn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct rsug_node *nnp;

	nhp = node_p->n_op->n_get_node(node_p);
	nnp = (struct rsug_node *)nhp;
	return nnp;
}

static void
rsugn_put_node(struct rsugn_interface *rsugn_p, struct rsug_node *nnp)
{
	struct rsugn_private *priv_p = (struct rsugn_private *)rsugn_p->gn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)nnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

static void
rsugn_destroy(struct rsugn_interface *rsugn_p)
{
	struct rsugn_private *priv_p = (struct rsugn_private *)rsugn_p->gn_private;
	struct node_interface *node_p = &priv_p->p_node;

	node_p->n_op->n_destroy(node_p);
	free((void *)priv_p);
}

struct rsugn_operations rsugn_op = {
	.gn_get_node = rsugn_get_node,
	.gn_put_node = rsugn_put_node,
	.gn_destroy = rsugn_destroy,
};

int
rsugn_initialization(struct rsugn_interface *rsugn_p, struct rsugn_setup *setup)
{
	char *log_header = "rsugn_initialization";
	struct rsugn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	rsugn_p->gn_op = &rsugn_op;
	priv_p = calloc(1, sizeof(*priv_p));
	rsugn_p->gn_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct rsug_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
