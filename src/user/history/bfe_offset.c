/*
 * bfe.c
 * 	Implementation of  BIO Flush Engine Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/uio.h>

#include "nid_log.h"
#include "list.h"
#include "lck_if.h"
#include "pp_if.h"
#include "ddn_if.h"
#include "bre_if.h"
#include "bfe_if.h"

#define BFE_MAX_IOV		1024
//#define BFE_OFFSET_FLUSH	1

struct bfe_private {
	struct lck_interface	p_lck;
	struct lck_node		p_lnode;
#ifdef BFE_OFFSET_FLUSH
	struct list_head	p_flush_head;
#endif
	struct list_head	p_page_head;
	struct bre_interface	*p_bre;
	struct pp_interface	*p_pp;
	uint8_t			p_stop;
	uint8_t			p_flush_stop;
};

static void
bfe_flush_page(struct bfe_interface *bfe_p, struct pp_page_node *np)
{
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct bre_interface *bre_p = priv_p->p_bre;
	struct lck_interface *lck_p = &priv_p->p_lck;

	if (np->pp_flushed == np->pp_occupied) {
		bre_p->br_op->br_release_page(bre_p, np);
	} else {
		lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
	 	list_add_tail(&np->pp_list, &priv_p->p_page_head);
		lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);
	}
}

static int 
bfe_right_extend_node(struct bfe_interface *bfe_p, struct data_description_node *np, uint32_t extend)
{
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	int rc = 1;

	lck_p->l_op->l_wlock(lck_p, &np->d_page_node->pp_lnode);
	if (!np->d_flushing) {
		np->d_len += extend;
		rc = 0;
	}
	lck_p->l_op->l_wunlock(lck_p, &np->d_page_node->pp_lnode);
	return rc;
}

/*
 * Algorithm:
 * 	Add a new node 
 * Notice:
 * 	The caller should guarantee that this node would not be "freed" before 
 * 	the end of this call	
 *	The caller should make sure that the page is not in p_page_head yet
 */
static void
bfe_add_node(struct bfe_interface *bfe_p, struct data_description_node *np, struct data_description_node *pre_np)
{
	char *log_header = "bfe_add_node";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
#ifdef BFE_OFFSET_FLUSH
	struct list_head *pre_list = NULL;
#endif

	nid_log_debug("%s: start (np:%p, pre_np:%p) ...", log_header, np, pre_np);
#ifdef BFE_OFFSET_FLUSH
	if (pre_np)
		pre_list = &pre_np->d_rlist;
#endif

	/* add the np to pp_flush_head of it's page */
	lck_p->l_op->l_wlock(lck_p, &np->d_page_node->pp_lnode);
	list_add_tail(&np->d_flist, &np->d_page_node->pp_flush_head);

#ifdef BFE_OFFSET_FLUSH
	lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
	if (pre_list && !pre_np->d_flushing)
		__list_add(&np->d_rlist, pre_list, pre_list->next);
	else
		list_add_tail(&np->d_rlist, &priv_p->p_flush_head);
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);
#endif

	lck_p->l_op->l_wunlock(lck_p, &np->d_page_node->pp_lnode);

}

/*
 * Algorithm:
 * 	Cut the left part of an existing node which has not been flushed yet
 * Notice:
 * 	The caller should guarantee that this node would not be "freed" before 
 * 	the end of this call	
 */
static void
bfe_left_cut_node(struct bfe_interface *bfe_p, struct data_description_node *np, uint32_t cut_len)
{
	char *log_header = "bfe_left_cut_node";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct pp_page_node *np_pnp = np->d_page_node;

	nid_log_debug("%s: start (np:%p) ...", log_header, np);
	lck_p->l_op->l_wlock(lck_p, &np->d_page_node->pp_lnode);
	np->d_pos += cut_len;
	np->d_offset += cut_len;
	np->d_len -= cut_len;
	if (!np->d_flushed)
		np_pnp->pp_flushed += cut_len;
	lck_p->l_op->l_wunlock(lck_p, &np->d_page_node->pp_lnode);
}

