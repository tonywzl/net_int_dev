/*
 * cse.c
 * 	Implementation of Content Search Engine Module
 */

#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>

#include "nid_log.h"
#include "lck_if.h"
#include "allocator_if.h"
#include "fpn_if.h"
#include "brn_if.h"
#include "srn_if.h"
#include "pp_if.h"
#include "brn_if.h"
#include "cdn_if.h"
#include "blksn_if.h"
#include "rc_if.h"
#include "cse_if.h"
#include "fpc_if.h"
#include "blksn_if.h"
#include "cse_crc_cbn.h"
#include "cse_mds_cbn.h"
#include "rw_if.h"


#define CSE_HT_SIZE	(1 << 20)

struct cse_ht_slot {
	struct lck_node		s_lnode;
	int			s_counter;
	struct list_head	s_head;
	uint8_t			s_updating;
};

struct cse_private {
	struct allocator_interface	*p_allocator;
	struct rc_interface		*p_rc;
	struct lck_interface		p_lck;
	struct lck_node			p_lnode;
	pthread_mutex_t			p_sync_lck;
	pthread_cond_t			p_sync_cond;
	struct list_head		p_memcache_head;
	struct cdn_interface		*p_cdn;
	struct srn_interface		*p_srn;
	struct pp_interface		*p_pp;
	struct fpn_interface		*p_fpn;
	struct fpc_interface		*p_fpc;
	struct blksn_interface		*p_blksn;
	struct cse_ht_slot		*p_hash_table;
	struct pp_page_node		*p_cur_page;
	struct cse_mds_cbn_interface	p_cmcbn;
	volatile int			p_hit_counter;
	volatile int			p_unhit_counter;
	int				p_block_shift;
	int				p_block_size;
	int				p_block_mask;
	uint32_t			p_pagesz;
	uint8_t				p_fp_size;
	uint8_t				p_stop;
	uint32_t			p_ht_size;
};

/*
 * Algorithm:
 * 	Search fp_np. If found it, copy data to the request node (rn_p)
 * 	The data could be either in memory or on-disk (ssd)
 *
 * Input:
 * 	fp_np: fp_node to search
 * 	rn_p: provides a buffer to copy the block of fp_np associated, if the block does exist
 * Return:
 * 	0: if successfully found the block
 * 	-1: if cannot find the block
 */
static int
__cse_search(struct cse_private *priv_p, struct fp_node *fp_np, struct sub_request_node *rn_p)
{
	char *log_header = "__cse_search";
	struct rc_interface *rc_p = priv_p->p_rc;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct fpc_interface *fpc_p = priv_p->p_fpc;
	struct cse_ht_slot *the_slot;
	struct content_description_node *cd_np;
	int rc = 0, got_it = 0;
	uint64_t hval;

	nid_log_debug("%s: start", log_header);
//	nid_log_error("%s: offset: 0x%lx", log_header, rn_p->sr_offset);
	assert(rn_p->sr_len == (uint32_t)priv_p->p_block_size);
	hval = fpc_p->fpc_op->fpc_hvcalculate(fpc_p, fp_np->fp) % priv_p->p_ht_size;
	the_slot = priv_p->p_hash_table + hval;

	lck_p->l_op->l_rlock(lck_p, &the_slot->s_lnode);
	list_for_each_entry(cd_np, struct content_description_node, &the_slot->s_head, cn_slist) {
		if (!fpc_p->fpc_op->fpc_cmp(fpc_p, cd_np->cn_fp, fp_np->fp)) {
			got_it = 1;
			break;
		}
	}

	if (got_it) {
		nid_log_debug("%s: got it", log_header);
		__sync_add_and_fetch(&priv_p->p_hit_counter, 1);
		if (cd_np->cn_is_ondisk) {
			rc_p->rc_op->rc_read_block(rc_p, rn_p->sr_buf, (uint64_t)cd_np->cn_data);
		} else {
			memcpy(rn_p->sr_buf, (char *)cd_np->cn_data, priv_p->p_block_size);
		}
	} else {
//		nid_log_error("%s: offset: 0x%lx  not found!", log_header, rn_p->sr_offset);
		__sync_add_and_fetch(&priv_p->p_unhit_counter, 1);
		rc = -1;
	}
	lck_p->l_op->l_runlock(lck_p, &the_slot->s_lnode);

	if (got_it) {
		rc_p->rc_op->rc_touch_block(rc_p, cd_np, fp_np->fp);
	}
	return rc;
}


