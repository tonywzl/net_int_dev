/*
 * blksn.c
 * 	Implementation of Block Node With Variable Size Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "blksn_if.h"

struct allocator_interface;
struct blksn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct block_size_node *
blksn_get_node(struct blksn_interface *blksn_p)
{
	struct blksn_private *priv_p = (struct blksn_private *)blksn_p->bsn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct block_size_node *bnp;

	nhp = node_p->n_op->n_get_node(node_p);
	bnp = (struct block_size_node *)nhp;
	bnp->bsn_list.prev = NULL;
	bnp->bsn_list.next = NULL;
	return bnp;
}

static void
blksn_put_node(struct blksn_interface *blksn_p, struct block_size_node *bnp)
{
	struct blksn_private *priv_p = (struct blksn_private *)blksn_p->bsn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)bnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct blksn_operations blksn_op = {
	.bsn_get_node = blksn_get_node,
	.bsn_put_node = blksn_put_node,
};

int
blksn_initialization(struct blksn_interface *blksn_p, struct blksn_setup *setup)
{
	char *log_header = "blksn_initialization";
	struct blksn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	blksn_p->bsn_op = &blksn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	blksn_p->bsn_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct block_size_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
