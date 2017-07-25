/*
 * nse.c
 * 	Implementation of  of Namespace Search Engine Module
 */

#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <stdbool.h>
#include <unistd.h>

#include "lstn_if.h"
#include "nid_log.h"
#include "lck_if.h"
#include "fpn_if.h"
#include "brn_if.h"
#include "srn_if.h"
#include "rtree_if.h"
#include "brn_if.h"
#include "nse_if.h"
#include "nse_crc_cbn.h"
#include "nse_mdsn.h"
#include "nse_mds_cbn.h"
#include "allocator_if.h"
#include "rw_if.h"

struct nse_private {
	struct allocator_interface	*p_allocator;
	struct lck_interface		p_lck;
	struct lck_node			p_lnode;
	struct lck_node			p_update_lnode;
	struct rtree_interface		p_fp_rtree;
	struct list_head		p_fp_head;
	struct list_head		p_fp_lru_head;
	struct srn_interface		*p_srn;
	struct fpn_interface		*p_fpn;
	struct brn_interface		*p_brn;
	struct fp_node			*p_traverse_cur;
	struct nse_mdsn_interface	p_nse_mdsn;
	struct nse_mds_cbn_interface	p_nse_mds_cbn;
	struct lstn_interface		p_nse_lstn;
	struct nse_stat			p_nse_stat;
	int				p_fp_size;
	int				p_block_shift;
	int				p_block_size;
	int				p_block_mask;
	uint8_t				p_dropcache;
	uint8_t				p_traverse_start;
	char				p_to_stop;
};

static void *
_nse_reclaim_fp_thread(void *p) {
	struct nse_interface *nse_p = (struct nse_interface *)p;
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct rtree_interface *rtree_p = &priv_p->p_fp_rtree;
	struct brn_interface *brn_p = priv_p->p_brn;
	struct fpn_interface *fpn_p = priv_p->p_fpn;
	struct lstn_interface *lstn_p = &priv_p->p_nse_lstn;
	struct fp_node *fp_np = NULL, *fp_np2; // fp_np is used for keeping on go through lru list during thread running, be careful when touch it.
	struct bridge_node *br_np;
	uint64_t i, ref_count;
	struct list_head reclaim_head;
	INIT_LIST_HEAD(&reclaim_head);
	struct list_node *lst_np, *lst_np1;

next:
	if (priv_p->p_to_stop)
		goto out;

	if (priv_p->p_nse_stat.fp_num >= priv_p->p_nse_stat.fp_max_num) {
		lck_p->l_op->l_rlock(lck_p, &priv_p->p_lnode);
		if (!fp_np) {

			fp_np = list_first_entry(&priv_p->p_fp_lru_head, struct fp_node, fp_lru_list);
		}

		for (i=0; i< priv_p->p_nse_stat.fp_rec_num; i++) {
			if (fp_np->fp_lru_list.next != &priv_p->p_fp_lru_head) {
				fp_np2 = list_entry(fp_np->fp_lru_list.next, struct fp_node, fp_lru_list);
			} else {
				fp_np2 = list_first_entry(&priv_p->p_fp_lru_head, struct fp_node, fp_lru_list);
			}

			ref_count = __sync_fetch_and_sub(&fp_np->fp_ref, 1);
			if (ref_count == 1) {
				lst_np = lstn_p->ln_op->ln_get_node(lstn_p);
				lst_np->ln_data = fp_np;
				list_add_tail(&lst_np->ln_list, &reclaim_head);
			}

			fp_np = fp_np2;
		}
		lck_p->l_op->l_runlock(lck_p, &priv_p->p_lnode);

		/*
		 * start reclaim.
		 */
		lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
		list_for_each_entry_safe(lst_np, lst_np1, struct list_node, &reclaim_head, ln_list) {
			fp_np2 = lst_np->ln_data;
			list_del(&lst_np->ln_list);

			if (fp_np2->fp_ref == 0) {
				/*
				 * no one hold rlock/wlock and added fp_np->fp_ref during try to request wlock.
				 */
				list_del(&fp_np2->fp_lru_list);
				list_del(&fp_np2->fp_list);
				br_np = fp_np2->fp_parent;
				rtree_p->rt_op->rt_remove(rtree_p, br_np->bn_index, br_np);

				brn_p->bn_op->bn_put_node(brn_p, br_np);
				fpn_p->fp_op->fp_put_node(fpn_p, fp_np2);
				priv_p->p_nse_stat.fp_num--;
			}

			lstn_p->ln_op->ln_put_node(lstn_p, lst_np);
		}
		lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);
	}

	usleep(100000);
	goto next;

out:
	return NULL;
}

