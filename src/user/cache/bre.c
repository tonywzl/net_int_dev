/*
 * bre.c
 * 	Implementation of  BIO Release Engine Module
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include "list.h"
#include "nid_log.h"
#include "ddn_if.h"
#include "pp_if.h"
#include "bse_if.h"
#include "bre_if.h"
#include "wc_if.h"

struct bre_private {
	struct pp_interface	*p_pp;
	struct ddn_interface	*p_ddn;
	struct bse_interface	*p_bse;
	struct wc_interface 	*p_wc;
	void			*p_wc_chain_handle;
	pthread_mutex_t		p_lck;
	pthread_cond_t		p_cond;
	struct list_head	p_page_head;
	struct list_head	p_to_page_head;
	struct list_head	p_coalesced_page_head;
	struct list_head	p_to_coalesced_page_head;
	struct list_head	*p_to_flush;
	uint32_t		p_page_counter;
	uint8_t			p_stop;
	uint8_t			p_page_release_stop;
	uint8_t			p_dropcache;
	uint8_t			p_sequence_mode;
};

static void
bre_release_page(struct bre_interface *bre_p, void *page_p)
{
	char *log_header = "bre_release_page";
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	struct pp_page_node *np = (struct pp_page_node *)page_p;

	nid_log_debug("%s: start (np:%p)...", log_header, np);
	assert(np->pp_flushed == np->pp_occupied);
	assert(np->pp_where == PP_WHERE_FLUSH);
	pthread_mutex_lock(&priv_p->p_lck);
	if (list_empty(&priv_p->p_page_head) && list_empty(&priv_p->p_coalesced_page_head))
		pthread_cond_signal(&priv_p->p_cond);

	if (!np->pp_coalesced) {
		list_add_tail(&np->pp_rlist, &priv_p->p_page_head);
	} else {
		list_add_tail(&np->pp_rlist, &priv_p->p_coalesced_page_head);
	}
	np->pp_where = PP_WHERE_RELEASE;
	priv_p->p_page_counter++;
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bre_release_page_async(struct bre_interface *bre_p, void *page_p)
{
	char *log_header = "bre_release_page";
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	struct pp_page_node *np = (struct pp_page_node *)page_p;

	nid_log_debug("%s: start (np:%p)...", log_header, np);
	assert(np->pp_flushed == np->pp_occupied);
	assert(np->pp_where == PP_WHERE_FLUSHING_DONE);
	pthread_mutex_lock(&priv_p->p_lck);
	if (list_empty(&priv_p->p_page_head) && list_empty(&priv_p->p_coalesced_page_head))
		pthread_cond_signal(&priv_p->p_cond);

	if (!np->pp_coalesced) {
		list_add_tail(&np->pp_rlist, &priv_p->p_page_head);
	} else {
		list_add_tail(&np->pp_rlist, &priv_p->p_coalesced_page_head);
	}
	np->pp_where = PP_WHERE_RELEASE;
	priv_p->p_page_counter++;
	pthread_mutex_unlock(&priv_p->p_lck);
}

static inline void
__bre_release_page(struct bre_private *priv_p, struct pp_page_node *np) {
	struct data_description_node *dnp, *dnp2;
	struct bse_interface *bse_p = priv_p->p_bse;
	struct ddn_interface *ddn_p = priv_p->p_ddn;
	struct pp_interface *pp_p = priv_p->p_pp;
	int left;

	list_for_each_entry_safe(dnp, dnp2, struct data_description_node, &np->pp_release_head, d_flist) {
		assert(dnp->d_where == DDN_WHERE_RELEASE);
		list_del(&dnp->d_flist);		// removed from pp_release_head
		bse_p->bs_op->bs_del_node(bse_p, dnp);
		left = ddn_p->d_op->d_put_node(ddn_p, dnp);
		assert(left == 0);
	}
	assert(list_empty(&np->pp_flush_head) && list_empty(&np->pp_release_head));
	pp_p->pp_op->pp_free_node(pp_p, np);
}


static void
bre_release_page_immediately(struct bre_interface *bre_p, void *page_p)
{
	char *log_header = "bre_release_page_immediately";
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;

	struct pp_page_node *np = (struct pp_page_node *)page_p;

	nid_log_debug("%s: start (np:%p) ...", log_header, np);
	assert(np->pp_flushed == np->pp_occupied);
	assert(np->pp_where == PP_WHERE_FLUSH || np->pp_where == PP_WHERE_FLUSHING_DONE);

	pthread_mutex_lock(&priv_p->p_lck);
	if (np->pp_coalesced) {
		list_add_tail(&np->pp_rlist, &priv_p->p_coalesced_page_head);
		priv_p->p_page_counter++;
		pthread_cond_signal(&priv_p->p_cond);
	} else {
		pthread_mutex_unlock(&priv_p->p_lck);
		__bre_release_page(priv_p, np);
		pthread_mutex_lock(&priv_p->p_lck);
	}
	np->pp_where = PP_WHERE_RELEASE;
	pthread_mutex_unlock(&priv_p->p_lck);

}

/*
 * Algorithm:
 * 	Release the page immediately, instead of going through
 * 	flush/release procedures
 */
