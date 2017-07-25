
#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "mrw_wn_if.h"
#include "allocator_if.h"

struct mrw_wn_private {
	struct allocator_interface	*p_allocator;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct mrw_wn *
mrw_wn_get_node(struct mrw_wn_interface *mrw_wn_p)
{
	struct mrw_wn_private *priv_p = (struct mrw_wn_private *)mrw_wn_p->wn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct mrw_wn *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct mrw_wn *)nhp;
	return dnp;
}

static void
mrw_wn_put_node(struct mrw_wn_interface *mrw_wn_p, struct mrw_wn *dnp)
{
	struct mrw_wn_private *priv_p = (struct mrw_wn_private *)mrw_wn_p->wn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct mrw_wn_operations mrw_wn_op = {
	.wn_get_node = mrw_wn_get_node,
	.wn_put_node = mrw_wn_put_node,
};

int
mrw_wn_initialization(struct mrw_wn_interface *mrw_wn_p, struct mrw_wn_setup *setup)
{
	char *log_header = "mrw_wn_initialization";
	struct mrw_wn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	mrw_wn_p->wn_op = &mrw_wn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	mrw_wn_p->wn_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = ALLOCATOR_SET_MRW_WN;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct mrw_wn);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup);
	return 0;
}
