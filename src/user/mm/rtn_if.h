/*
 * rtn_if.h
 * 	Interface of Radix Tree Node Module
 */
#ifndef NID_RTN_IF_H
#define NID_RTN_IF_H

#include <stdint.h>
#include <list.h>
#include <node_if.h>

#define RTREE_MAX_MAP_SIZE	64

struct rtn_stat {
	uint32_t	s_segsz;
	uint32_t	s_nseg;
	uint32_t	s_nfree;
	uint32_t	s_nused;
};

struct rtn_segment;
struct rtree_node {
	struct node_header	n_header;
	uint16_t		n_height;
	uint64_t		n_bitmap;
	uint64_t		n_count;
	struct rtree_node	*n_parent;
	void			*n_slots[RTREE_MAX_MAP_SIZE];
};

struct rtn_interface;
struct rtn_operations {
	struct rtree_node*	(*rt_get_node)(struct rtn_interface *);
	void			(*rt_put_node)(struct rtn_interface *, struct rtree_node *);
	void			(*rt_close)(struct rtn_interface *);
	void			(*rt_get_stat)(struct rtn_interface *, struct rtn_stat *);
};

struct rtn_interface {
	void			*rt_private;
	struct rtn_operations	*rt_op;
};

struct rtn_setup {
};

extern int rtn_initialization(struct rtn_interface *, struct rtn_setup *);

#endif