/*
 * Algorithm:
 *
 * Notice:
 * 	The caller should guarantee that this node would not be released before 
 * 	the end of this call	
 */
static void 
bfe_right_cut_node(struct bfe_interface *bfe_p, struct data_description_node *np,
	uint32_t cut_len)
{
	char *log_header = "bfe_right_cut_node";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct pp_page_node *pnp = np->d_page_node;

	nid_log_debug("%s: start (np:%p) ...", log_header, np);
	lck_p->l_op->l_wlock(lck_p, &np->d_page_node->pp_lnode);
	np->d_len -= cut_len;
	if (!np->d_flushed)
		pnp->pp_flushed += cut_len;
	lck_p->l_op->l_wunlock(lck_p, &np->d_page_node->pp_lnode);
}

/*
 * Algorithm:
 *
 * Notice:
 * 	The caller should guarantee that this node would not be released before
 * 	the end of this call
 */
static int
bfe_right_cut_node_adjust(struct bfe_interface *bfe_p, struct data_description_node *np,
	uint32_t cut_len, struct data_description_node *a_np)
{
	char *log_header = "bfe_right_cut_node_adjust";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct pp_page_node *pnp = np->d_page_node;
	uint32_t adjust_len = a_np->d_len;
	int rc = 0;

	nid_log_debug("%s: start (np:%p, a_np:%p) ...", log_header, np, a_np);
	lck_p->l_op->l_wlock(lck_p, &np->d_page_node->pp_lnode);
	np->d_len -= cut_len;
	if (!np->d_flushing) {
		a_np->d_flushing = 0;
		a_np->d_flushed = 0;
		pnp->pp_flushed += cut_len - adjust_len;
		__list_add(&a_np->d_flist, &np->d_flist, np->d_flist.next);

#ifdef BFE_OFFSET_FLUSH
		/*
		 * this is a new node, don't forget to insert it in p_flush_head
		 */
		lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
		__list_add(&a_np->d_rlist, &np->d_rlist, np->d_rlist.next);
		lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);
#endif

	} else {
		a_np->d_flushing = 1;
		a_np->d_flushed = 1;
		if (!np->d_flushed)
			pnp->pp_flushed += cut_len;	// since d_len already got cut
		list_add_tail(&a_np->d_rlist, &np->d_page_node->pp_release_head);
	}
	lck_p->l_op->l_wunlock(lck_p, &np->d_page_node->pp_lnode);
	return rc;
}

/*
 * Algorithm:
 * 	If node still in the flush list, remove it from the flush list
 * Notice:
 * 	The caller should guarantee that this node would not be "freed" before 
 * 	the end of this call	
 */ 	
static void
bfe_del_node(struct bfe_interface *bfe_p, struct data_description_node *np)
{
	char *log_header = "bfe_del_node";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct pp_page_node *pnp = np->d_page_node;

	nid_log_debug("%s: start (np:%p) ...", log_header, np);
	lck_p->l_op->l_wlock(lck_p, &pnp->pp_lnode);
	if (!np->d_flushing) {
		list_del(&np->d_flist);
		pnp->pp_flushed += np->d_len;
		np->d_flushing = 1;
		np->d_flushed = 1;
		lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
#ifdef BFE_OFFSET_FLUSH
		list_del(&np->d_rlist);	// removed from p_flush_head
#endif
		lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);
	}
	lck_p->l_op->l_wunlock(lck_p, &np->d_page_node->pp_lnode);

	nid_log_debug("%s: done (np:%p) ...", log_header, np);
}

static void
bfe_close(struct bfe_interface *bfe_p)
{
	char *log_header = "bfe_close";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	nid_log_notice("%s: start ...", log_header);
	priv_p->p_stop = 1;
	while (!priv_p->p_flush_stop)
		sleep(1);
	free(priv_p);
	bfe_p->bf_private = NULL;
}