static int
cse_check_fp(struct cse_interface *cse_p)
{

	char *log_header = "cse_check_fp";
	 struct cse_private *priv_p = (struct cse_private *)cse_p->cs_private;
	struct rc_interface *rc_p = priv_p->p_rc;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct fpc_interface *fpc_p = priv_p->p_fpc;
	struct cse_ht_slot *the_slot;
	struct content_description_node *cd_np;
	char	*buf = NULL;
	unsigned char *fpbuf = NULL;
	int rc = 0;
	uint64_t hval;



	nid_log_debug("%s: start", log_header);
	//assert(rn_p->sr_len == (uint32_t)priv_p->p_block_size);
	for (hval = 0; hval < priv_p->p_ht_size; hval++){

		the_slot = priv_p->p_hash_table + hval;
		if ( the_slot != NULL && !list_empty(&the_slot->s_head)){

			lck_p->l_op->l_rlock(lck_p, &the_slot->s_lnode);
			list_for_each_entry(cd_np, struct content_description_node, &the_slot->s_head, cn_slist) {
				if (cd_np->cn_is_ondisk) {
					rc_p->rc_op->rc_read_block(rc_p, buf, (uint64_t)cd_np->cn_data);
				//	rc = -1;
				} else {
					memcpy(buf, (char *)cd_np->cn_data, priv_p->p_block_size);
				}
				fpc_p->fpc_op->fpc_calculate(fpc_p, (void *)buf , priv_p->p_block_size, (unsigned char*)fpbuf);
				rc = fpc_p->fpc_op->fpc_cmp(fpc_p, fpbuf, cd_np->cn_fp);
				if (rc)
					goto out;
			}
			lck_p->l_op->l_runlock(lck_p, &the_slot->s_lnode);
		}
	}
out:
	return rc;
}

/*
 * Add all existing fp into cse rtree search list and dsrec lru list.
 *
 * arguments:
 * 1> cse_p
 * 2> cd_head_p, the list includes the nodes need to be added.
 */
static void
cse_recover_fp(struct cse_interface *cse_p, struct list_head *cd_head_p)
{
	char *log_header = "cse_recover_fp";
	struct cse_private *priv_p = (struct cse_private *)cse_p->cs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct rc_interface *rc_p = priv_p->p_rc;
	struct fpc_interface *fpc_p = priv_p->p_fpc;

	struct cse_ht_slot *the_slot;
	struct content_description_node *cd_np, *cd_np2;
	uint64_t hval = 0;

	nid_log_debug("%s: start", log_header);
	list_for_each_entry_safe(cd_np, cd_np2, struct content_description_node, cd_head_p, cn_slist) {
		list_del(&cd_np->cn_slist);
		hval = fpc_p->fpc_op->fpc_hvcalculate(fpc_p, cd_np->cn_fp) % priv_p->p_ht_size;
		the_slot = priv_p->p_hash_table + hval;
		lck_p->l_op->l_wlock(lck_p, &the_slot->s_lnode);
		list_add_tail(&cd_np->cn_slist, &the_slot->s_head);
		lck_p->l_op->l_wunlock(lck_p, &the_slot->s_lnode);
		rc_p->rc_op->rc_insert_block(rc_p, cd_np); // Add block to lru list waiting for reclaim
	}
}

/*
 * Algorithm:
 * 	If the fp exists in the hash table, do nothing.
 *
 * Notice:
 */
