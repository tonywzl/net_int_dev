/*
 * cdn.c
 * 	Implementation of Content Description Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "cdn_if.h"

struct allocator_interface;
struct cdn_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	int				p_seg_size;
	struct node_interface		p_node;
	int				p_fp_size;
};

static struct content_description_node *
cdn_get_node(struct cdn_interface *cdn_p)
{
	struct cdn_private *priv_p = (struct cdn_private *)cdn_p->cn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct content_description_node *bnp;

	nhp = node_p->n_op->n_get_node(node_p);
	bnp = (struct content_description_node *)nhp;
	return bnp;
}

static void
cdn_put_node(struct cdn_interface *cdn_p, struct content_description_node *bnp)
{
	struct cdn_private *priv_p = (struct cdn_private *)cdn_p->cn_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)bnp;

	node_p->n_op->n_put_node(node_p, nhp);
}

struct cdn_operations cdn_op = {
	.cn_get_node = cdn_get_node,
	.cn_put_node = cdn_put_node,
};

int
cdn_initialization(struct cdn_interface *cdn_p, struct cdn_setup *setup)
{
	char *log_header = "cdn_initialization";
	struct cdn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	cdn_p->cn_op = &cdn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	cdn_p->cn_private = priv_p;
	
	priv_p->p_fp_size = setup->fp_size;
	priv_p->p_allocator = setup->allocator;
	priv_p->p_set_id = setup->set_id;
	priv_p->p_seg_size = setup->seg_size;

	node_setup.allocator = priv_p->p_allocator;
	node_setup.a_set_id = priv_p->p_set_id;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct content_description_node) + priv_p->p_fp_size;
	node_setup.seg_size = priv_p->p_seg_size;
	node_initialization(&priv_p->p_node, &node_setup);
	return 0;
}