/*
 * Algorithm:
 * 	For every block in iov, search p_fp_rtree.
 * 	1> if the block has an fp entry in p_fp_rtree, use the new fp to replace the old one in p_fp_rtree
 * 	2> if th eblock does not have an entry in p_fp_rtree, add the new fp entry to p_fp_rtree
 *
 * Input:
 * 	offset: must be aligned by block_size
 * 	iov: every term must be multiple of block_size
 * Output:
 * 	fp_head: list_head of fp.
 */
static void
nse_updatev(struct nse_interface *nse_p, struct iovec *iov, int iov_counter,
	off_t offset, struct list_head *ag_fp_head)
{
//	char *log_header = "nse_updatev";
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	struct rtree_interface *rtree_p = &priv_p->p_fp_rtree;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct brn_interface *brn_p = priv_p->p_brn;
	struct fpn_interface *fpn_p = priv_p->p_fpn;
	struct bridge_node *br_np, *br_np2, *first_br_np;
	struct fp_node *fp_np, *fp_np2, *first_fp_np, *fp_np_ag;
	uint64_t key_index = (offset >> priv_p->p_block_shift), hint_index = 0;
	void *hint_parent = NULL;
	size_t the_size;
	int i, on_left;

//	nid_log_error("%s: offset:0x%lx %lu", log_header, offset, key_index);
	assert((offset & priv_p->p_block_mask) == 0);	// must be aligned
	fp_np_ag = list_entry(ag_fp_head->next, struct fp_node, fp_list);
	lck_p->l_op->l_wlock(lck_p, &priv_p->p_update_lnode);
	lck_p->l_op->l_rlock(lck_p, &priv_p->p_lnode);
	for (i = 0; i < iov_counter; i++) {
		the_size = iov[i].iov_len;
		assert((the_size & priv_p->p_block_mask) == 0);

		while (the_size) {
			/*
			 * Search rtree to get br_np
			 */
			br_np = rtree_p->rt_op->rt_hint_search_around(rtree_p, key_index, hint_index, hint_parent);
			if (br_np && br_np->bn_index == key_index) {
				/*
				 * Found the namespace, it got overwrite, update fp
				 */
				fp_np = (struct fp_node *)br_np->bn_data;
				assert(fp_np->fp_parent == br_np);
				memcpy(fp_np->fp, fp_np_ag->fp, priv_p->p_fp_size);	// update fp

				__sync_fetch_and_add(&fp_np->fp_ref, 1);
			} else {
				/*
				 * Not Found,  create new br node, insert to rtree
				 */
				br_np2 = brn_p->bn_op->bn_get_node(brn_p);
				fp_np2 = fpn_p->fp_op->fp_get_node(fpn_p);
				br_np2->bn_data = (void *)fp_np2;
				br_np2->bn_index = key_index;
				fp_np2->fp_parent = br_np2;
				memcpy(fp_np2->fp, fp_np_ag->fp, priv_p->p_fp_size);
				fp_np2->fp_ref = 1;

				if (!br_np) {

					/*
					 * insert rtree and p_fp_head must be protected by wlock of p_lnode
					 */
					lck_p->l_op->l_uplock(lck_p, &priv_p->p_lnode);
					rtree_p->rt_op->rt_insert(rtree_p, key_index, br_np2);
					if (list_empty(&priv_p->p_fp_head)) {
						list_add(&fp_np2->fp_list, &priv_p->p_fp_head);
					} else {
						first_fp_np = list_entry(priv_p->p_fp_head.next, struct fp_node , fp_list);
						first_br_np = first_fp_np->fp_parent;
						assert(first_br_np->bn_data == first_fp_np);
						if (first_br_np->bn_index > key_index) {
							/* smaller than all index in the rtree */
							list_add(&fp_np2->fp_list, &priv_p->p_fp_head);
						} else {
							/* larger than all index in the rtree */
							list_add_tail(&fp_np2->fp_list, &priv_p->p_fp_head);
						}
					}

					priv_p->p_nse_stat.fp_num++;
					list_add_tail(&fp_np2->fp_lru_list, &priv_p->p_fp_lru_head);
					lck_p->l_op->l_downlock(lck_p, &priv_p->p_lnode);
				} else {
					/*
					 *  Found a br_np, use it for quick finding the insert location of p_fp_head list.
					 */
					fp_np = (struct fp_node *)br_np->bn_data;
					assert(fp_np->fp_parent == br_np);

					/*
					 * Go through left
					 */
					while (br_np->bn_index > key_index) {
						if (fp_np->fp_list.prev == &priv_p->p_fp_head)
							break;

						fp_np = list_entry(fp_np->fp_list.prev, struct fp_node, fp_list);
						br_np = fp_np->fp_parent;
						assert(br_np->bn_data == fp_np);
						assert(br_np->bn_index != key_index);
					}

					/*
					 * Go through right 
					 */
					while (br_np->bn_index < key_index) {
						if (fp_np->fp_list.next == &priv_p->p_fp_head)
							break;

						fp_np = list_entry(fp_np->fp_list.next, struct fp_node, fp_list);
						br_np = fp_np->fp_parent;
						assert(br_np->bn_data == fp_np);
						assert(br_np->bn_index != key_index);
					}

					/*
					 * Now, br_np is the closest neighbor, but not sure which side
					 */
					if (br_np->bn_index > key_index)
						on_left = 1;
					else
						on_left = 0;

					/*
					 * insert rtree and p_fp_head must be protected by wlock of p_lnode
					 */
					lck_p->l_op->l_uplock(lck_p, &priv_p->p_lnode);
					rtree_p->rt_op->rt_insert(rtree_p, key_index, br_np2);
					if (on_left) {
						list_add_tail(&fp_np2->fp_list, &fp_np->fp_list);// prev to fp_np
					} else {
						list_add(&fp_np2->fp_list, &fp_np->fp_list);	// next to fp_np
					}

					priv_p->p_nse_stat.fp_num++;
					list_add_tail(&fp_np2->fp_lru_list, &priv_p->p_fp_lru_head);
					lck_p->l_op->l_downlock(lck_p, &priv_p->p_lnode);
				}

				br_np = br_np2;
			} // not found

			hint_index = key_index;
			hint_parent = br_np->bn_parent;
			key_index++;
			the_size -= priv_p->p_block_size;
			if (fp_np_ag->fp_list.next != ag_fp_head)
				fp_np_ag = list_entry(fp_np_ag->fp_list.next, struct fp_node, fp_list);
			else
				assert(!the_size && (i + 1 == iov_counter));
		} // while the_size
	} // for iov_counter 

	lck_p->l_op->l_runlock(lck_p, &priv_p->p_lnode);
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_update_lnode);
}

