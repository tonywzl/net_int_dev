
#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "mrw_wsn_if.h"
#include "allocator_if.h"

struct mrw_wsn_private {
	struct allocator_interface	*p_allocator;
	int				p_seg_size;
	struct node_interface		p_node;
};

static struct mrw_wsn *
mrw_wsn_get_node(struct mrw_wsn_interface *mrw_wsn_p)
{
	struct mrw_wsn_private *priv_p = (struct mrw_wsn_private *)mrw_wsn_p->wsn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct mrw_wsn *dnp;

	nhp = node_p->n_op->n_get_node(node_p);
	dnp = (struct mrw_wsn *)nhp;
	return dnp;
}

static void
mrw_wsn_put_node(struct mrw_wsn_interface *mrw_wsn_p, struct mrw_wsn *dnp)
{
	struct mrw_wsn_private *priv_p = (struct mrw_wsn_private *)mrw_wsn_p->wsn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)dnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct mrw_wsn_operations mrw_wsn_op = {
	.wsn_get_node = mrw_wsn_get_node,
	.wsn_put_node = mrw_wsn_put_node,
};

int
mrw_wsn_initialization(struct mrw_wsn_interface *mrw_wsn_p, struct mrw_wsn_setup *setup)
{
	char *log_header = "mrw_wsn_initialization";
	struct mrw_wsn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	mrw_wsn_p->wsn_op = &mrw_wsn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	mrw_wsn_p->wsn_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = ALLOCATOR_SET_MRW_WSN;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct mrw_wsn);
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup);
	return 0;
}
