/*
 * d2an.c
 * 	Implementation of two Data (d2) type A Node Module 
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "d2an_if.h"

struct allocator_interface;
struct d2an_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct d2a_node *
d2an_get_node(struct d2an_interface *d2an_p)
{
	struct d2an_private *priv_p = (struct d2an_private *)d2an_p->d2_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct d2a_node *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct d2a_node *)nhp;
	return dnp;
}

static void
d2an_put_node(struct d2an_interface *d2an_p, struct d2a_node *dnp)
{
	struct d2an_private *priv_p = (struct d2an_private *)d2an_p->d2_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

static void
d2an_destroy(struct d2an_interface *d2an_p)
{
	struct d2an_private *priv_p = (struct d2an_private *)d2an_p->d2_private;
	struct node_interface *node_p = &priv_p->p_node;

	node_p->n_op->n_destroy(node_p);
	free((void *)priv_p);
}

struct d2an_operations d2an_op = {
	.d2_get_node = d2an_get_node,
	.d2_put_node = d2an_put_node,
	.d2_destroy = d2an_destroy,
};

int
d2an_initialization(struct d2an_interface *d2an_p, struct d2an_setup *setup)
{
	char *log_header = "d2an_initialization";
	struct d2an_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	d2an_p->d2_op = &d2an_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	d2an_p->d2_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct d2a_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
