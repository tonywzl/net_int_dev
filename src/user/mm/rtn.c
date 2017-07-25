/*
 * rtn.c
 * 	Implementation of  rtn Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "node_if.h"
#include "rtn_if.h"

struct rtn_private {
	struct node_interface	p_node;
	uint64_t	p_nused;
	uint32_t	p_segsz;
};

static struct rtree_node *
rtn_get_node(struct rtn_interface *rtn_p)
{
	struct rtn_private *priv_p = (struct rtn_private *)rtn_p->rt_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp;
	struct rtree_node *bnp;

	nhp = node_p->n_op->n_get_node(node_p);
	bnp = (struct rtree_node *)nhp;
	priv_p->p_nused++;
	return bnp;
}

static void
rtn_put_node(struct rtn_interface *rtn_p, struct rtree_node *bnp)
{
	struct rtn_private *priv_p = (struct rtn_private *)rtn_p->rt_private;
	struct node_interface *node_p = &priv_p->p_node;
	struct node_header *nhp = (struct node_header *)bnp;

	node_p->n_op->n_put_node(node_p, nhp);
	priv_p->p_nused--;
}

static void
rtn_close(struct rtn_interface *rtn_p)
{
	char *log_header = "rtn_close";
	struct rtn_private *priv_p = (struct rtn_private *)rtn_p->rt_private;
	struct node_interface *node_p = &priv_p->p_node;
	nid_log_notice("%s: start (rtn_p:%p) ...", log_header, rtn_p);
#if 0
	// rtn can only safely close, when no node in use
	if(priv_p->p_nused) {
		nid_log_warning("%s: %lu nodes in used state", log_header, priv_p->p_nused);
		assert(0);
	}
#endif

	// destroy node
	node_p->n_op->n_destroy(node_p);

	free(priv_p);
	rtn_p->rt_private = NULL;
}

static void
rtn_get_stat(struct rtn_interface *rtn_p, struct rtn_stat *stat)
{
	struct rtn_private *priv_p = (struct rtn_private *)rtn_p->rt_private;
	assert(priv_p);
	stat->s_segsz = priv_p->p_segsz;
	stat->s_nseg = 0;
	stat->s_nfree = 0; 
	stat->s_nused = priv_p->p_nused;
}

struct rtn_operations rtn_op = {
	.rt_get_node = rtn_get_node,
	.rt_put_node = rtn_put_node,
	.rt_close = rtn_close,
	.rt_get_stat = rtn_get_stat,
};

int
rtn_initialization(struct rtn_interface *rtn_p, struct rtn_setup *setup)
{
	char *log_header = "rtn_initialization";
	struct rtn_private *priv_p;
	struct node_setup node_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	rtn_p->rt_op = &rtn_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	rtn_p->rt_private = priv_p;
	
	node_setup.allocator = NULL;
	node_setup.a_set_id = 0;
	node_setup.alignment = 0;
	node_setup.node_size = sizeof(struct rtree_node);
	node_setup.seg_size = 1024;
	node_initialization(&priv_p->p_node, &node_setup); 
	priv_p->p_nused = 0;
	priv_p->p_segsz = node_setup.seg_size;
	return 0;
}
