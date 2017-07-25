/*
 * bwc_rbn.c
 * 	Implementation of BWC Recover Buffer Node Module
 */

#include <stdlib.h>
#include <unistd.h>

#include "nid_log.h"
#include "node_if.h"
#include "bwc_rbn_if.h"

struct allocator_interface;
struct bwc_rbn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct bwc_rb_node *
bwc_rbn_get_node(struct bwc_rbn_interface *bwc_rbn_p)
{
	struct bwc_rbn_private *priv_p = (struct bwc_rbn_private *)bwc_rbn_p->nn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct bwc_rb_node *nnp;

	nhp = node_p->n_op->n_get_node(node_p);
	nnp = (struct bwc_rb_node *)nhp;

	return nnp;
}

static void
bwc_rbn_put_node(struct bwc_rbn_interface *bwc_rbn_p, struct bwc_rb_node *nnp)
{
	struct bwc_rbn_private *priv_p = (struct bwc_rbn_private *)bwc_rbn_p->nn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)nnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

static void
bwc_rbn_destroy(struct bwc_rbn_interface *bwc_rbn_p)
{
	struct bwc_rbn_private *priv_p = (struct bwc_rbn_private *)bwc_rbn_p->nn_private;
	struct node_interface *node_p = &priv_p->p_node;

	node_p->n_op->n_destroy(node_p);
	free((void *)priv_p);
}

struct bwc_rbn_operations bwc_rbn_op = {
	.nn_get_node = bwc_rbn_get_node,
	.nn_put_node = bwc_rbn_put_node,
	.nn_destroy = bwc_rbn_destroy,
};

int
bwc_rbn_initialization(struct bwc_rbn_interface *bwc_rbn_p, struct bwc_rbn_setup *setup)
{
	char *log_header = "bwc_rbn_initialization";
	struct bwc_rbn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	bwc_rbn_p->nn_op = &bwc_rbn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bwc_rbn_p->nn_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = setup->alignment;
	node_setup.node_size = sizeof(struct bwc_rb_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
