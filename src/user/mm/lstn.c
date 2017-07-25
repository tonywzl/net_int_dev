/*
 * lstn.c
 * 	Implementation of List Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "lstn_if.h"

struct allocator_interface;
struct lstn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct list_node *
lstn_get_node(struct lstn_interface *lstn_p)
{
	struct lstn_private *priv_p = (struct lstn_private *)lstn_p->ln_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct list_node *lnp;

	nhp = node_p->n_op->n_get_node(node_p);
	lnp = (struct list_node *)nhp;
	return lnp;
}

static void
lstn_put_node(struct lstn_interface *lstn_p, struct list_node *lnp)
{
	struct lstn_private *priv_p = (struct lstn_private *)lstn_p->ln_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)lnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct lstn_operations lstn_op = {
	.ln_get_node = lstn_get_node,
	.ln_put_node = lstn_put_node,
};

int
lstn_initialization(struct lstn_interface *lstn_p, struct lstn_setup *setup)
{
	char *log_header = "lstn_initialization";
	struct lstn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	lstn_p->ln_op = &lstn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	lstn_p->ln_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct list_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
