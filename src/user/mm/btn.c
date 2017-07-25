/*
 * btn.c
 * 	Implementation of Bse Tree Slot Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "btn_if.h"

struct allocator_interface;
struct btn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct btn_node *
btn_get_node(struct btn_interface *btn_p)
{
	struct btn_private *priv_p = (struct btn_private *)btn_p->bt_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct btn_node *bnp;

	nhp = node_p->n_op->n_get_node(node_p);
	bnp = (struct btn_node *)nhp;
	return bnp;
}

static void
btn_put_node(struct btn_interface *btn_p, struct btn_node *bnp)
{
	struct btn_private *priv_p = (struct btn_private *)btn_p->bt_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)bnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

static void
btn_destroy(struct btn_interface *btn_p)
{
	struct btn_private *priv_p = (struct btn_private *)btn_p->bt_private;
	struct node_interface *node_p = &priv_p->p_node;

	node_p->n_op->n_destroy(node_p);
	free((void *)priv_p);
}

struct btn_operations btn_op = {
	.bt_get_node = btn_get_node,
	.bt_put_node = btn_put_node,
	.bt_destroy = btn_destroy,
};

int
btn_initialization(struct btn_interface *btn_p, struct btn_setup *setup)
{
	char *log_header = "btn_initialization";
	struct btn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	btn_p->bt_op = &btn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	btn_p->bt_private = priv_p;
	
	priv_p->p_allocator = setup->allocator;
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct btn_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
