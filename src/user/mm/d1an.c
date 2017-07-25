/*
 * d1an.c
 * 	Implementation of One Data (d1) type A Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "d1an_if.h"

struct allocator_interface;
struct d1an_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	int				p_node_size;
	struct node_interface		p_node;
};

static struct d1a_node *
d1an_get_node(struct d1an_interface *d1an_p)
{
	struct d1an_private *priv_p = (struct d1an_private *)d1an_p->d1_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct d1a_node *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct d1a_node *)nhp;
	return dnp;
}

static void
d1an_put_node(struct d1an_interface *d1an_p, struct d1a_node *dnp)
{
	struct d1an_private *priv_p = (struct d1an_private *)d1an_p->d1_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

static void
d1an_destroy(struct d1an_interface *d1an_p)
{
	struct d1an_private *priv_p = (struct d1an_private *)d1an_p->d1_private;
	struct node_interface *node_p = &priv_p->p_node;

	node_p->n_op->n_destroy(node_p);
	free((void *)priv_p);
}

struct d1an_operations d1an_op = {
	.d1_get_node = d1an_get_node,
	.d1_put_node = d1an_put_node,
	.d1_destroy = d1an_destroy,
};

int
d1an_initialization(struct d1an_interface *d1an_p, struct d1an_setup *setup)
{
	char *log_header = "d1an_initialization";
	struct d1an_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	d1an_p->d1_op = &d1an_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	d1an_p->d1_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;
	priv_p->p_node_size = setup->node_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = priv_p->p_node_size;
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