static void
__cse_insert(struct cse_private *priv_p, struct fp_node *fp_np, char *data)
{
	char *log_header = "__cse_insert";
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct cdn_interface *cdn_p = priv_p->p_cdn;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct fpc_interface *fpc_p = priv_p->p_fpc;
	struct rc_interface *rc_p = priv_p->p_rc;
	struct cse_ht_slot *the_slot;
	struct content_description_node *cd_np;
	struct pp_page_node *cur_page, *new_page;
	uint32_t cur_occupied;
	int do_it = 1;
	uint64_t hval;

	hval = fpc_p->fpc_op->fpc_hvcalculate(fpc_p, fp_np->fp) % priv_p->p_ht_size;
	the_slot = priv_p->p_hash_table + hval;

	lck_p->l_op->l_wlock(lck_p, &the_slot->s_lnode);
	while (the_slot->s_updating) {
		lck_p->l_op->l_wunlock(lck_p, &the_slot->s_lnode);
		usleep(10000);
		lck_p->l_op->l_wlock(lck_p, &the_slot->s_lnode);
	}
	the_slot->s_updating = 1;
	list_for_each_entry(cd_np, struct content_description_node, &the_slot->s_head, cn_slist) {
		if (!fpc_p->fpc_op->fpc_cmp(fpc_p, cd_np->cn_fp, fp_np->fp)) {
			/* the content exists, nothing need to do */
			do_it = 0;
			rc_p->rc_op->rc_touch_block(rc_p, cd_np, fp_np->fp);
			break;
		}
	}

	if (do_it) {
		nid_log_debug("cse_insert a node to read cache.");

		pthread_mutex_lock(&priv_p->p_sync_lck);
		while (!priv_p->p_cur_page) {
			pthread_mutex_unlock(&priv_p->p_sync_lck);
			lck_p->l_op->l_wunlock(lck_p, &the_slot->s_lnode);
			usleep(10000);
			lck_p->l_op->l_wlock(lck_p, &the_slot->s_lnode);
			pthread_mutex_lock(&priv_p->p_sync_lck);
		}
		cur_page = priv_p->p_cur_page;
		cur_occupied = cur_page->pp_occupied;

		cd_np = cdn_p->cn_op->cn_get_node(cdn_p);
		cd_np->cn_is_lru = 0;
		cd_np->cn_is_ondisk = 0;
		memcpy(cd_np->cn_fp, fp_np->fp, priv_p->p_fp_size);
		cd_np->cn_data = (unsigned long)(cur_page->pp_page + cur_occupied);
		memcpy(cur_page->pp_page + cur_occupied, data, priv_p->p_block_size);

		list_add_tail(&cd_np->cn_plist, &cur_page->pp_flush_head);
		list_add_tail(&cd_np->cn_slist, &the_slot->s_head);

		cur_page->pp_occupied += priv_p->p_block_size;	// Add current occupied before use
		if (cur_page->pp_occupied >= priv_p->p_pagesz) {
			assert(cur_page->pp_occupied == priv_p->p_pagesz);
			nid_log_debug("%s: pp_get_node ...", log_header);
			list_add_tail(&cur_page->pp_list, &priv_p->p_memcache_head);
			pthread_cond_signal(&priv_p->p_sync_cond);
			priv_p->p_cur_page = NULL;
			pthread_mutex_unlock(&priv_p->p_sync_lck);
			lck_p->l_op->l_wunlock(lck_p, &the_slot->s_lnode);

			new_page = pp_p->pp_op->pp_get_node(pp_p);
			lck_p->l_op->l_wlock(lck_p, &the_slot->s_lnode);
			pthread_mutex_lock(&priv_p->p_sync_lck);
			priv_p->p_cur_page = new_page;
			INIT_LIST_HEAD(&priv_p->p_cur_page->pp_flush_head);
			priv_p->p_cur_page->pp_occupied = 0;
			priv_p->p_cur_page->pp_flushed = 0;
		}

		pthread_mutex_unlock(&priv_p->p_sync_lck);
	}
	the_slot->s_updating = 0;
	lck_p->l_op->l_wunlock(lck_p, &the_slot->s_lnode);

}
/*
 * Input:
 * 	fp_head: list of fp_node to search
 * 	sreq_head: list of sub_request_node, in order with fp_head. Each node in the list must
 * 	have len of priv_p->p_block_size
 */