static void
bre_quick_release_page(struct bre_interface *bre_p, void *page_p)
{
	char *log_header = "bre_quick_release_page";
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct pp_page_node *np = (struct pp_page_node *)page_p;
	nid_log_debug("%s: start (np:%p) ...", log_header, np);
	assert(np->pp_flushed == np->pp_occupied);
	assert(list_empty(&np->pp_release_head));
	pp_p->pp_op->pp_free_node(pp_p, np);
}

static uint32_t 
bre_get_page_counter(struct bre_interface *bre_p)
{
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	return priv_p->p_page_counter;
}

static inline void
_bre_release_page_all(struct bre_interface * bre_p) {

	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct bse_interface *bse_p = priv_p->p_bse;
	struct ddn_interface *ddn_p = priv_p->p_ddn;
	struct pp_page_node *np, *np2;
	struct data_description_node *dnp, *dnp2;

	pthread_mutex_lock(&priv_p->p_lck);
	list_splice_init(&priv_p->p_to_page_head, &priv_p->p_page_head);
	list_splice_init(&priv_p->p_to_coalesced_page_head, &priv_p->p_coalesced_page_head);
	pthread_mutex_unlock(&priv_p->p_lck);

	if (!list_empty(&priv_p->p_coalesced_page_head)) {
		list_for_each_entry_safe(np, np2, struct pp_page_node, &priv_p->p_coalesced_page_head, pp_rlist) {
			if (!np->pp_coalesced) {
				list_del(&np->pp_rlist); // removed from p_coalesced_page_head
				list_add_tail(&np->pp_rlist, &priv_p->p_page_head);
			}
		}
	}

	list_for_each_entry_safe(np, np2, struct pp_page_node, &priv_p->p_page_head, pp_rlist) {
		assert(np->pp_where == PP_WHERE_RELEASE);
		list_del(&np->pp_rlist);
		list_for_each_entry_safe(dnp, dnp2, struct data_description_node, &np->pp_release_head, d_flist) {
			assert(dnp->d_where == DDN_WHERE_RELEASE);
			list_del(&dnp->d_flist);		// removed from pp_release_head
			bse_p->bs_op->bs_del_node(bse_p, dnp);
			ddn_p->d_op->d_put_node(ddn_p, dnp);
		}
		pp_p->pp_op->pp_free_node(pp_p, np);
	}
}

