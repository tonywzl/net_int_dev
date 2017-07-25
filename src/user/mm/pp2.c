/*
 * pp2.c
 *  Implementation of Page Pool2 Module
 */


#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>

#include "list.h"
#include "lck_if.h"
#include "nid_log.h"
#include "allocator_if.h"
#include "pp2_if.h"

#define DEFAULT_PAGESZSHIFT	23	// 8M
#define	ALIGNMENT	4	//4bit
#define get_align_offset(alignment, offset, inttype) \
	(alignment == 0 ? offset : (offset + (inttype)alignment - 1) & (~(((inttype)alignment)- 1)))
#define get_align_length(alignment, len, newoffset, inttype) \
	(alignment == 0 ? len : (( (newoffset + (inttype)len + (inttype)alignment - 1) & (~(((inttype)alignment)- 1)) )) - newoffset)



struct pp2_page_node {
	void			*pp2_handle;
	struct list_head	pp2_list;
	struct lck_node		pp2_lnode;
	uint32_t		pp2_occupied;	// need page lock for access
	uint32_t		pp2_nuser;	// need page lock for access
	uint8_t			pp2_oob;
	char			pp2_padding[3];
	char			pp2_page_buf[];
};

struct pp2_page_segment {
	struct pp2_page_node	*pp2_seg_owner;
	uint32_t		pp2_seg_size;
	char			pp2_seg_buf[];
};

struct pp2_private {
	struct allocator_interface	*p_allocator;
	struct lck_interface		p_segment_lck;
	int				p_set_id;
	struct list_head		p_free_head;
	struct pp2_page_node		*p_curpage; 	// current non-full page for further use
	pthread_mutex_t			p_lck;
	pthread_cond_t			p_free_cond;
	uint32_t			p_poolsz;	// pool size: maximum number of page node. Set to 0 if no limit.
	uint32_t			p_poollen;	// pool len: current number of pages
	uint32_t			p_extend;
	uint32_t			p_nfree;	// number of nodes in the free list
	uint32_t			p_pagesz;	// page size: must be power of 2
	uint8_t				p_pageszshift;
	int 				p_get_zero;	// flag for whether to zero the buffer before get
	int				p_put_zero;	// flag for whether to zero the buffer after put
	uint64_t			p_getcounter;
	uint64_t			p_putcounter;
	int				p_alignment;
};

static void
__pp2_extend(struct pp2_interface *pp2_p)
{
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct pp2_page_node *np = NULL;
	int i, to_extend;
	void *a_handle;

	nid_log_debug("_pp2_extend: start (len:%"PRIu32", size:%"PRIu32")...",
		priv_p->p_poollen, priv_p->p_poolsz);
	if (priv_p->p_poolsz) {
		if (priv_p->p_poollen >= priv_p->p_poolsz)
			return;
		if (priv_p->p_poollen + priv_p->p_extend > priv_p->p_poolsz)
			to_extend = priv_p->p_poolsz - priv_p->p_poollen;
		else
			to_extend = priv_p->p_extend;
	} else {
		to_extend = priv_p->p_extend;
	}

	for (i = 0; i < to_extend; i++) {
		a_handle = allocator_p->a_op->a_malloc(allocator_p, priv_p->p_set_id,
			sizeof(*np) + (size_t)priv_p->p_pagesz);
		np = (struct pp2_page_node *)allocator_p->a_op->a_get_data(allocator_p, a_handle);
		np->pp2_handle = a_handle;
		lck_node_init(&np->pp2_lnode);
		np->pp2_occupied = 0;
		np->pp2_nuser = 0;
		np->pp2_oob = 0;
//		np->pp2_where = PP2_WHERE_FREE;

		pthread_mutex_lock(&priv_p->p_lck);
		if (priv_p->p_poolsz == 0 || priv_p->p_poollen < priv_p->p_poolsz) {
			list_add(&np->pp2_list, &priv_p->p_free_head);
			priv_p->p_poollen++;
			priv_p->p_nfree++;
		} else {
			allocator_p->a_op->a_free(allocator_p, np->pp2_handle);
			np = NULL;
		}
		pthread_mutex_unlock(&priv_p->p_lck);
		if (!np)
			break;
	}
}

