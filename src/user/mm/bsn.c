/*
 * bsn.c
 * 	Implementation of BIO Search Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "bsn_if.h"

struct allocator_interface;
struct bsn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct bsn_node *
bsn_get_node(struct bsn_interface *bsn_p)
{
	struct bsn_private *priv_p = (struct bsn_private *)bsn_p->bn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct bsn_node *bnp;

	nhp = node_p->n_op->n_get_node(node_p);
	bnp = (struct bsn_node *)nhp;
	return bnp;
}

static void
bsn_put_node(struct bsn_interface *bsn_p, struct bsn_node *bnp)
{
	struct bsn_private *priv_p = (struct bsn_private *)bsn_p->bn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)bnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct bsn_operations bsn_op = {
	.bn_get_node = bsn_get_node,
	.bn_put_node = bsn_put_node,
};

int
bsn_initialization(struct bsn_interface *bsn_p, struct bsn_setup *setup)
{
	char *log_header = "bsn_initialization";
	struct bsn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	bsn_p->bn_op = &bsn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bsn_p->bn_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct bsn_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
