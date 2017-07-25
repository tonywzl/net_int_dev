/*
 * rtree_if.h
 * 	Interface of Radix Tree Module
 */
#ifndef NID_RTREE_IF_H
#define NID_RTREE_IF_H

#include <stdint.h>

struct rtree_stat {
	uint32_t	s_rtn_nseg;
	uint32_t	s_rtn_segsz;
	uint32_t	s_rtn_nfree;
	uint32_t	s_rtn_nused;
};

struct rtree_interface;
struct rtree_operations {
	void*	(*rt_insert)(struct rtree_interface *, uint64_t, void *);
	void*	(*rt_hint_insert)(struct rtree_interface *, uint64_t, void *, uint64_t, void *);
	void*	(*rt_remove)(struct rtree_interface *, uint64_t, void *);
	void*	(*rt_hint_remove)(struct rtree_interface *, uint64_t, void *, void *);
	void*	(*rt_search)(struct rtree_interface *, uint64_t);
	void*	(*rt_hint_search)(struct rtree_interface *, uint64_t, uint64_t, void *);
	void*	(*rt_hint_search_around)(struct rtree_interface *, uint64_t, uint64_t, void *);
	void*	(*rt_search_around)(struct rtree_interface *, uint64_t);
	void	(*rt_get_stat)(struct rtree_interface *, struct rtree_stat *);
	void	(*rt_close)(struct rtree_interface *);
	void	(*rt_iter_start)(struct rtree_interface *);
	void	(*rt_iter_stop)(struct rtree_interface *);
	void*	(*rt_iter_next)(struct rtree_interface *);
};

struct rtree_interface {
	void			*rt_private;
	struct rtree_operations	*rt_op;
};

struct allocator_interface;
struct rtree_setup {
	struct allocator_interface	*allocator;
	void				(*extend_cb)(void *, void *);
	void*				(*insert_cb)(void *, void *, void *);
	void*				(*remove_cb)(void *, void *);
	void				(*shrink_to_root_cb)(void * );
};

extern int rtree_initialization(struct rtree_interface *, struct rtree_setup *);
#endif