static void
bre_close(struct bre_interface *bre_p)
{
	char *log_header = "bre_close";
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;

	nid_log_notice("%s: start ...", log_header);
	priv_p->p_stop = 1;

	if (!priv_p->p_sequence_mode) {
		// wait _bre_page_release_thread, thread may be in wait cond signal state
		// try to wake it up
		while(!priv_p->p_page_release_stop) {
			pthread_cond_signal(&priv_p->p_cond);
			usleep(100*1000);
		}
	}

	// release all page
	_bre_release_page_all(bre_p);

	pthread_mutex_destroy(&priv_p->p_lck);
	pthread_cond_destroy(&priv_p->p_cond);
	free(priv_p);
	bre_p->br_private = NULL;
}

static void
bre_dropcache_start(struct bre_interface *bre_p, int do_sync)
{
	char *log_header = "bre_dropcache_start";
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;

	nid_log_warning("%s: start (sync:%d)...", log_header, do_sync);
	priv_p->p_dropcache = 1;
	if (do_sync) {
		while (!priv_p->p_stop && !list_empty(&priv_p->p_page_head) &&
				!priv_p->p_page_release_stop) {
			nid_log_warning("%s: waiting for droping ...", log_header);
			sleep(1);
		}
	}

	nid_log_warning("%s: end (sync:%d)...", log_header, do_sync);
}

static void
bre_dropcache_stop(struct bre_interface *bre_p)
{
	char *log_header = "bre_dropcache_stop";
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	nid_log_warning("%s: start ...", log_header);
       	priv_p->p_dropcache = 0;	
}

static void
bre_release_page_sequence(struct bre_interface *bre_p, void *page_p)
{
	char *log_header = "bre_release_page_sequence";
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	struct pp_page_node *np = (struct pp_page_node *)page_p;

	nid_log_warning("%s: start (np:%p)...", log_header, np);
	assert(np->pp_flushed == np->pp_occupied);
	assert(np->pp_where == PP_WHERE_FLUSHING_DONE);
	pthread_mutex_lock(&priv_p->p_lck);
	if (!np->pp_coalesced) {
		list_add_tail(&np->pp_rlist, &priv_p->p_to_page_head);
	} else {
		list_add_tail(&np->pp_rlist, &priv_p->p_to_coalesced_page_head);
	}
	np->pp_where = PP_WHERE_RELEASE;
	priv_p->p_page_counter++;
	pthread_mutex_unlock(&priv_p->p_lck);
}

/**
 * Only one thread can call this function.
 */
static void
bre_update_sequence(struct bre_interface * bre_p, uint64_t seq)
{
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	struct wc_interface *wc_p = priv_p->p_wc;

	nid_log_warning("bre_update_sequence:%lu", seq);

	_bre_release_page_all(bre_p);
	wc_p->wc_op->wc_flush_update(wc_p, priv_p->p_wc_chain_handle, seq);

}

struct bre_operations bre_op = {
	.br_release_page = bre_release_page,
	.br_release_page_async = bre_release_page_async,
	.br_quick_release_page = bre_quick_release_page,
	.br_release_page_immediately = bre_release_page_immediately,
	.br_get_page_counter = bre_get_page_counter,
	.br_close = bre_close,
	.br_dropcache_start = bre_dropcache_start,
	.br_dropcache_stop = bre_dropcache_stop,
	.br_update_sequence = bre_update_sequence,
	.br_release_page_sequence = bre_release_page_sequence,
};