static void
cse_search_list(struct cse_interface *cse_p, struct list_head *fp_head,
	struct list_head *sreq_head, struct list_head *not_found_head, struct list_head *not_found_fp_head, int *not_found_num)
{
	struct cse_private *priv_p = (struct cse_private *)cse_p->cs_private;
	struct sub_request_node *rn_p, *rn_p2;
	struct fp_node *fp_np, *fp_np1;
	int rc;

	if (list_empty(fp_head))
		return;

	rn_p = list_first_entry(sreq_head, struct sub_request_node, sr_list);
	(*not_found_num) = 0;
	list_for_each_entry_safe(fp_np, fp_np1, struct fp_node, fp_head, fp_list) {
		rc = __cse_search(priv_p, fp_np, rn_p);
		rn_p2 = list_entry(rn_p->sr_list.next, struct sub_request_node, sr_list);
		if (rc) {
			list_del(&rn_p->sr_list);	//removed from sreq_head
			list_add_tail(&rn_p->sr_list, not_found_head);
			list_del(&fp_np->fp_list);
			list_add_tail(&fp_np->fp_list, not_found_fp_head);
			(*not_found_num)++;
		}
		rn_p = rn_p2;
	}
}


static inline void
_cse_fill_real_sreq_and_free(struct list_head *sreq_head, struct list_head *fp_head,
		struct srn_interface *srn_p, struct fpn_interface *fpn_p)
{
	struct sub_request_node *sreq_p, *sreq1_p;
	struct fp_node *fp;

	list_for_each_entry_safe(sreq_p, sreq1_p, struct sub_request_node, sreq_head, sr_list) {
		list_del(&sreq_p->sr_list);
		if (sreq_p->sr_real) {
			memcpy(sreq_p->sr_real->sr_buf,
				sreq_p->sr_buf + (sreq_p->sr_real->sr_offset - sreq_p->sr_offset),
				sreq_p->sr_real->sr_len);
			srn_p->sr_op->sr_put_node(srn_p, sreq_p->sr_real);
		}
		srn_p->sr_op->sr_put_node(srn_p, sreq_p);
	}

	while (!list_empty(fp_head)) {
		fp = list_first_entry(fp_head, struct fp_node, fp_list);
		list_del(&fp->fp_list);
		fpn_p->fp_op->fp_put_node(fpn_p, fp);
	}
}

static void
nse_search_fetch_callback(int err, void* arg)
{
	struct cse_mds_cb_node *cmc_node = arg;
	struct cse_private *priv_p = (struct cse_private *)cmc_node->cm_cse_p->cs_private;
	_cse_fill_real_sreq_and_free(&cmc_node->cm_not_found_head, &cmc_node->cm_not_found_fp_head,
			priv_p->p_srn, priv_p->p_fpn);
	cmc_node->cm_super_callback(err, cmc_node->cm_super_arg);
	priv_p->p_cmcbn.cm_op->cm_put_node(&priv_p->p_cmcbn, cmc_node);
}

/*
 * Input:
 * 	fp_head: list of fp_node to search
 * 	sreq_head: list of sub_request_node, in order with fp_head. Each node in the list must
 * 	have len of priv_p->p_block_size
 */
