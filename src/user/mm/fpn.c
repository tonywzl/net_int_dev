/*
 * fpn.c
 * 	Implementation of Finger Print Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "fpn_if.h"

struct allocator_interface;
struct fpn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
	int				p_fp_size;
};

static struct fp_node *
fpn_get_node(struct fpn_interface *fpn_p)
{
	struct fpn_private *priv_p = (struct fpn_private *)fpn_p->fp_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct fp_node *np;

	nhp = node_p->n_op->n_get_node(node_p);
	np = (struct fp_node *)nhp;
	return np;
}

static void
fpn_put_node(struct fpn_interface *fpn_p, struct fp_node *np)
{
	struct fpn_private *priv_p = (struct fpn_private *)fpn_p->fp_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)np;

	node_p->n_op->n_put_node(node_p, nhp);
}

static int
fpn_get_fp_size(struct fpn_interface *fpn_p)
{
	struct fpn_private *priv_p = (struct fpn_private *)fpn_p->fp_private;
	return priv_p->p_fp_size;	
}

struct fpn_operations fpn_op = {
	.fp_get_node = fpn_get_node,
	.fp_put_node = fpn_put_node,
	.fp_get_fp_size = fpn_get_fp_size,
};

int
fpn_initialization(struct fpn_interface *fpn_p, struct fpn_setup *setup)
{
	char *log_header = "fpn_initialization";
	struct fpn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	fpn_p->fp_op = &fpn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	fpn_p->fp_private = priv_p;

	priv_p->p_fp_size = setup->fp_size;
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct fp_node) + priv_p->p_fp_size;
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 

	return 0;
}