static void
__pp2_free(struct pp2_interface *pp2_p, struct pp2_page_node *np)
{
	char *log_header = "pp2_free_node";
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct list_head *free_lp = &priv_p->p_free_head;

	pthread_mutex_lock(&priv_p->p_lck);
	if (np->pp2_oob) {
		allocator_p->a_op->a_free(allocator_p, np->pp2_handle);
		pthread_mutex_unlock(&priv_p->p_lck);
		return;
	}

	lck_node_init(&np->pp2_lnode);
	np->pp2_occupied = 0;
	np->pp2_nuser = 0;
//	np->pp2_where = PP2_WHERE_FREE;
	//list_add_tail(&np->pp2_list, free_lp);
	list_add(&np->pp2_list, free_lp);
	priv_p->p_nfree++;

	pthread_cond_signal(&priv_p->p_free_cond);
	pthread_mutex_unlock(&priv_p->p_lck);
	nid_log_info("%s: np:%p, p_poollen:%"PRIu32", p_nfree:%"PRIu32,
			log_header, np, priv_p->p_poollen, priv_p->p_nfree);
}

/*
 * Algorithm:
 * 	- We always assume that the required size won't be greater than a page size.
 *	- Need global lock to access current page.
 *	- Need page lock to access page specific fields.
 *	- If current page doesn't have enough space for this request,
 *	  switch it to NULL and set to_free so that it is allowed to be put back to free list or directly freed (if out of bandwidth)
 *	  And it doesn't matter that some remaining space has not been utilized for this round.
 */
