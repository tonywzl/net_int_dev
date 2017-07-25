/*
 * pp_if.h
 * 	Interface of Page Pool Module
 */
#ifndef NID_PP_IF_H
#define NID_PP_IF_H

#include "stdint.h"
#include "list.h"
#include "lck_if.h"
#include "nid_shared.h"

#define PP_WHERE_FREE		1
#define PP_WHERE_PREFLUSH	2
#define PP_WHERE_FLUSH		3
#define PP_WHERE_FLUSHING	4
#define PP_WHERE_FLUSHING_DONE	5
#define PP_WHERE_RELEASE	6

struct pp_page_node {
	void			*pp_handle;
	void			*pp_page_handle;
	struct list_head	pp_list;
	struct list_head	pp_rlist;
	struct list_head	pp_flush_head;
	struct list_head	pp_release_head;
	uint64_t		pp_start_seq;
	uint64_t		pp_last_flush_seq;
	uint32_t		pp_occupied;
	uint32_t		pp_flushed;
	volatile int		pp_coalesced;
	void			*pp_page;
	void			*pp_tfn;	// transfer flush node
	struct lck_node		pp_lnode;
	uint8_t			pp_where;
	uint8_t			pp_oob;
};

struct pp_interface;
struct pp_operations {
	struct pp_page_node*	(*pp_get_node)(struct pp_interface *);
	struct pp_page_node*	(*pp_get_node_timed)(struct pp_interface *, int);
	struct pp_page_node*	(*pp_get_node_nowait)(struct pp_interface *);
	struct pp_page_node*	(*pp_get_node_forcibly)(struct pp_interface *);
	void			(*pp_free_node)(struct pp_interface *, struct pp_page_node *);
	uint32_t		(*pp_get_pageszshift)(struct pp_interface *);
	uint32_t		(*pp_get_pagesz)(struct pp_interface *);
	uint32_t		(*pp_get_poolsz)(struct pp_interface *);
	uint32_t		(*pp_get_poollen)(struct pp_interface *);
	uint32_t		(*pp_get_nfree)(struct pp_interface *);
	uint32_t		(*pp_get_avail)(struct pp_interface *);
	char*			(*pp_get_name)(struct pp_interface *);
	struct pp_page_node*	(*pp_get_node_nondata)(struct pp_interface *);
	void			(*pp_free_data)(struct pp_interface *, struct pp_page_node *);
	void			(*pp_destroy)(struct pp_interface *);
};

struct pp_interface {
	void			*pp_private;
	struct pp_operations	*pp_op;
};

struct allocator_interface;
struct pp_setup {
	char				pp_name[NID_MAX_PPNAME];
	struct allocator_interface	*allocator;
	int				set_id;
	uint32_t			page_size;
	uint32_t			pool_size;
};

extern int pp_initialization(struct pp_interface *, struct pp_setup *);
#endif