struct bfe_operations bfe_op = {
	.bf_flush_page = bfe_flush_page,
	.bf_right_extend_node = bfe_right_extend_node,
	.bf_add_node = bfe_add_node,
	.bf_left_cut_node = bfe_left_cut_node,
	.bf_right_cut_node = bfe_right_cut_node,
	.bf_right_cut_node_adjust = bfe_right_cut_node_adjust,
	.bf_del_node = bfe_del_node,
	.bf_close = bfe_close,
};

static int 
_bfe_flush_list(struct list_head *to_flush_head)
{
	struct data_description_node *first_np = NULL, *pre_np = NULL, *np;
	struct iovec iov[BFE_MAX_IOV];
	int vec_counter = 0, total_counter = 0;

	if (list_empty(to_flush_head))
		return 0;

	first_np = list_first_entry(to_flush_head, struct data_description_node, d_flist);
	iov[vec_counter].iov_base = (char *)(first_np->d_pos);
	iov[vec_counter++].iov_len = first_np->d_len;
	pre_np = first_np;
	list_for_each_entry(np, struct data_description_node, to_flush_head, d_flist) {
		if (np == first_np)
			continue;

		if ((vec_counter < BFE_MAX_IOV) &&
		    (pre_np->d_fhandle == pre_np->d_fhandle) &&
		    (np->d_offset == pre_np->d_offset + pre_np->d_len - 1)) {
			iov[vec_counter].iov_base = (char *)(np->d_pos);
			iov[vec_counter++].iov_len = np->d_len;
			pre_np = np;
			continue;
		}

		pwritev(first_np->d_fhandle, iov, vec_counter, first_np->d_offset);
		total_counter++;
		vec_counter = 0;
		first_np = np;
		iov[vec_counter].iov_base = (char *)(first_np->d_pos);
		iov[vec_counter++].iov_len = first_np->d_len;
		pre_np = first_np;
	}
	pwritev(first_np->d_fhandle, iov, vec_counter, first_np->d_offset);
	total_counter++;

	return total_counter;
}

static void *
_bfe_flush_thread(void *p)
{
	char *log_header = "_bfe_flush_thread";
	struct bfe_interface *bfe_p = (struct bfe_interface *)p;
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct pp_interface *pp_p = (struct pp_interface *)priv_p->p_pp;
	struct bre_interface *bre_p = priv_p->p_bre;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct data_description_node *np, *np2;
	struct pp_page_node *pnp;
	struct list_head to_flush_head, to_flush_head2, to_release_head;
	uint32_t pool_sz, pool_len, pool_nfree, flush_counter, writev_counter, idle_counter;

	nid_log_info("%s: start ...", log_header);
	pool_sz = pp_p->pp_op->pp_get_poolsz(pp_p);
	idle_counter = 0;
	INIT_LIST_HEAD(&to_flush_head);
	INIT_LIST_HEAD(&to_flush_head2);
	INIT_LIST_HEAD(&to_release_head);

next_try:
	if (priv_p->p_stop && list_empty(&priv_p->p_page_head))
		goto out;

	if (list_empty(&priv_p->p_page_head)) {
		sleep(1);
		goto next_try;
	}

	pool_nfree = pp_p->pp_op->pp_get_nfree(pp_p);
	pool_len = pp_p->pp_op->pp_get_poollen(pp_p);
	if ((pool_nfree + pool_sz - pool_len) > (pool_sz/2) && idle_counter < 60) {
		sleep(1);
		idle_counter++;
		goto next_try;
	}
	idle_counter = 0;

#ifdef BFE_OFFSET_FLUSH
	/*
	 * flushing p_flush_head 
	 */
	flush_counter = 0;
	lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
	list_for_each_entry_safe(np, np2, struct data_description_node, &priv_p->p_flush_head, d_rlist) {
		list_del(&np->d_rlist);
		list_add_tail(&np->d_rlist, &to_flush_head2);
		if (++flush_counter >= 1024)
			break;
	}
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);

	list_for_each_entry_safe(np, np2, struct data_description_node, &to_flush_head2, d_rlist) {
		list_del(&np->d_rlist);
		pnp = np->d_page_node;
		lck_p->l_op->l_wlock(lck_p, &pnp->pp_lnode);
		assert(!np->d_flushing && !np->d_flushed);
		np->d_flushing = 1;
		list_del(&np->d_flist);
		lck_p->l_op->l_wunlock(lck_p, &pnp->pp_lnode);
		list_add_tail(&np->d_flist, &to_flush_head);
	}

	writev_counter = _bfe_flush_list(&to_flush_head);
	nid_log_notice("%s: p_flush_head flushing done (flush:%d/writev:%d)", log_header, flush_counter, writev_counter);

	list_for_each_entry_safe(np, np2, struct data_description_node, &to_flush_head, d_flist) {
		list_del(&np->d_flist);
		pnp = np->d_page_node;
		lck_p->l_op->l_wlock(lck_p, &pnp->pp_lnode);
		pnp->pp_flushed += np->d_len;
		list_add_tail(&np->d_rlist, &pnp->pp_release_head);
		np->d_flushed = 1;
		lck_p->l_op->l_wunlock(lck_p, &pnp->pp_lnode);
	}