/*
 * Algorithm:
 *	search match blocks of a given request (rn_p) from p_fp_rtree
 *
 * Output:
 * 	found_head:
 * 		list of a sub requests on behalf of all matched blocks,
 * 		each sub request has size of one block
 * 	fp_found_head:
 *		list of fp correspondent of found_head
 * 	not_found_head:
 * 		list of a sub requests on behalf of all non-matched blocks,
 * 		each sub request contains number of contiguous blocks
 */
static void
nse_search(struct nse_interface *nse_p, struct sub_request_node *rn_p,
	struct list_head *fp_found_head, struct list_head *found_head, struct list_head *not_found_head)
{
//	char *log_header = "nse_search";
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	struct rtree_interface *rtree_p = &priv_p->p_fp_rtree;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct fpn_interface *fpn_p = priv_p->p_fpn;
	struct sub_request_node *new_rn;
	struct bridge_node *br_np;
	struct fp_node *fp_np = NULL, *new_fp = NULL;
	uint64_t first_index, last_index, pre_index;
	struct list_head not_found_head2;
	off_t delta_off, notalign_delta_off = 0;
	uint32_t notalign_len = 0, notalign_real_len = rn_p->sr_len;
	int got_nothing = 0;

	lck_p->l_op->l_rlock(lck_p, &priv_p->p_lnode);
	if ((rn_p->sr_offset & priv_p->p_block_mask) || (rn_p->sr_len & priv_p->p_block_mask)) {
		if ((rn_p->sr_offset & priv_p->p_block_mask)) {
			notalign_delta_off = priv_p->p_block_size - (rn_p->sr_offset & priv_p->p_block_mask);
			notalign_delta_off = notalign_delta_off > rn_p->sr_len ? rn_p->sr_len : notalign_delta_off;
			new_rn = srn_p->sr_op->sr_get_node(srn_p);
			new_rn->sr_buf = rn_p->sr_buf;
			new_rn->sr_offset = rn_p->sr_offset;
			new_rn->sr_len = notalign_delta_off;
			new_rn->sr_real = NULL;
			list_add_tail(&new_rn->sr_list, not_found_head);
		}
		notalign_len = (rn_p->sr_len - notalign_delta_off) & priv_p->p_block_mask;
		notalign_real_len = rn_p->sr_len - notalign_len - notalign_delta_off;
		if (notalign_len) {
			new_rn = srn_p->sr_op->sr_get_node(srn_p);
			new_rn->sr_buf = rn_p->sr_buf + notalign_delta_off + notalign_real_len;
			new_rn->sr_offset = rn_p->sr_offset + notalign_delta_off + notalign_real_len;
			new_rn->sr_len = notalign_len;
			new_rn->sr_real = NULL;
			list_add_tail(&new_rn->sr_list, not_found_head);
		}
		if (notalign_real_len == 0) {
			got_nothing = 1;
			goto out;
		}
	}

