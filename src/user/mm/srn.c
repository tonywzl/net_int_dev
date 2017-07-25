/*
 * srn.c
 * 	Implementation of  srn Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "srn_if.h"

struct allocator_interface;
struct srn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct sub_request_node *
srn_get_node(struct srn_interface *srn_p)
{
	struct srn_private *priv_p = (struct srn_private *)srn_p->sr_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct sub_request_node *bnp;

	nhp = node_p->n_op->n_get_node(node_p);
	bnp = (struct sub_request_node *)nhp;
	return bnp;
}

static void
srn_put_node(struct srn_interface *srn_p, struct sub_request_node *bnp)
{
	struct srn_private *priv_p = (struct srn_private *)srn_p->sr_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)bnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct srn_operations srn_op = {
	.sr_get_node = srn_get_node,
	.sr_put_node = srn_put_node,
};

int
srn_initialization(struct srn_interface *srn_p, struct srn_setup *setup)
{
	char *log_header = "srn_initialization";
	struct srn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	srn_p->sr_op = &srn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	srn_p->sr_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;
	
	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id; 
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct sub_request_node);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup); 
	return 0;
}