#endif

	/*
	 * forcely flushing one page
	 */
	lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
	pnp = list_first_entry(&priv_p->p_page_head, struct pp_page_node, pp_list);
	list_del(&pnp->pp_list);
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);

	flush_counter = 0;
	lck_p->l_op->l_wlock(lck_p, &pnp->pp_lnode);
	list_for_each_entry(np, struct data_description_node, &pnp->pp_flush_head, d_flist) {
		assert(!np->d_flushing && !np->d_flushed);
		np->d_flushing = 1;
#ifdef BFE_OFFSET_FLUSH
		list_del(&np->d_rlist);	// removed from p_flush_head
#endif
		flush_counter++;
	}
	list_splice_tail_init(&pnp->pp_flush_head, &to_flush_head);
	lck_p->l_op->l_wunlock(lck_p, &pnp->pp_lnode);

	nid_log_notice("%s: page flushing %d packets", log_header, flush_counter);
#if 0
	list_for_each_entry(np, struct data_description_node, &to_flush_head, d_flist) {
		flush_counter--;
		pwrite(np->d_fhandle, np->d_pos, np->d_len, np->d_offset);
	}
	assert(flush_counter == 0);
#endif
	writev_counter = _bfe_flush_list(&to_flush_head);
	nid_log_notice("%s: flushing done (flush:%d/writev:%d)", log_header, flush_counter, writev_counter);

	lck_p->l_op->l_wlock(lck_p, &pnp->pp_lnode);
	list_for_each_entry_safe(np, np2, struct data_description_node, &to_flush_head, d_flist) {
		pnp->pp_flushed += np->d_len;
		list_del(&np->d_flist);
		list_add_tail(&np->d_rlist, &pnp->pp_release_head);
		np->d_flushed = 1;
	}
	lck_p->l_op->l_wunlock(lck_p, &pnp->pp_lnode);
	assert(pnp->pp_flushed == pnp->pp_occupied);

	/*
	 * the page pnp is ready to release
	 */
	bre_p->br_op->br_release_page(bre_p, pnp);
	goto next_try;
out:
	priv_p->p_flush_stop = 1;
	return NULL;
}

int
bfe_initialization(struct bfe_interface *bfe_p, struct bfe_setup *setup)
{
	struct bfe_private *priv_p;
	struct lck_setup lck_setup;
	pthread_attr_t attr;
	pthread_t thread_id;

	nid_log_info("bfe_initialization start ...");
	assert(setup);
	bfe_p->bf_op = &bfe_op;
	priv_p = calloc(1, sizeof(*priv_p));
	bfe_p->bf_private = priv_p;
	lck_initialization(&priv_p->p_lck, &lck_setup);
	lck_node_init(&priv_p->p_lnode);
#ifdef BFE_OFFSET_FLUSH
	INIT_LIST_HEAD(&priv_p->p_flush_head);
#endif
	INIT_LIST_HEAD(&priv_p->p_page_head);
	priv_p->p_pp = setup->pp;
	priv_p->p_bre = setup->bre;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _bfe_flush_thread, (void *)bfe_p);
	return 0;
}
