/*
 * rsun.c
 * 	Implementation of Resource Unit Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "rsun_if.h"

struct allocator_interface;
struct rsun_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct rsu_node *
rsun_get_node(struct rsun_interface *rsun_p)
{
	struct rsun_private *priv_p = (struct rsun_private *)rsun_p->un_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct rsu_node *lnp;

	nhp = node_p->n_op->n_get_node(node_p);
	lnp = (struct rsu_node *)nhp;
	return lnp;
}

static void
rsun_put_node(struct rsun_interface *rsun_p, struct rsu_node *lnp)
{
	struct rsun_private *priv_p = (struct rsun_private *)rsun_p->un_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)lnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct rsun_operations rsun_op = {
	.un_get_node = rsun_get_node,
	.un_put_node = rsun_put_node,
};

int
rsun_initialization(struct rsun_interface *rsun_p, struct rsun_setup *setup)
{
	char *log_header = "rsun_initialization";
	struct rsun_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	rsun_p->un_op = &rsun_op;
	priv_p = calloc(1, sizeof(*priv_p));
	rsun_p->un_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct rsu_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
