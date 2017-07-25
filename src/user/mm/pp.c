/*
 * pp.c
 * 	Implementation of Page Pool Module
 */

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#include "list.h"
#include "nid_log.h"
#include "allocator_if.h"
#include "pp_if.h"
#include "nid_shared.h"

#define DEFAULT_PAGESZSHIFT	23	// 8M
#define DEFAULT_POOLSIZE	1024

struct pp_private {
	struct allocator_interface	*p_allocator;
	int				p_set_id;
	struct list_head		p_free_head;
	pthread_mutex_t			p_lck;
	pthread_cond_t			p_free_cond;
	uint32_t			p_poolsz;	// pool size: maximum number of page node
	uint32_t			p_poollen;	// pool len: current number of pages
	uint32_t			p_extend;
	uint32_t			p_nfree;	// number of nodes in the free list
	uint32_t			p_pagesz;	// page size: must be power of 2
	uint8_t				p_pageszshift;
	char				p_name[NID_MAX_PPNAME];
};

static void
_pp_extend(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct pp_page_node *np = NULL;
	int i, to_extend;
	void *a_handle;

	nid_log_debug("_pp_extend: start (len:%d, size:%d)...", 
		priv_p->p_poollen, priv_p->p_poolsz);
	if (priv_p->p_poollen >= priv_p->p_poolsz)
		return;
	if (priv_p->p_poollen + priv_p->p_extend > priv_p->p_poolsz)
		to_extend = priv_p->p_poolsz - priv_p->p_poollen;
	else
		to_extend = priv_p->p_extend;

	for (i = 0; i < to_extend; i++) {
		a_handle = allocator_p->a_op->a_calloc(allocator_p, priv_p->p_set_id, 1,
			sizeof(*np));
		np = allocator_p->a_op->a_get_data(allocator_p, a_handle);
		np->pp_handle = a_handle;

		a_handle = allocator_p->a_op->a_memalign(allocator_p, priv_p->p_set_id,
			getpagesize(), priv_p->p_pagesz);
		np->pp_page = allocator_p->a_op->a_get_data(allocator_p, a_handle);
		np->pp_page_handle = a_handle;

		np->pp_where = PP_WHERE_FREE;
		np->pp_oob = 0;
		pthread_mutex_lock(&priv_p->p_lck);
		if (priv_p->p_poollen < priv_p->p_poolsz) {
			list_add(&np->pp_list, &priv_p->p_free_head);
			priv_p->p_poollen++;
			priv_p->p_nfree++;
		} else {
			allocator_p->a_op->a_free(allocator_p, np->pp_page_handle);
			allocator_p->a_op->a_free(allocator_p, np->pp_handle);
			np = NULL;
		}
		pthread_mutex_unlock(&priv_p->p_lck);
		if (!np)
			break;
	}
}

static struct pp_page_node *
pp_get_node(struct pp_interface *pp_p)
{
	char *log_header = "pp_get_node";
	struct pp_private *priv_p = pp_p->pp_private;
	struct list_head *free_lp = &priv_p->p_free_head;
	struct pp_page_node *np;

	pthread_mutex_lock(&priv_p->p_lck);
	while (!priv_p->p_nfree && priv_p->p_poolsz > priv_p->p_poollen) {
		pthread_mutex_unlock(&priv_p->p_lck);
		_pp_extend(pp_p);
		pthread_mutex_lock(&priv_p->p_lck);
	}

	while (!priv_p->p_nfree) {
		/*
		 * the pagefer pool is not extendable, we have to wait for 
		 * other threads to release  pagefers
		 */
		nid_log_notice("%s: out of page", log_header);
		pthread_cond_wait(&priv_p->p_free_cond, &priv_p->p_lck);
	}

	np = list_first_entry(free_lp, struct pp_page_node, pp_list);
	list_del(&np->pp_list);
	priv_p->p_nfree--;
	pthread_mutex_unlock(&priv_p->p_lck);
	nid_log_info("%s: np:%p, p_poollen:%u, p_nfree:%u", log_header, np, priv_p->p_poollen, priv_p->p_nfree);
	assert(np->pp_where == PP_WHERE_FREE);
	return np;
}