static void
cse_search_fetch(struct cse_interface *cse_p, struct list_head *fp_head, struct list_head *sreq_head,
		cse_callback callback, struct cse_crc_cb_node *arg)
{
	char *log_header = "cse_search_fetch";
	struct cse_private *priv_p = (struct cse_private *)cse_p->cs_private;
	struct list_head not_found_head, not_found_fp_head;
	struct sub_request_node *sreq_p;
	struct cse_mds_cb_node *cmc_node;
	struct fp_node *fp_nd;
	int i, not_found_num;

	INIT_LIST_HEAD(&not_found_head);
	INIT_LIST_HEAD(&not_found_fp_head);

	cse_search_list(cse_p, fp_head, sreq_head, &not_found_head, &not_found_fp_head, &not_found_num);
	if (list_empty(&not_found_head)) {
		/* All node have been founded, callback.*/
		_cse_fill_real_sreq_and_free(sreq_head, fp_head, priv_p->p_srn, priv_p->p_fpn);
		callback(0, arg);
	} else {
		/* Process and drop found list first*/
		_cse_fill_real_sreq_and_free(sreq_head, fp_head, priv_p->p_srn, priv_p->p_fpn);

		/* TODO Will replace with pp2*/
		struct iovec* vec = x_malloc(sizeof(struct iovec) * not_found_num);
		char *fp = x_malloc(priv_p->p_fp_size * not_found_num), *fpbuf = fp;
		if (! (vec && fp) ) {
			nid_log_error("%s: x_malloc failed.", log_header);
		}

		/* Make iovec and fp buffer for MDS*/
		sreq_p = list_first_entry(&not_found_head, struct sub_request_node, sr_list);
		i = 0;
		list_for_each_entry(fp_nd, struct fp_node, &not_found_fp_head, fp_list) {
			vec[i].iov_base = sreq_p->sr_buf;
			vec[i].iov_len = sreq_p->sr_len;
			memcpy(fpbuf, fp_nd->fp, priv_p->p_fp_size);
			sreq_p = list_first_entry(&sreq_p->sr_list, struct sub_request_node, sr_list);
			fpbuf += priv_p->p_fp_size;
			i++;
		}
		/* Make callback argument*/
		cmc_node = priv_p->p_cmcbn.cm_op->cm_get_node(&priv_p->p_cmcbn);
		cmc_node->cm_cse_p = cse_p;
		cmc_node->cm_vec = vec;
		cmc_node->cm_fp = fp;
		INIT_LIST_HEAD(&cmc_node->cm_not_found_head);
		INIT_LIST_HEAD(&cmc_node->cm_not_found_fp_head);
		list_splice_init(&not_found_head, &cmc_node->cm_not_found_head);
		list_splice_init(&not_found_fp_head, &cmc_node->cm_not_found_fp_head);
		cmc_node->cm_super_arg = arg;
		cmc_node->cm_super_callback = callback;

		arg->cc_rw->rw_op->rw_fetch_data(arg->cc_rw, vec, not_found_num, fp, (rw_callback2_fn)nse_search_fetch_callback, cmc_node);
	}

}

/*
 * Algorithm:
 * 	Insert contents into content table
 */
static void
cse_updatev(struct cse_interface *cse_p,
	struct iovec *iov, int iov_counter, struct list_head *fp_head)
{
	char *header = "cse_updatev";
	struct cse_private *priv_p = (struct cse_private *)cse_p->cs_private;
	struct fp_node *fp_np;
	size_t the_size;
	char *p;
	int i;

	nid_log_debug("%s: start ...", header);
	fp_np = list_first_entry(fp_head, struct fp_node, fp_list);
	for (i = 0; i < iov_counter; i++) {
		the_size = iov[i].iov_len;
		p = iov[i].iov_base;
		while (the_size) {
			__cse_insert(priv_p, fp_np, p);
			assert(the_size >= (size_t)priv_p->p_block_size);
			the_size -= priv_p->p_block_size;
			p += priv_p->p_block_size;
			fp_np = list_entry(fp_np->fp_list.next, struct fp_node, fp_list);
		}
	}
}

