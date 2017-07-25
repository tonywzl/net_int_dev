/*
 * ddn.c
 * 	Implementation of Data Description Node Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "ddn_if.h"

struct allocator_interface;
struct ddn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct data_description_node *
ddn_get_node(struct ddn_interface *ddn_p)
{
	struct ddn_private *priv_p = (struct ddn_private *)ddn_p->d_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct data_description_node *bnp;

	nhp = node_p->n_op->n_get_node(node_p);
	bnp = (struct data_description_node *)nhp;
	bnp->d_user_counter = 1;
	return bnp;
}

static int
ddn_put_node(struct ddn_interface *ddn_p, struct data_description_node *bnp)
{
	struct ddn_private *priv_p = (struct ddn_private *)ddn_p->d_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)bnp;
	int left = __sync_sub_and_fetch(&bnp->d_user_counter, 1);
	if (left <= 0) {
		/* left == 0 means subtract user_counter to 0 by ddn_put_node;
		 * left == -1 means there is some body still hold the node when first time call put_node
		 * and then the user_counter decrease to zero, call this again and got -1 */
		assert(left == 0 || left == -1);
		node_p->n_op->n_put_node(node_p, nhp);
	}
	return left;
}

static void
ddn_destroy(struct ddn_interface *ddn_p)
{
	struct ddn_private *priv_p = (struct ddn_private *)ddn_p->d_private;
	struct node_interface *node_p = &priv_p->p_node;

	node_p->n_op->n_destroy(node_p);
	free((void *)priv_p);
}

struct ddn_operations ddn_op = {
	.d_get_node = ddn_get_node,
	.d_put_node = ddn_put_node,
	.d_destroy = ddn_destroy,
};

int
ddn_initialization(struct ddn_interface *ddn_p, struct ddn_setup *setup)
{
	char *log_header = "ddn_initialization";
	struct ddn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	ddn_p->d_op = &ddn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	ddn_p->d_private = priv_p;

	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct data_description_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
