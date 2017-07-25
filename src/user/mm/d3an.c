/*
 * d3an.c
 * 	Implementation of two Data (d3) type A Node Module 
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "d3an_if.h"

struct allocator_interface;
struct d3an_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct d3a_node *
d3an_get_node(struct d3an_interface *d3an_p)
{
	struct d3an_private *priv_p = (struct d3an_private *)d3an_p->d3_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct d3a_node *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct d3a_node *)nhp;
	return dnp;
}

static void
d3an_put_node(struct d3an_interface *d3an_p, struct d3a_node *dnp)
{
	struct d3an_private *priv_p = (struct d3an_private *)d3an_p->d3_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct d3an_operations d3an_op = {
	.d3_get_node = d3an_get_node,
	.d3_put_node = d3an_put_node,
};

int
d3an_initialization(struct d3an_interface *d3an_p, struct d3an_setup *setup)
{
	char *log_header = "d3an_initialization";
	struct d3an_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	d3an_p->d3_op = &d3an_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	d3an_p->d3_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct d3a_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
