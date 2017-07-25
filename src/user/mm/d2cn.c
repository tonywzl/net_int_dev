/*
 * d2cn.c
 * 	Implementation of two Data (d2) type C Node Module 
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "d2cn_if.h"

struct allocator_interface;
struct d2cn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct d2c_node *
d2cn_get_node(struct d2cn_interface *d2cn_p)
{
	struct d2cn_private *priv_p = (struct d2cn_private *)d2cn_p->d2_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct d2c_node *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct d2c_node *)nhp;
	return dnp;
}

static void
d2cn_put_node(struct d2cn_interface *d2cn_p, struct d2c_node *dnp)
{
	struct d2cn_private *priv_p = (struct d2cn_private *)d2cn_p->d2_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

static void
d2cn_destroy(struct d2cn_interface *d2cn_p)
{
	struct d2cn_private *priv_p = (struct d2cn_private *)d2cn_p->d2_private;
	struct node_interface *node_p = &priv_p->p_node;

	node_p->n_op->n_destroy(node_p);
	free((void *)priv_p);
}

struct d2cn_operations d2cn_op = {
	.d2_get_node = d2cn_get_node,
	.d2_put_node = d2cn_put_node,
	.d2_destroy = d2cn_destroy,
};

int
d2cn_initialization(struct d2cn_interface *d2cn_p, struct d2cn_setup *setup)
{
	char *log_header = "d2cn_initialization";
	struct d2cn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	d2cn_p->d2_op = &d2cn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	d2cn_p->d2_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct d2c_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