static struct pp_page_node *
pp_get_node_timed(struct pp_interface *pp_p, int wait_secs)
{
	char *log_header = "pp_get_node_timed";
	struct pp_private *priv_p = pp_p->pp_private;
	struct list_head *free_lp = &priv_p->p_free_head;
	struct pp_page_node *np = NULL;
	struct timeval now;
	struct timespec ts;

	pthread_mutex_lock(&priv_p->p_lck);
	while (!priv_p->p_nfree && priv_p->p_poolsz > priv_p->p_poollen) {
		pthread_mutex_unlock(&priv_p->p_lck);
		_pp_extend(pp_p);
		pthread_mutex_lock(&priv_p->p_lck);
	}

	if (!priv_p->p_nfree) {
		/*
		 * the pagefer pool is not extendable, we have to wait for
		 * other threads to release  pagefers
		 */
		nid_log_notice("%s: out of page", log_header);
		gettimeofday(&now, NULL);
		ts.tv_sec = now.tv_sec + (__time_t)wait_secs;
		ts.tv_nsec = now.tv_usec * 1000;
		pthread_cond_timedwait(&priv_p->p_free_cond, &priv_p->p_lck, &ts);
	}

	if (priv_p->p_nfree) {
		np = list_first_entry(free_lp, struct pp_page_node, pp_list);
		assert(np->pp_where == PP_WHERE_FREE);
		list_del(&np->pp_list);
		priv_p->p_nfree--;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	nid_log_info("%s: np:%p, p_poollen:%u, p_nfree:%u", log_header, np, priv_p->p_poollen, priv_p->p_nfree);
	return np;
}

/*
 * Only used for recovery, not necessary to be managed by lazy-reclaim mechanism
 */
static struct pp_page_node *
pp_get_node_nondata(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct pp_page_node *np;
	void *a_handle;

	a_handle = allocator_p->a_op->a_calloc(allocator_p, priv_p->p_set_id, 1,
		sizeof(*np));
	np = allocator_p->a_op->a_get_data(allocator_p, a_handle);
	np->pp_handle = a_handle;
	return np;
}

static struct pp_page_node *
pp_get_node_nowait(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	struct list_head *free_lp = &priv_p->p_free_head;
	struct pp_page_node *np = NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	while (!priv_p->p_nfree && priv_p->p_poolsz > priv_p->p_poollen) {
		pthread_mutex_unlock(&priv_p->p_lck);
		_pp_extend(pp_p);
		pthread_mutex_lock(&priv_p->p_lck);
	}

	if (priv_p->p_nfree) {
		np = list_first_entry(free_lp, struct pp_page_node, pp_list);
		list_del(&np->pp_list);
		priv_p->p_nfree--;
		assert(np->pp_where == PP_WHERE_FREE);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return np;
}

static struct pp_page_node *
pp_get_node_forcibly(struct pp_interface *pp_p)
{
	char *log_header = "pp_get_node_forcibly";
	struct pp_private *priv_p = pp_p->pp_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct list_head *free_lp = &priv_p->p_free_head;
	struct pp_page_node *np;
	void *a_handle;

	pthread_mutex_lock(&priv_p->p_lck);
	while (!priv_p->p_nfree && priv_p->p_poolsz > priv_p->p_poollen) {
		pthread_mutex_unlock(&priv_p->p_lck);
		_pp_extend(pp_p);
		pthread_mutex_lock(&priv_p->p_lck);
	}

	if (!priv_p->p_nfree) {
		pthread_mutex_unlock(&priv_p->p_lck);
		/*
		 * the pagefer pool is not extendable, we have to allocate memory dynamically 
		 */
		nid_log_notice("%s: out of page, dyunamically allocate a page", log_header);
		a_handle = allocator_p->a_op->a_calloc(allocator_p, priv_p->p_set_id, 1,
			sizeof(*np));
		np = allocator_p->a_op->a_get_data(allocator_p, a_handle);
		np->pp_handle = a_handle;

		a_handle = allocator_p->a_op->a_memalign(allocator_p, priv_p->p_set_id,
			getpagesize(), priv_p->p_pagesz);
		np->pp_page = allocator_p->a_op->a_get_data(allocator_p, a_handle);
		np->pp_page_handle = a_handle;

		np->pp_where = PP_WHERE_FREE;
		np->pp_oob = 1;
	} else {
		np = list_first_entry(free_lp, struct pp_page_node, pp_list);
		list_del(&np->pp_list);
		priv_p->p_nfree--;
		pthread_mutex_unlock(&priv_p->p_lck);
		assert(np->pp_where == PP_WHERE_FREE);
		assert(np->pp_oob == 0);
	}
	nid_log_info("%s: np:%p, p_poollen:%u, p_nfree:%u", log_header, np, priv_p->p_poollen, priv_p->p_nfree);
	return np;
}

static void
pp_free_node(struct pp_interface *pp_p, struct pp_page_node *np)
{
	char *log_header = "pp_free_node";
	struct pp_private *priv_p = pp_p->pp_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct list_head *free_lp = &priv_p->p_free_head;

	/* non-data case */
	if (!np->pp_page) {
		allocator_p->a_op->a_free(allocator_p, np->pp_handle);
		return;
	}

	if (np->pp_oob) {
		allocator_p->a_op->a_free(allocator_p, np->pp_page_handle);
		allocator_p->a_op->a_free(allocator_p, np->pp_handle);
		return;
	}

	pthread_mutex_lock(&priv_p->p_lck);
	np->pp_where = PP_WHERE_FREE;
	//list_add_tail(&np->pp_list, free_lp);
	list_add(&np->pp_list, free_lp);
	priv_p->p_nfree++;
	pthread_cond_signal(&priv_p->p_free_cond);
	pthread_mutex_unlock(&priv_p->p_lck);
	nid_log_info("%s: np:%p, p_poollen:%u, p_nfree:%u", log_header, np, priv_p->p_poollen, priv_p->p_nfree);
}

/*
 * Don't really want to free the memory of the data buffer part,
 * make a new pp node to hold the buffer and add it to the free list.
 */
static void
pp_free_data(struct pp_interface *pp_p, struct pp_page_node *np)
{
	struct pp_private *priv_p = pp_p->pp_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct pp_page_node *new_np;
	void *a_handle;

	if (np->pp_oob) {
		allocator_p->a_op->a_free(allocator_p, np->pp_page_handle);
		return;
	}

	a_handle = allocator_p->a_op->a_calloc(allocator_p, priv_p->p_set_id, 1,
		sizeof(*new_np));
	new_np = allocator_p->a_op->a_get_data(allocator_p, a_handle);
	new_np->pp_handle = a_handle;

	new_np->pp_page = np->pp_page;
	new_np->pp_page_handle = np->pp_page_handle;

	/* clear the page reference so that pp_free_node() knows this is non-data */
	np->pp_page = NULL;
	np->pp_page_handle = NULL;

	new_np->pp_where = PP_WHERE_FREE;
	new_np->pp_oob = 0;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add(&new_np->pp_list, &priv_p->p_free_head);
	priv_p->p_nfree++;
	pthread_cond_signal(&priv_p->p_free_cond);
	pthread_mutex_unlock(&priv_p->p_lck);
}

static uint32_t
pp_get_pageszshift(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	return priv_p->p_pageszshift;
}

static uint32_t
pp_get_pagesz(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	return priv_p->p_pagesz;
}

static uint32_t 
pp_get_poolsz(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	return priv_p->p_poolsz;
}

static uint32_t 
pp_get_poollen(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	return priv_p->p_poollen;
}

static uint32_t 
pp_get_nfree(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	return priv_p->p_nfree;
}

static uint32_t
pp_get_avail(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	uint32_t avail;
	pthread_mutex_lock(&priv_p->p_lck);
	avail = priv_p->p_poolsz - priv_p->p_poollen + priv_p->p_nfree;
	pthread_mutex_unlock(&priv_p->p_lck);
	return avail;
}

static char *
pp_get_name(struct pp_interface *pp_p)
{
	struct pp_private *priv_p = pp_p->pp_private;
	return priv_p->p_name;
}

static void
pp_destroy(struct pp_interface *pp_p)
{
	char *log_header = "pp_destroy";
	struct pp_private *priv_p = pp_p->pp_private;
	struct pp_page_node *np;
	struct allocator_interface *allocator_p = priv_p->p_allocator;

	nid_log_warning("%s: start ...", log_header);
	assert(priv_p->p_poollen == priv_p->p_nfree);

	while (!list_empty(&priv_p->p_free_head)) {
		np = list_first_entry(&priv_p->p_free_head, struct pp_page_node, pp_list);
		list_del(&np->pp_list);
		priv_p->p_nfree--;
		allocator_p->a_op->a_free(allocator_p, np->pp_page_handle);
		allocator_p->a_op->a_free(allocator_p, np->pp_handle);
	}
	assert(priv_p->p_nfree == 0);

	pthread_cond_destroy(&priv_p->p_free_cond);
	pthread_mutex_destroy(&priv_p->p_lck);
	free((void *)priv_p);
	pp_p->pp_private = NULL;
	nid_log_warning("%s: end ...", log_header);
}

struct pp_operations pp_op = {
	.pp_get_node = pp_get_node,
	.pp_get_node_timed = pp_get_node_timed,
	.pp_get_node_nowait = pp_get_node_nowait,
	.pp_get_node_forcibly = pp_get_node_forcibly,
	.pp_free_node = pp_free_node,
	.pp_get_pageszshift = pp_get_pageszshift,
	.pp_get_pagesz = pp_get_pagesz,
	.pp_get_poolsz = pp_get_poolsz,
	.pp_get_poollen = pp_get_poollen,
	.pp_get_nfree = pp_get_nfree,
	.pp_get_avail = pp_get_avail,
	.pp_get_name = pp_get_name,
	.pp_get_node_nondata = pp_get_node_nondata,
	.pp_free_data = pp_free_data,
	.pp_destroy = pp_destroy,
};

int
pp_initialization(struct pp_interface *pp_p, struct pp_setup *setup)
{
	struct pp_private *priv_p;
	uint32_t pool_len;
	char *log_header ="pp_initialization";

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	priv_p = x_calloc(1, sizeof(*priv_p));
	pp_p->pp_private = priv_p;
	pp_p->pp_op = &pp_op;

	strcpy(priv_p->p_name, setup->pp_name);
	INIT_LIST_HEAD(&priv_p->p_free_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	pthread_cond_init(&priv_p->p_free_cond, NULL);
	priv_p->p_allocator = setup->allocator;	
	priv_p->p_set_id = setup->set_id;
	priv_p->p_extend = 4;
	priv_p->p_pageszshift = DEFAULT_PAGESZSHIFT;
	priv_p->p_pagesz = (1 << priv_p->p_pageszshift);
	if (setup->page_size) {
		uint32_t pagesz = setup->page_size;
		priv_p->p_pagesz = (setup->page_size << 20);
		priv_p->p_pageszshift = 20;
		while (pagesz != 1) {
			pagesz = (pagesz >> 1);
			priv_p->p_pageszshift++;
		}
	}
	nid_log_info("%s: p_pageszshift:%u", log_header, priv_p->p_pageszshift);
	priv_p->p_poolsz = DEFAULT_POOLSIZE;
	if (setup->pool_size)
		priv_p->p_poolsz = setup->pool_size;

	pool_len = priv_p->p_extend;
	if (pool_len == 0)
		pool_len = priv_p->p_poolsz;
	while (priv_p->p_poollen < pool_len)
		_pp_extend(pp_p);

	nid_log_info("%s: done with (poollen:%u, poolsz:%u, pagesz:%uM)",
		log_header, priv_p->p_poollen, priv_p->p_poolsz, (priv_p->p_pagesz >> 20));
	return 0;
}