	INIT_LIST_HEAD(&not_found_head2);
	first_index = ((rn_p->sr_offset + notalign_delta_off) >> priv_p->p_block_shift);
	last_index = ((rn_p->sr_offset +  notalign_real_len - 1) >> priv_p->p_block_shift);

	br_np = (struct bridge_node *)rtree_p->rt_op->rt_search_around(rtree_p, first_index);
	if (br_np) {
		fp_np = (struct fp_node *)br_np->bn_data;
		assert(fp_np->fp_parent == br_np);

	} else if (!list_empty(&priv_p->p_fp_head)) {
		/*
		 * first_index is either smaller than all index in the rtree or larger than all
		 */
		fp_np = list_first_entry(&priv_p->p_fp_head, struct fp_node, fp_list);
		br_np = fp_np->fp_parent;
		assert(br_np->bn_data == fp_np);
		if (first_index > br_np->bn_index) {
			/*
			 * The first_index is larger than all index in the rtree
			 */
			got_nothing = 1;	
			goto out;
		}

	} else {
		/* the whole nse is empty */
		got_nothing = 1;
		goto out;
	}
	assert(fp_np);

	/*
	 * go to left
	 */
	while (br_np->bn_index > first_index) {
		assert(fp_np && br_np);
		if (fp_np->fp_list.prev == &priv_p->p_fp_head)
			break;
		fp_np = list_entry(fp_np->fp_list.prev, struct fp_node, fp_list);
		br_np = fp_np->fp_parent;
		assert(br_np->bn_data == fp_np);
	}

	/*
	 * there are scenario:
	 * 	1> fp_np is closest left neighbor of index_key
	 * 	2> fp_np is the index_key
	 * 	3> fp_np is the closest right neighbor of index_key
	 *
	 * go to right
	 */
	while (br_np->bn_index < first_index) {
		if (fp_np->fp_list.next == &priv_p->p_fp_head) {
	    		got_nothing = 1;	
			goto out;
		}
		fp_np = list_entry(fp_np->fp_list.next, struct fp_node, fp_list);
		br_np = fp_np->fp_parent;
		assert(br_np->bn_data == fp_np);
	}

	/*
	 * Now, there are two scenario:
	 * 	1> fp_np is the first_index
	 * 	2> fp_np is the closest right neighbor of first_index.
	 * If we found the bn_index already larger than last_index,
	 * means we found nothing, goto out
	 */
	if (br_np->bn_index > last_index) {
	    	got_nothing = 1;	
		goto out;
	}

	/*
	 * Now , we know we at least found one matched, i.e. br_np/fp_np
	 * since it is in [first_index, last_index]
	 * Start from left to find out matched/non-matched blocks
	 * pre_index: the first index not been processed yet.
	 */
	pre_index = first_index;
	while (br_np->bn_index <= last_index) {
		if (pre_index < br_np->bn_index) {
			/*
			 * got an index gap [pre_index, br_np->bn_index - 1]
			 */
			delta_off = ((pre_index - first_index) << priv_p->p_block_shift);
			new_rn = srn_p->sr_op->sr_get_node(srn_p);
			new_rn->sr_buf = rn_p->sr_buf + notalign_delta_off + delta_off;
			new_rn->sr_offset = rn_p->sr_offset + notalign_delta_off + delta_off;
			new_rn->sr_len = ((br_np->bn_index - pre_index) << priv_p->p_block_shift);
			new_rn->sr_real = NULL;
			list_add_tail(&new_rn->sr_list, not_found_head);
			pre_index = br_np->bn_index;	// br_np->bn_index - 1 processed
		} else {
			assert(pre_index == br_np->bn_index);
		}

		/*
		 * br_np->bn_index is found
		 */
		delta_off = ((br_np->bn_index - first_index) << priv_p->p_block_shift);
		new_rn = srn_p->sr_op->sr_get_node(srn_p);
		new_rn->sr_buf = rn_p->sr_buf + notalign_delta_off + delta_off;
		new_rn->sr_offset = rn_p->sr_offset + notalign_delta_off + delta_off;
		new_rn->sr_len = priv_p->p_block_size;	// always one block
		new_rn->sr_real = NULL;
		list_add_tail(&new_rn->sr_list, found_head);

		new_fp = fpn_p->fp_op->fp_get_node(fpn_p);
		memcpy(new_fp->fp, fp_np->fp, priv_p->p_fp_size);
		list_add_tail(&new_fp->fp_list, fp_found_head);
		pre_index = br_np->bn_index + 1;	// br_np->bn_index processed

		__sync_fetch_and_add(&fp_np->fp_ref, 1);

		/*
		 * goto next node
		 */
		if (fp_np->fp_list.next == &priv_p->p_fp_head)
			break;

		fp_np = list_entry(fp_np->fp_list.next, struct fp_node, fp_list);
		br_np = fp_np->fp_parent;
		assert(br_np->bn_data == fp_np);
		assert(pre_index <= br_np->bn_index);
	} // while