static struct pp2_page_segment *
__pp2_get_from_curpage(struct pp2_interface *pp2_p, uint32_t size)
{
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct lck_interface *seg_lck_p = &priv_p->p_segment_lck;
	struct pp2_page_node *np;
	struct pp2_page_segment *sp = NULL;
	uint32_t size_gap, size_orign, size_align = 0, size_required = 0 ;
	uint64_t offset_align, offset_orign, offset_gap;
	int do_free = 0;

	size_orign = (uint32_t)sizeof(*sp) + size;
	pthread_mutex_lock(&priv_p->p_lck);
	np = priv_p->p_curpage;
	if (np) {
		seg_lck_p->l_op->l_wlock(seg_lck_p, &np->pp2_lnode);
		offset_orign = (uint64_t)(&np->pp2_page_buf[0] + np->pp2_occupied);
		offset_align = get_align_offset(priv_p->p_alignment, offset_orign, uint64_t);
		offset_gap = offset_align - offset_orign;
		size_align =(uint32_t)(get_align_length(priv_p->p_alignment, size_orign, offset_align, uint64_t));
		size_gap = size_align - size_orign;
		size_required = size_align + offset_gap;
		assert( !(offset_align % (priv_p->p_alignment)) && !(size_align % (priv_p->p_alignment)) );
		assert(size_required <= priv_p->p_pagesz);
		if (size_required <= (priv_p->p_pagesz - np->pp2_occupied)) {
			sp = (struct pp2_page_segment *)(&np->pp2_page_buf[0] + np->pp2_occupied + offset_gap);
			sp->pp2_seg_owner = np;
			sp->pp2_seg_size = size + size_gap;
			np->pp2_occupied += size_required;
			np->pp2_nuser++;
	                priv_p->p_getcounter++;
			nid_log_debug("pp2_get_from_curpage: np:%p,sp:%p, offset_orign :%lx, offset_align :%lx, offset_gap: %lu, size_orign:%u, size_align: %u, size_gap: %u, size_required: %u, np_occupied:%u",
					np, sp, offset_orign, offset_align,offset_gap,  size_orign, size_align,size_gap, size_required, np->pp2_occupied);
		} else {
			np->pp2_occupied = priv_p->p_pagesz; // set to page size to free
			priv_p->p_curpage = NULL;
			if (np->pp2_nuser == 0)
				do_free = 1;
		}
		seg_lck_p->l_op->l_wunlock(seg_lck_p, &np->pp2_lnode);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (do_free)
		__pp2_free(pp2_p, np);

	if (sp && priv_p->p_get_zero) {
		memset((void *)&sp->pp2_seg_buf[0], 0, (size_t)size);
	}
	return sp;
}

static struct pp2_page_segment *
__pp2_get_from_newpage(struct pp2_interface *pp2_p, struct pp2_page_node *np, uint32_t size)
{
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct lck_interface *seg_lck_p = &priv_p->p_segment_lck;
	struct pp2_page_segment *sp;
	uint32_t size_gap, size_orign, size_align = 0, size_required = 0 ;
	uint64_t offset_align, offset_orign, offset_gap;

	size_orign = (uint32_t)sizeof(*sp) + size;
	seg_lck_p->l_op->l_wlock(seg_lck_p, &np->pp2_lnode);
	offset_orign = (uint64_t)(&np->pp2_page_buf[0] + np->pp2_occupied);
	offset_align = get_align_offset(priv_p->p_alignment, offset_orign, uint64_t);
	offset_gap = offset_align - offset_orign;
	size_align =(uint32_t)(get_align_length(priv_p->p_alignment, size_orign, offset_align, uint64_t));
	size_gap = size_align - size_orign;
	assert( !(offset_align % (priv_p->p_alignment)) && !(size_align % (priv_p->p_alignment)) );
	size_required = size_align + offset_gap;
	assert(size_required <= priv_p->p_pagesz);
//	sp = (struct pp2_page_segment *)(&np->pp2_page_buf[0] + np->pp2_occupied + offset_gap);
	sp = (struct pp2_page_segment *)offset_align;

	sp->pp2_seg_owner = np;
	sp->pp2_seg_size = size + size_gap;
	np->pp2_occupied += size_required;
	np->pp2_nuser++;
	seg_lck_p->l_op->l_wunlock(seg_lck_p, &np->pp2_lnode);

	nid_log_debug("get_from_newpage: np:%p,sp:%p, offset_orign :%lx, offset_align :%lx, offset_gap: %lu, size_orign:%u, size_align: %u, size_gap: %u, size_required: %u, np_occupied:%u",
			np, sp, offset_orign, offset_align,offset_gap,  size_orign, size_align,size_gap, size_required, np->pp2_occupied);
	if (priv_p->p_get_zero) {
		memset((void *)&sp->pp2_seg_buf[0], 0, (size_t)size);
	}
	return sp;
}

static void *
pp2_get(struct pp2_interface *pp2_p, uint32_t size)
{
#if 0
	char *log_header = "pp2_get_node";
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct list_head *free_lp = &priv_p->p_free_head;
	struct pp2_page_node *np;
	struct pp2_page_segment *sp;

	nid_log_debug("%s: start", log_header);
	sp = __pp2_get_from_curpage(pp2_p, size);
	if (!sp) {
		pthread_mutex_lock(&priv_p->p_lck);
		while (!priv_p->p_nfree && (priv_p->p_poolsz == 0 ||
				priv_p->p_poolsz > priv_p->p_poollen)) {
			pthread_mutex_unlock(&priv_p->p_lck);
			__pp2_extend(pp2_p);
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

		np = list_first_entry(free_lp, struct pp2_page_node, pp2_list);
		list_del(&np->pp2_list);
		priv_p->p_nfree--;
		priv_p->p_curpage = np;

		sp = __pp2_get_from_newpage(pp2_p, np, size);
                priv_p->p_getcounter++;
		pthread_mutex_unlock(&priv_p->p_lck);
		nid_log_info("%s: np:%p, p_poollen:%"PRIu32", p_nfree:%"PRIu32,
				log_header, np, priv_p->p_poollen, priv_p->p_nfree);
//		assert(np->pp2_where == PP2_WHERE_FREE);
	}

	nid_log_debug("%s: pp2_seg_buf :%p", log_header, &sp->pp2_seg_buf[0]);
	return (void *)&sp->pp2_seg_buf[0];
#endif
	assert(pp2_p);
	return x_calloc(1, size);
}

static void *
pp2_get_zero(struct pp2_interface *pp2_p, uint32_t size)
{
#if 0
	void * rs = pp2_p->pp2_op->pp2_get(pp2_p, size);
	struct pp2_private *priv_p = pp2_p->pp2_private;

	if (priv_p->p_get_zero == 0) {
                memset((void *)rs, 0, (size_t)size);
        }

        return rs;
#endif
	assert(pp2_p);
	return x_calloc(1, size);
}

static void *
pp2_get_nowait(struct pp2_interface *pp2_p, uint32_t size)
{
	char *log_header = "pp2_get_nowait";
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct list_head *free_lp = &priv_p->p_free_head;
	struct pp2_page_node *np = NULL;
	struct pp2_page_segment *sp;

	nid_log_debug("%s: start", log_header);
	sp = __pp2_get_from_curpage(pp2_p, size);
	if (!sp) {
		pthread_mutex_lock(&priv_p->p_lck);
		while (!priv_p->p_nfree && (priv_p->p_poolsz  == 0 ||
				priv_p->p_poolsz > priv_p->p_poollen)) {
			pthread_mutex_unlock(&priv_p->p_lck);
			__pp2_extend(pp2_p);
			pthread_mutex_lock(&priv_p->p_lck);
		}

		if (priv_p->p_nfree) {
			np = list_first_entry(free_lp, struct pp2_page_node, pp2_list);
			list_del(&np->pp2_list);
			priv_p->p_nfree--;
			priv_p->p_curpage = np;
//			assert(np->pp2_where == PP2_WHERE_FREE);

			sp = __pp2_get_from_newpage(pp2_p, np, size);
                	priv_p->p_getcounter++;

		}
		pthread_mutex_unlock(&priv_p->p_lck);
	}

	return sp ? (void *)&sp->pp2_seg_buf[0] : NULL;
}

static void *
pp2_get_forcibly(struct pp2_interface *pp2_p, uint32_t size)
{
	char *log_header = "pp2_get_node_forcibly";
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct list_head *free_lp = &priv_p->p_free_head;
	struct pp2_page_node *np;
	void *a_handle;
	struct pp2_page_segment *sp;

	nid_log_debug("%s: start", log_header);
	sp = __pp2_get_from_curpage(pp2_p, size);
	if (!sp) {
		pthread_mutex_lock(&priv_p->p_lck);
		while (!priv_p->p_nfree && (priv_p->p_poolsz == 0 ||
				priv_p->p_poolsz > priv_p->p_poollen)) {
			pthread_mutex_unlock(&priv_p->p_lck);
			__pp2_extend(pp2_p);
			pthread_mutex_lock(&priv_p->p_lck);
		}

		if (!priv_p->p_nfree) {
			pthread_mutex_unlock(&priv_p->p_lck);
			/*
			 * the pagefer pool is not extendable, we have to allocate memory dynamically
			 */
			nid_log_notice("%s: out of page, dynamically allocate a page", log_header);

			a_handle = allocator_p->a_op->a_malloc(allocator_p, priv_p->p_set_id,
				sizeof(*np) + (size_t)priv_p->p_pagesz);
			np = (struct pp2_page_node *)allocator_p->a_op->a_get_data(allocator_p, a_handle);
			np->pp2_handle = a_handle;
			lck_node_init(&np->pp2_lnode);
			np->pp2_occupied = 0;
			np->pp2_nuser = 0;
			np->pp2_oob = 1;
//			np->pp2_where = PP2_WHERE_FREE;

			sp = __pp2_get_from_newpage(pp2_p, np, size);

			pthread_mutex_lock(&priv_p->p_lck);
			priv_p->p_curpage = np;
                	priv_p->p_getcounter++;
			pthread_mutex_unlock(&priv_p->p_lck);
		} else {
			np = list_first_entry(free_lp, struct pp2_page_node, pp2_list);
			list_del(&np->pp2_list);
			priv_p->p_nfree--;
			priv_p->p_curpage = np;

			sp = __pp2_get_from_newpage(pp2_p, np, size);
                	priv_p->p_getcounter++;
			pthread_mutex_unlock(&priv_p->p_lck);
//			assert(np->pp2_where == PP2_WHERE_FREE);
			assert(np->pp2_oob == 0);
		}
		nid_log_info("%s: np:%p, p_poollen:%"PRIu32", p_nfree:%"PRIu32,
				log_header, np, priv_p->p_poollen, priv_p->p_nfree);
	}

	return (void *)&sp->pp2_seg_buf[0];
}

/*
 * Algorithm:
 * 	Just need the page lock to tell if occupied == 0. If yes, we try to hold the global lock and free by __pp2_try_free().
 * 	But during the  window between page lock dropped and global lock held, others may jump in and start to use this page again.
 * 	So __pp2_try_free() always checks to_free to decide whether to do free or just return.
 */
static void
pp2_put(struct pp2_interface *pp2_p, void *seg_buf) {
/*
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct lck_interface *seg_lck_p = &priv_p->p_segment_lck;
	assert(seg_buf);
	struct pp2_page_segment *sp = container_of(seg_buf, struct pp2_page_segment, pp2_seg_buf);
	struct pp2_page_node *np = sp->pp2_seg_owner;
	int do_free = 0;
	nid_log_debug("pp2_put: np:%p, sp:%p, sp_buf:%p", np, sp, seg_buf);

	seg_lck_p->l_op->l_wlock(seg_lck_p, &np->pp2_lnode);
	if (priv_p->p_put_zero) {
		memset((void *)&sp->pp2_seg_buf[0], 0, (size_t)sp->pp2_seg_size);
	}

	if (--(np->pp2_nuser) == 0 && np->pp2_occupied == priv_p->p_pagesz) {
		do_free = 1;
		nid_log_debug("pp2_put: do_free np %p", np);
	}
	seg_lck_p->l_op->l_wunlock(seg_lck_p, &np->pp2_lnode);

	if (do_free) {
		__pp2_free(pp2_p, np);
	}
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_putcounter++;
	pthread_mutex_unlock(&priv_p->p_lck);
*/
	assert(pp2_p);
	free(seg_buf);
}

static void
pp2_display(struct pp2_interface *pp2_p){
	struct pp2_private *priv_p = pp2_p->pp2_private;

	nid_log_warning("The number of pp2 get: %ld", priv_p->p_getcounter);
	nid_log_warning("The number of pp2 put: %ld", priv_p->p_putcounter);
}

static void
pp2_cleanup(struct pp2_interface *pp2_p){
	struct pp2_private *priv_p = pp2_p->pp2_private;
	struct pp2_page_node *np;
	struct allocator_interface *allocator_p = priv_p->p_allocator;

	if (priv_p->p_getcounter == priv_p->p_putcounter) {
		pthread_mutex_lock(&priv_p->p_lck);
		while(!list_empty(priv_p->p_free_head.next)) {
			np = list_entry(priv_p->p_free_head.next, struct pp2_page_node, pp2_list);
            		list_del(&np->pp2_list);
			allocator_p->a_op->a_free(allocator_p, np->pp2_handle);
		}
		if (priv_p->p_getcounter != 0) {
			allocator_p->a_op->a_free(allocator_p, priv_p->p_curpage->pp2_handle);
		}
		pthread_mutex_unlock(&priv_p->p_lck);
	}
	free(priv_p);
	pp2_p->pp2_private = NULL;
}

struct pp2_operations pp2_op = {
	.pp2_get = pp2_get,
	.pp2_get_zero = pp2_get_zero,
	.pp2_get_nowait = pp2_get_nowait,
	.pp2_get_forcibly = pp2_get_forcibly,
	.pp2_put = pp2_put,
	.pp2_display = pp2_display,
	.pp2_cleanup = pp2_cleanup,
};

int
pp2_initialization(struct pp2_interface *pp2_p, struct pp2_setup *setup)
{
	struct pp2_private *priv_p;
	uint32_t pool_len;
	struct lck_setup lck_setup;
	char *log_header ="pp2_initialization";

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	priv_p = x_calloc(1, sizeof(*priv_p));
	pp2_p->pp2_private = priv_p;
	pp2_p->pp2_op = &pp2_op;

	lck_initialization(&priv_p->p_segment_lck, &lck_setup);
	INIT_LIST_HEAD(&priv_p->p_free_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	pthread_cond_init(&priv_p->p_free_cond, NULL);
	priv_p->p_allocator = setup->allocator;
	priv_p->p_set_id = setup->set_id;
	priv_p->p_extend = 4;
	priv_p->p_pageszshift = DEFAULT_PAGESZSHIFT;
	priv_p->p_pagesz = (1 << priv_p->p_pageszshift);
	priv_p->p_alignment = ALIGNMENT;
	if (setup->page_size) {
		uint32_t pagesz = setup->page_size;
		priv_p->p_pagesz = (setup->page_size << 20);
		priv_p->p_pageszshift = 20;
		while (pagesz != 1) {
			pagesz = (pagesz >> 1);
			priv_p->p_pageszshift++;
		}
	}
	nid_log_info("%s: p_pageszshift:%"PRIu8, log_header, priv_p->p_pageszshift);
	priv_p->p_poolsz = setup->pool_size;

	pool_len = priv_p->p_extend;
	while (priv_p->p_poollen < pool_len)
		__pp2_extend(pp2_p);

	priv_p->p_get_zero = setup->get_zero;
	priv_p->p_put_zero = setup->put_zero;


	priv_p->p_getcounter = 0;
	priv_p->p_putcounter = 0;
	nid_log_info("%s: done with (poollen:%"PRIu32", poolsz:%"PRIu32", pagesz:%"PRIu32"M) "
			"get_zero: %d, put_zero: %d",
		log_header, priv_p->p_poollen, priv_p->p_poolsz, (priv_p->p_pagesz >> 20),
		priv_p->p_get_zero, priv_p->p_put_zero);

	return 0;
}
