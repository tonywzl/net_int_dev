
#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "mrw_rn_if.h"
#include "allocator_if.h"

struct mrw_rn_private {
	struct allocator_interface	*p_allocator;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct mrw_rn *
mrw_rn_get_node(struct mrw_rn_interface *mrw_rn_p)
{
	struct mrw_rn_private *priv_p = (struct mrw_rn_private *)mrw_rn_p->rn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct mrw_rn *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct mrw_rn *)nhp;
	return dnp;
}

static void
mrw_rn_put_node(struct mrw_rn_interface *mrw_rn_p, struct mrw_rn *dnp)
{
	struct mrw_rn_private *priv_p = (struct mrw_rn_private *)mrw_rn_p->rn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct mrw_rn_operations mrw_rn_op = {
	.rn_get_node = mrw_rn_get_node,
	.rn_put_node = mrw_rn_put_node,
};

int
mrw_rn_initialization(struct mrw_rn_interface *mrw_rn_p, struct mrw_rn_setup *setup)
{
	char *log_header = "mrw_rn_initialization";
	struct mrw_rn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	mrw_rn_p->rn_op = &mrw_rn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	mrw_rn_p->rn_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = ALLOCATOR_SET_MRW_RN;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct mrw_rn);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup);
	return 0;
}