	if (pre_index <= last_index) {
		/*
		 * got index gap [pre_index, last_index]
		 */
		delta_off = ((pre_index - first_index) << priv_p->p_block_shift);
		new_rn = srn_p->sr_op->sr_get_node(srn_p);
		new_rn->sr_buf = rn_p->sr_buf + notalign_delta_off + delta_off;
		new_rn->sr_offset = rn_p->sr_offset + notalign_delta_off + delta_off;
		new_rn->sr_len = ((last_index - pre_index + 1) << priv_p->p_block_shift);
		new_rn->sr_real = NULL;
		list_add_tail(&new_rn->sr_list, not_found_head);
	}
	assert(!got_nothing);

out:
	lck_p->l_op->l_runlock(lck_p, &priv_p->p_lnode);
	if (got_nothing) {
		new_rn = srn_p->sr_op->sr_get_node(srn_p);
		new_rn->sr_buf = rn_p->sr_buf + notalign_delta_off;
		new_rn->sr_offset = rn_p->sr_offset + notalign_delta_off;
		new_rn->sr_len = notalign_real_len;
		new_rn->sr_real = NULL;
		list_add_tail(&new_rn->sr_list, not_found_head);
//		nid_log_error("%s: not found. offset:%lx", log_header, new_rn->sr_offset);
		__sync_add_and_fetch(&priv_p->p_nse_stat.unhit_num, 1);
	} else {
//		nid_log_error("%s: found. offset:%lx", log_header, new_rn->sr_offset);
		__sync_add_and_fetch(&priv_p->p_nse_stat.hit_num, 1);	
	}
}


static void
nse_search_fetch_callback(int err, void* arg)
{
	char *log_header = "nse_search_fetch_callback";
	struct nse_mds_cb_node *nmn_cb_p = arg;
	struct nse_mds_node *nmn_p = nmn_cb_p->nm_nse_mds;
	struct list_head *found_head, *fp_found_head;
	struct nse_private *priv_p = (struct nse_private *)nmn_p->nm_nse_p->ns_private;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct fpn_interface *fpn_p = priv_p->p_fpn;
	struct sub_request_node *new_rn, *sreq = nmn_cb_p->nm_sreq_p;
	struct fp_node *new_fp = NULL;
	uint32_t nblock, i, off, off_fp, dia_off;
	bool *fpmap;
	char *fpbuf;
	int cnt;

	found_head = &nmn_p->nm_super_arg->nc_found_head;
	fp_found_head = &nmn_p->nm_super_arg->nc_fp_found_head;

	if (err) {
		nmn_p->nm_errcode = err;
	} else {
		nblock = nmn_cb_p->nm_len >> priv_p->p_block_shift;
		fpbuf = nmn_cb_p->nm_fp;
		fpmap = nmn_cb_p->nm_fp_bitmap;
		dia_off = nmn_cb_p->nm_isalignment ? 0 : sreq->sr_offset - nmn_cb_p->nm_off;
		off = 0; off_fp = 0;
		for (i = 0; i < nblock; i++) {
			if (fpmap[i]) {
				// Found FP
				new_rn = srn_p->sr_op->sr_get_node(srn_p);
				if (nmn_cb_p->nm_isalignment || (i != 0 && i != nblock-1)) {
					new_rn->sr_buf = sreq->sr_buf + off - dia_off;
					new_rn->sr_offset = nmn_cb_p->nm_off + off;
					new_rn->sr_len = priv_p->p_block_size;
					new_rn->sr_real = NULL;
				} else {
					// Not alignment need more process
					if (i == 0 && i == nblock-1) {
						// Only one not alignment node
						new_rn->sr_buf = x_malloc(priv_p->p_block_size); // TODO use pp2 replace
						new_rn->sr_offset = nmn_cb_p->nm_off;
						new_rn->sr_len = priv_p->p_block_size;
						new_rn->sr_real = srn_p->sr_op->sr_get_node(srn_p);
						new_rn->sr_real->sr_buf = sreq->sr_buf;
						new_rn->sr_real->sr_offset = sreq->sr_offset;
						new_rn->sr_real->sr_len = sreq->sr_len;
						new_rn->sr_real->sr_real = NULL;
					} else if (i == 0 && i != nblock-1) {
						// First node not alignment
						new_rn->sr_buf = x_malloc(priv_p->p_block_size); // TODO use pp2 replace
						new_rn->sr_offset = nmn_cb_p->nm_off;
						new_rn->sr_len = priv_p->p_block_size;
						new_rn->sr_real = srn_p->sr_op->sr_get_node(srn_p);
						new_rn->sr_real->sr_buf = sreq->sr_buf;
						new_rn->sr_real->sr_offset = sreq->sr_offset;
						new_rn->sr_real->sr_len = priv_p->p_block_size - (sreq->sr_offset&priv_p->p_block_mask);
						new_rn->sr_real->sr_real = NULL;
					} else if (i !=0 && i == nblock-1) {
						// Last node not alignment
						new_rn->sr_buf = x_malloc(priv_p->p_block_size); // TODO use pp2 replace
						new_rn->sr_offset = nmn_cb_p->nm_off + off;
						new_rn->sr_len = priv_p->p_block_size;
						new_rn->sr_real = srn_p->sr_op->sr_get_node(srn_p);
						new_rn->sr_real->sr_buf = sreq->sr_buf + off - dia_off;
						new_rn->sr_real->sr_offset = new_rn->sr_offset;
						new_rn->sr_real->sr_len = (sreq->sr_offset + sreq->sr_len) & priv_p->p_block_mask;
						new_rn->sr_real->sr_real = NULL;
					} else {
						// Cannot go to here!!
						assert(0);
					}

					if (! new_rn->sr_buf ) {
						nid_log_error("%s: x_malloc failed.", log_header);
					}
				}
				list_add_tail(&new_rn->sr_list, found_head);
				new_fp = fpn_p->fp_op->fp_get_node(fpn_p);
				memcpy(new_fp->fp, fpbuf + off_fp , priv_p->p_fp_size);
				list_add_tail(&new_fp->fp_list, fp_found_head);
			}
			off += priv_p->p_block_size;
			off_fp += priv_p->p_fp_size;
		}
	}

	cnt = __sync_sub_and_fetch(&nmn_p->nm_sub_request_cnt, 1);
	if (cnt == 0) {
		// Do response
		nmn_p->nm_super_callback(nmn_p->nm_errcode, nmn_p->nm_super_arg);
		priv_p->p_nse_mdsn.nm_op->nm_put_node(&priv_p->p_nse_mdsn, nmn_p);
	}

	// TODO Use pp2 replace
	free(nmn_cb_p->nm_fp);
	free(nmn_cb_p->nm_fp_bitmap);
	srn_p->sr_op->sr_put_node(srn_p, sreq);
	priv_p->p_nse_mds_cbn.nm_op->nm_put_node(&priv_p->p_nse_mds_cbn, nmn_cb_p);

}