static void *
_bre_page_release_thread(void *p)
{
	char *log_header = "_bre_page_release_thread";
	struct bre_interface *bre_p = (struct bre_interface *)p;
	struct bre_private *priv_p = (struct bre_private *)bre_p->br_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct bse_interface *bse_p = priv_p->p_bse;
	struct ddn_interface *ddn_p = priv_p->p_ddn;
	struct pp_page_node *np, *np2;
	struct data_description_node *dnp, *dnp2;
	uint32_t pool_sz, pool_len, pool_nfree, pool_available;

	nid_log_info("%s: start ...", log_header);
	pool_sz = pp_p->pp_op->pp_get_poolsz(pp_p);

next_try:
	// don't care whether there still have pages need to release, it will release by bre_close
	if (priv_p->p_stop)
		goto out;

	pool_nfree = pp_p->pp_op->pp_get_nfree(pp_p);
	pool_len = pp_p->pp_op->pp_get_poollen(pp_p);
	pool_available = pool_nfree + pool_sz - pool_len;
	if (!priv_p->p_dropcache && (pool_available > 64)) {
		sleep(1);
		goto next_try;
	}

	pthread_mutex_lock(&priv_p->p_lck);
	while (!priv_p->p_stop && list_empty(&priv_p->p_page_head) && list_empty(&priv_p->p_coalesced_page_head)) {
		pthread_cond_wait(&priv_p->p_cond, &priv_p->p_lck);
	}
	if (priv_p->p_stop) {
		pthread_mutex_unlock(&priv_p->p_lck);
		goto next_try;
	}
	
	if (!list_empty(&priv_p->p_coalesced_page_head)) {
		list_for_each_entry_safe(np, np2, struct pp_page_node, &priv_p->p_coalesced_page_head, pp_rlist) {
			if (!np->pp_coalesced) {
				list_del(&np->pp_rlist); // removed from p_coalesced_page_head
				list_add_tail(&np->pp_rlist, &priv_p->p_page_head);
			}
		}
	}

	if (!list_empty(&priv_p->p_page_head)) {
		np = list_first_entry(&priv_p->p_page_head, struct pp_page_node, pp_rlist);
		assert(np->pp_flushed == np->pp_occupied);
		assert(np->pp_where == PP_WHERE_RELEASE);
		list_del(&np->pp_rlist);
		priv_p->p_page_counter--;
		pthread_mutex_unlock(&priv_p->p_lck);
		list_for_each_entry_safe(dnp, dnp2, struct data_description_node, &np->pp_release_head, d_flist) {
			assert(dnp->d_where == DDN_WHERE_RELEASE);
			list_del(&dnp->d_flist);		// removed from pp_release_head
			bse_p->bs_op->bs_del_node(bse_p, dnp);
			ddn_p->d_op->d_put_node(ddn_p, dnp);
		}
		assert(list_empty(&np->pp_flush_head) && list_empty(&np->pp_release_head));
		pp_p->pp_op->pp_free_node(pp_p, np);
		nid_log_notice("%s: cleanup one node", log_header);
	} else {
		pthread_mutex_unlock(&priv_p->p_lck);
		usleep(10000);	// 10 million secs
	}

	goto next_try;;

out:
	priv_p->p_page_release_stop = 1;
	nid_log_notice("%s: exit", log_header);
	return NULL;
}

int
bre_initialization(struct bre_interface *bre_p, struct bre_setup *setup)
{
	char *log_header = "bre_initialization";
	struct bre_private *priv_p;
	pthread_attr_t attr;
	pthread_t thread_id;

	nid_log_info("%s: start ...", log_header);
	bre_p->br_op = &bre_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bre_p->br_private = priv_p;
	priv_p->p_pp = setup->pp;
	priv_p->p_ddn = setup->ddn;
	priv_p->p_bse = setup->bse;
	priv_p->p_wc = setup->wc;
	priv_p->p_wc_chain_handle = setup->wc_chain_handle;
	priv_p->p_sequence_mode = setup->sequence_mode;
	INIT_LIST_HEAD(&priv_p->p_page_head);
	INIT_LIST_HEAD(&priv_p->p_coalesced_page_head);
	INIT_LIST_HEAD(&priv_p->p_to_page_head);
	INIT_LIST_HEAD(&priv_p->p_to_coalesced_page_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	pthread_cond_init(&priv_p->p_cond, NULL);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (!setup->sequence_mode) {
		pthread_create(&thread_id, &attr, _bre_page_release_thread, (void *)bre_p);
	}

	return 0;
}
