/*
 * txn.c
 * 	Implementation of Transfer Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "txn_if.h"

struct allocator_interface;
struct txn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct tx_node *
txn_get_node(struct txn_interface *txn_p)
{
	struct txn_private *priv_p = (struct txn_private *)txn_p->tn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct tx_node *tnp;

	nhp = node_p->n_op->n_get_node(node_p);
	tnp = (struct tx_node *)nhp;
	return tnp;
}

static void
txn_put_node(struct txn_interface *txn_p, struct tx_node *tnp)
{
	struct txn_private *priv_p = (struct txn_private *)txn_p->tn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)tnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct txn_operations txn_op = {
	.tn_get_node = txn_get_node,
	.tn_put_node = txn_put_node,
};

int
txn_initialization(struct txn_interface *txn_p, struct txn_setup *setup)
{
	char *log_header = "txn_initialization";
	struct txn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	txn_p->tn_op = &txn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	txn_p->tn_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct tx_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