#define get_align_offset(isalign, alignment, offset, inttype) \
	(isalign == 1 ? offset : (~(((inttype)alignment)- 1)) & offset)

#define get_align_length(isalign, alignment, offset, len, newoffset, inttype) \
	(isalign == 1 ? len : (((~(((inttype)alignment)- 1)) & (offset+len-1)) + alignment - newoffset))

static void
nse_search_fetch(struct nse_interface *nse_p, struct sub_request_node *rn_p,
		nse_callback callback, struct nse_crc_cb_node *arg)
{
	char *log_header = "nse_search_fetch";
	struct list_head not_found_head, *found_head, *fp_found_head;
	struct sub_request_node *sreq_p;
	struct nse_mds_node *nmn_p;
	struct nse_mds_cb_node *nmn_cb_p;
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	uint32_t nblock;
	int cnt;

	INIT_LIST_HEAD(&not_found_head);
	found_head = &arg->nc_found_head;
	fp_found_head = &arg->nc_fp_found_head;

	nse_search(nse_p, rn_p, fp_found_head, found_head, &not_found_head);

	if (list_empty(&not_found_head)) {
		/* All nodes are found, return. */
		callback(0, arg);
	} else {
		/* Do MDS search of NSE. */
		nmn_p = priv_p->p_nse_mdsn.nm_op->nm_get_node(&priv_p->p_nse_mdsn);
		nmn_p->nm_sub_request_cnt = 1;
		nmn_p->nm_super_arg = arg;
		nmn_p->nm_super_callback = callback;
		nmn_p->nm_errcode = 0;
		nmn_p->nm_nse_p = nse_p;

		while (!list_empty(&not_found_head)) {
			sreq_p = list_first_entry(&not_found_head, struct sub_request_node, sr_list);
			list_del(&sreq_p->sr_list);

			nmn_cb_p = priv_p->p_nse_mds_cbn.nm_op->nm_get_node(&priv_p->p_nse_mds_cbn);
			nmn_cb_p->nm_sreq_p = sreq_p;
			nmn_cb_p->nm_nse_mds = nmn_p;
			/*Check if sub-request is alignment or not, if not alignment, fix it to alignment.*/
			nmn_cb_p->nm_isalignment = (bool)!((sreq_p->sr_offset & priv_p->p_block_mask) ||
				        (sreq_p->sr_len & priv_p->p_block_mask));
			nmn_cb_p->nm_off = get_align_offset(nmn_cb_p->nm_isalignment, priv_p->p_block_size, sreq_p->sr_offset, off_t);
			nmn_cb_p->nm_len = get_align_length(nmn_cb_p->nm_isalignment, priv_p->p_block_size, sreq_p->sr_offset,
					sreq_p->sr_len, nmn_cb_p->nm_off, off_t);
			nblock = nmn_cb_p->nm_len >> priv_p->p_block_shift;
			// TODO Use pp2 replace x_malloc.
			nmn_cb_p->nm_fp = x_malloc(32 * nblock);
			nmn_cb_p->nm_fp_bitmap = x_malloc(sizeof(bool) * nblock);
			if (! (nmn_cb_p->nm_fp && nmn_cb_p->nm_fp_bitmap) ) {
				nid_log_error("%s: x_malloc failed.", log_header);
			}

			__sync_add_and_fetch(&nmn_p->nm_sub_request_cnt, 1);

			arg->nc_rw->rw_op->rw_fetch_fp(arg->nc_rw, arg->nc_rw_handle, nmn_cb_p->nm_off, (size_t)nmn_cb_p->nm_len,
					(rw_callback2_fn)nse_search_fetch_callback, nmn_cb_p, nmn_cb_p->nm_fp, nmn_cb_p->nm_fp_bitmap);
		}
		cnt = __sync_sub_and_fetch(&nmn_p->nm_sub_request_cnt, 1);
		if (cnt == 0) {
			// Do response
			nmn_p->nm_super_callback(nmn_p->nm_errcode, nmn_p->nm_super_arg);
			priv_p->p_nse_mdsn.nm_op->nm_put_node(&priv_p->p_nse_mdsn, nmn_p);
		}
	}
}