static void
cse_drop_block(struct cse_interface *cse_p, struct content_description_node *cd_np)
{
	struct cse_private *priv_p = (struct cse_private *)cse_p->cs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct fpc_interface *fpc_p = priv_p->p_fpc;
	struct cse_ht_slot *the_slot;
	uint64_t hval;

	hval = fpc_p->fpc_op->fpc_hvcalculate(fpc_p, cd_np->cn_fp) % priv_p->p_ht_size;
	the_slot = priv_p->p_hash_table + hval;
	lck_p->l_op->l_wlock(lck_p, &the_slot->s_lnode);
	list_del(&cd_np->cn_slist);
	lck_p->l_op->l_wunlock(lck_p, &the_slot->s_lnode);
}

static void
cse_info_hit(struct cse_interface *cse_p, struct cse_info_hit *info_hit)
{
	struct cse_private *priv_p = (struct cse_private *)cse_p->cs_private;
	info_hit->cs_hit_counter = priv_p->p_hit_counter;
	info_hit->cs_unhit_counter = priv_p->p_unhit_counter;
	nid_log_debug(" cse: hit_counter: %lu, unhit_counter : %lu", info_hit->cs_hit_counter, info_hit->cs_unhit_counter);

}

struct cse_operations cse_op = {
	.cs_search_list = cse_search_list,
	.cs_search_fetch = cse_search_fetch,
	.cs_updatev = cse_updatev,
	.cs_drop_block = cse_drop_block,
	.cs_check_fp = cse_check_fp,
	.cs_info_hit = cse_info_hit,
	.cs_recover_fp = cse_recover_fp,
};

/*
 * Algorithm:
 * 	Flushing in-memory cache data to disk (i.e ssd)
 */
static void *
_cse_sync_thread(void *p)
{
	char *log_header = "_cse_sync_thread";
	struct cse_interface *cse_p = (struct cse_interface *)p;
	struct cse_private *priv_p = (struct cse_private *)cse_p->cs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct rc_interface *rc_p = priv_p->p_rc;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct fpc_interface *fpc_p = priv_p->p_fpc;
	struct blksn_interface *blksn_p = priv_p->p_blksn;
	struct pp_page_node *pp_np;
	struct list_head space_head;
	struct content_description_node *cd_np, *cd_np2;
	struct block_size_node *bk_np, *bk_np2;
	uint64_t page_nr;
	uint64_t pp_pagesz;
	struct cse_ht_slot *the_slot;
	uint64_t hval;
	uint64_t bk_counter;

	nid_log_warning("%s: start ...", log_header);

next_page:
	if (priv_p->p_stop)
		goto out;

	pthread_mutex_lock(&priv_p->p_sync_lck);
	if (!priv_p->p_stop && list_empty(&priv_p->p_memcache_head)) {
		pthread_cond_wait(&priv_p->p_sync_cond, &priv_p->p_sync_lck);
	}
	pp_np = list_first_entry(&priv_p->p_memcache_head, struct pp_page_node, pp_list);
	list_del(&pp_np->pp_list);
	pthread_mutex_unlock(&priv_p->p_sync_lck);

	pp_pagesz = pp_p->pp_op->pp_get_pagesz(pp_p);
	page_nr = (pp_pagesz >> priv_p->p_block_shift);
	INIT_LIST_HEAD(&space_head);
	rc_p->rc_op->rc_flush_content(rc_p, pp_np->pp_page, page_nr, &space_head);

	bk_np = list_first_entry(&space_head, struct block_size_node, bsn_list);
	bk_counter = 0;
	list_for_each_entry_safe(cd_np, cd_np2, struct content_description_node, &pp_np->pp_flush_head, cn_plist) {
		list_del(&cd_np->cn_plist);
		hval = fpc_p->fpc_op->fpc_hvcalculate(fpc_p, cd_np->cn_fp) % priv_p->p_ht_size;
		the_slot = priv_p->p_hash_table + hval;
		lck_p->l_op->l_wlock(lck_p, &the_slot->s_lnode);
		cd_np->cn_is_ondisk = 1;			// data already been flushed to disk
		cd_np->cn_data = bk_np->bsn_index + bk_counter;	// position to disk
		lck_p->l_op->l_wunlock(lck_p, &the_slot->s_lnode);
		rc_p->rc_op->rc_insert_block(rc_p, cd_np);	// Add block to lru list waiting for reclaim
		bk_counter++;

		if (bk_counter >= bk_np->bsn_size) {
			assert(bk_counter == bk_np->bsn_size);
			bk_np2 = list_entry(bk_np->bsn_list.next, struct block_size_node, bsn_list);
			list_del(&bk_np->bsn_list);
			blksn_p->bsn_op->bsn_put_node(blksn_p, bk_np);
			bk_np = bk_np2;
			bk_counter = 0;
		}
	}
	assert(list_empty(&space_head));

	nid_log_debug("%s: pp_free_node ...", log_header);
	pp_p->pp_op->pp_free_node(pp_p, pp_np);

	goto next_page;

out:
	return NULL;
}

