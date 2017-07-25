/*
 * rc_bwcn.c
 * 	Implementation of two Data (d2) type C Node Module 
 */

#include "rc_wcn.h"

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"

struct allocator_interface;
struct rc_bwcn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct rc_wc_node *
rc_bwcn_get_node(struct rc_wcn_interface *rc_bwcn_p)
{
	struct rc_bwcn_private *priv_p = (struct rc_bwcn_private *)rc_bwcn_p->rm_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct rc_wc_node *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct rc_wc_node *)nhp;
	return dnp;
}

static void
rc_bwcn_put_node(struct rc_wcn_interface *rc_bwcn_p, struct rc_wc_node *dnp)
{
	struct rc_bwcn_private *priv_p = (struct rc_bwcn_private *)rc_bwcn_p->rm_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

static void
rc_bwcn_destroy(struct rc_wcn_interface *rc_bwcn_p)
{
	struct rc_bwcn_private *priv_p = (struct rc_bwcn_private *)rc_bwcn_p->rm_private;
	struct node_interface *node_p = &priv_p->p_node;

	node_p->n_op->n_destroy(node_p);
	free((void *)priv_p);
}

struct rc_wcn_operations rc_bwcn_op = {
	.rm_get_node = rc_bwcn_get_node,
	.rm_put_node = rc_bwcn_put_node,
	.rm_destroy = rc_bwcn_destroy,
};

int
rc_wcn_initialization(struct rc_wcn_interface *rc_bwcn_p, struct rc_wcn_setup *setup)
{
	char *log_header = "rc_bwcn_initialization";
	struct rc_bwcn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	rc_bwcn_p->rm_op = &rc_bwcn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	rc_bwcn_p->rm_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct rc_wc_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