static void
nse_dropcache_start(struct nse_interface *nse_p, int do_sync)
{
	char *log_header = "nse_dropcache_start";
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	nid_log_warning("%s: start ...", log_header);
	priv_p->p_dropcache = 1;
	if (do_sync) {
		// TODO: now
	} else {
		// TODO: by a thread
	}
}

static void
nse_dropcache_stop(struct nse_interface *nse_p)
{
	char *log_header = "nse_dropcache_stop";
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	nid_log_warning("%s: start ...", log_header);
	priv_p->p_dropcache = 0;
}

static void
nse_get_stat(struct nse_interface *nse_p, struct nse_stat *stat)
{
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	stat->fp_max_num = priv_p->p_nse_stat.fp_max_num;
	stat->fp_min_num = priv_p->p_nse_stat.fp_min_num;
	stat->fp_num = priv_p->p_nse_stat.fp_num;
	stat->fp_rec_num = priv_p->p_nse_stat.fp_rec_num;
	stat->hit_num = priv_p->p_nse_stat.hit_num;
	stat->unhit_num= priv_p->p_nse_stat.unhit_num;
}

static int
nse_traverse_start(struct nse_interface *nse_p)
{
	char *log_header = "nse_traverse_start";
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	struct lck_interface *lck_p = &priv_p->p_lck;

	nid_log_warning("%s: start ...", log_header);
	if (priv_p->p_traverse_start) {
		return -1;
	}

	lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
	if (priv_p->p_traverse_start) {
		/* unlikely, but could happen */
		lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);
		return -1;
	}
	assert(!priv_p->p_traverse_cur);
	if (!list_empty(&priv_p->p_fp_head))
		priv_p->p_traverse_cur = list_first_entry(&priv_p->p_fp_head, struct fp_node, fp_list);
	priv_p->p_traverse_start = 1;
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);
		
	lck_p->l_op->l_rlock(lck_p, &priv_p->p_lnode);
	return 0;
}

static void 
nse_traverse_stop(struct nse_interface *nse_p)
{
	char *log_header = "nse_traverse_stop";
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	struct lck_interface *lck_p = &priv_p->p_lck;

	nid_log_warning("%s: start ...", log_header);
	assert(priv_p->p_traverse_start);
	priv_p->p_traverse_cur = NULL;
	priv_p->p_traverse_start = 0;
	lck_p->l_op->l_runlock(lck_p, &priv_p->p_lnode);

}

static int
nse_traverse_next(struct nse_interface *nse_p, char *fp_buf, uint64_t *block_index)
{
	struct nse_private *priv_p = (struct nse_private *)nse_p->ns_private;
	struct fp_node *fp_np = priv_p->p_traverse_cur;
	struct bridge_node *br_np;

	if (!fp_np)
		return -1;

	br_np = fp_np->fp_parent;
	assert(br_np->bn_data == fp_np);
	memcpy(fp_buf, fp_np->fp, priv_p->p_fp_size);
	*block_index = br_np->bn_index;

	if (fp_np->fp_list.next != &priv_p->p_fp_head)
		priv_p->p_traverse_cur = list_entry(fp_np->fp_list.next, struct fp_node, fp_list);
	else
		priv_p->p_traverse_cur = NULL;

	return 0;
}