int
cse_initialization(struct cse_interface *cse_p, struct cse_setup *setup)
{
	char *log_header = "cse_initialization";
	struct cse_private *priv_p;
	struct lck_setup lck_setup;
	struct pp_interface *pp_p;
	struct fpn_interface *fpn_p;
	pthread_attr_t attr;
	pthread_t thread_id;
	struct cse_mds_cbn_setup cmc_setup;
	uint64_t i;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	cse_p->cs_op = &cse_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	cse_p->cs_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_rc = setup->rc;
	priv_p->p_srn = setup->srn;
	priv_p->p_pp = setup->pp;
	priv_p->p_fpn = setup->fpn;
	priv_p->p_cdn = setup->cdn;
	priv_p->p_fpc = setup->fpc;
	priv_p->p_blksn = setup->blksn;
	fpn_p = priv_p->p_fpn;
	priv_p->p_fp_size = fpn_p->fp_op->fp_get_fp_size(fpn_p);
	pp_p = priv_p->p_pp;
	priv_p->p_pagesz = pp_p->pp_op->pp_get_pagesz(pp_p);
	priv_p->p_cur_page = pp_p->pp_op->pp_get_node(pp_p);

	INIT_LIST_HEAD(&priv_p->p_cur_page->pp_flush_head);
	nid_log_debug("%s: pp_flush_head has been inited", log_header);
	priv_p->p_cur_page->pp_occupied = 0;
	priv_p->p_cur_page->pp_flushed = 0;

	INIT_LIST_HEAD(&priv_p->p_memcache_head);
	priv_p->p_block_shift = setup->block_shift;
	priv_p->p_block_size = (1UL << priv_p->p_block_shift);
	priv_p->p_block_mask = (priv_p->p_block_size - 1);

	priv_p->p_hit_counter = 0;
	priv_p->p_unhit_counter = 0;

	cmc_setup.allocator = priv_p->p_allocator;
	cmc_setup.seg_size = 128;
	cmc_setup.set_id = ALLOCATOR_SET_CSE_MDS_CB;
	cse_mds_cbn_initialization(&priv_p->p_cmcbn, &cmc_setup);

	lck_initialization(&priv_p->p_lck, &lck_setup);
	lck_node_init(&priv_p->p_lnode);
	pthread_mutex_init(&priv_p->p_sync_lck, NULL);
	pthread_cond_init(&priv_p->p_sync_cond, NULL);

	priv_p->p_ht_size = setup->d_blocks_num;
	priv_p->p_hash_table = x_calloc(priv_p->p_ht_size, sizeof(struct cse_ht_slot));
	for (i = 0; i < priv_p->p_ht_size; i++) {
		INIT_LIST_HEAD(&priv_p->p_hash_table[i].s_head);
		lck_node_init(&priv_p->p_hash_table[i].s_lnode);
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _cse_sync_thread, (void *)cse_p);

	return 0;
}