struct nse_operations nse_op = {
	.ns_updatev = nse_updatev,
	.ns_search = nse_search,
	.ns_search_fetch = nse_search_fetch,
	.ns_dropcache_start = nse_dropcache_start,
	.ns_dropcache_stop = nse_dropcache_stop,
	.ns_get_stat = nse_get_stat,
	.ns_traverse_start = nse_traverse_start,
	.ns_traverse_stop = nse_traverse_stop,
	.ns_traverse_next = nse_traverse_next,
};

static void
__nse_rtree_extend_cb(void *target_slot, void *parent)
{
	struct bridge_node *slot = (struct bridge_node *)target_slot;
	slot->bn_parent = parent;
}

static void *
__nse_rtree_insert_cb(void *target_slot, void *new_node, void *parent)
{
	struct bridge_node *slot = (struct bridge_node *)target_slot;
	if (!slot) {
		slot = new_node;
		slot->bn_parent = parent;
	}
	return slot;
}

static void *
__nse_rtree_remove_cb(void *target_slot, void *target_node)
{
	assert(!target_node || target_slot == target_node);
	return NULL;
}


static void 
__nse_rtree_shrink_to_root_cb(void *node )
{
  	assert ( node );
  	struct bridge_node* bn = (struct bridge_node*)( node );
	bn->bn_parent = NULL;
}

int
nse_initialization(struct nse_interface *nse_p, struct nse_setup *setup)
{
	char *log_header = "nse_initialization";
	struct nse_private *priv_p;
	struct rtree_interface *rtree_p;
	struct fpn_interface *fpn_p;
	struct rtree_setup rtree_setup;
	struct lck_setup lck_setup;
	struct nse_mdsn_setup nm_setup;
	struct nse_mds_cbn_setup nmcb_setup;
	struct lstn_setup lstn_setup;
	pthread_attr_t attr;
	pthread_t thread_id;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	nse_p->ns_op = &nse_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	nse_p->ns_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_brn = setup->brn;
	priv_p->p_srn = setup->srn;
	priv_p->p_fpn = setup->fpn;
	fpn_p = priv_p->p_fpn;
	priv_p->p_fp_size = fpn_p->fp_op->fp_get_fp_size(fpn_p);
	INIT_LIST_HEAD(&priv_p->p_fp_head);
	INIT_LIST_HEAD(&priv_p->p_fp_lru_head);
	priv_p->p_nse_stat.fp_max_num = setup->size * 4;	// four times of rc fp number.
	priv_p->p_nse_stat.fp_min_num = setup->size;
	priv_p->p_nse_stat.fp_rec_num = 2000;
	priv_p->p_block_shift = setup->block_shift;
	priv_p->p_block_size = (1UL << priv_p->p_block_shift);
	priv_p->p_block_mask = (priv_p->p_block_size - 1);
	priv_p->p_nse_stat.hit_num = 0;
	priv_p->p_nse_stat.unhit_num = 0;
	priv_p->p_to_stop = 0;
	rtree_p = &priv_p->p_fp_rtree;
	rtree_setup.allocator = priv_p->p_allocator;
	rtree_setup.extend_cb = __nse_rtree_extend_cb;
	rtree_setup.insert_cb = __nse_rtree_insert_cb;
	rtree_setup.remove_cb = __nse_rtree_remove_cb;
	rtree_setup.shrink_to_root_cb = __nse_rtree_shrink_to_root_cb;
	rtree_initialization(rtree_p, &rtree_setup);

	lck_initialization(&priv_p->p_lck, &lck_setup);
	lck_node_init(&priv_p->p_lnode);
	lck_node_init(&priv_p->p_update_lnode);

	nm_setup.allocator = priv_p->p_allocator;
	nm_setup.seg_size = 128;
	nm_setup.set_id = ALLOCATOR_SET_NSE_MDS;
	nse_mdsn_initialization(&priv_p->p_nse_mdsn, &nm_setup);

	nmcb_setup.allocator = priv_p->p_allocator;
	nmcb_setup.seg_size = 128;
	nmcb_setup.set_id = ALLOCATOR_SET_NSE_MDS_CB;
	nse_mds_cbn_initialization(&priv_p->p_nse_mds_cbn, &nmcb_setup);

	lstn_setup.allocator = priv_p->p_allocator;
	lstn_setup.seg_size = 4096;
	lstn_setup.set_id  = ALLOCATOR_SET_NSE_LSTN;
	lstn_initialization(&priv_p->p_nse_lstn, &lstn_setup);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _nse_reclaim_fp_thread, nse_p);

	return 0;
}
