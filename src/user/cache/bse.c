/*
 * bse.c
 * 	Implementation of  Buffer Search Engine Module
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/time.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "lck_if.h"
#include "allocator_if.h"
#include "pp_if.h"
#include "bsn_if.h"
#include "ddn_if.h"
#include "srn_if.h"
#include "bfe_if.h"
#include "sac_if.h"
#include "btn_if.h"
#include "rtree_if.h"
#include "bse_if.h"

#define BSE_SLOT_SHIFT	14
#define	BSE_SLOT_SIZE	(1 << BSE_SLOT_SHIFT)
#define BSE_MAX_TO_READ_DDN 1024

struct bse_private {
	struct allocator_interface	*p_allocator;
	struct lck_interface		p_lck;
	struct lck_node			p_lnode;
	struct pp_interface		*p_pp;
	struct bsn_interface		p_bsn;
	struct ddn_interface		*p_ddn;
	struct srn_interface		*p_srn;
	struct bfe_interface		*p_bfe;
	char				*p_bufdevice;
	struct btn_interface		p_btn;
	struct rtree_interface		p_rtree;
	struct list_head		p_ddn_head;
	struct bsn_node			*p_master;
	struct list_head		p_snapshot_head;
	uint64_t			p_devsz;
	uint32_t			p_cachesz;
	char				*p_segbuffer;
	int				p_fhandle;
	struct data_description_node	*p_traverse_start;
	struct data_description_node	*p_traverse_cur;
	char				p_traverse_cross_page;
	sem_t				p_ddn_sem;
};

static void
_bse_rtree_extend_cb(void *target_slot, void *parent)
{
	struct btn_node *slot = (struct btn_node *)target_slot;
	slot->n_parent = parent;
}

static void *
_bse_rtree_insert_cb(void *target_slot, void *new_node, void *parent)
{
	struct btn_node *slot = (struct btn_node *)target_slot;
	struct data_description_node *np,
	       *new_np = (struct data_description_node *)new_node;
	struct bse_interface *bse_p = (struct bse_interface *)new_np->d_interface;
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct btn_interface *btn_p = &priv_p->p_btn;
	int did_it = 0;

	if (!slot) {
		//slot = x_calloc(1, sizeof(*slot));
		slot = (struct btn_node *)btn_p->bt_op->bt_get_node(btn_p);
		INIT_LIST_HEAD(&slot->n_ddn_head);
	}
	slot->n_parent = parent;

	list_for_each_entry(np, struct data_description_node, &slot->n_ddn_head, d_list) {
		assert(np->d_offset != new_np->d_offset);
		if (np->d_offset > new_np->d_offset) {
			assert(np->d_offset > new_np->d_offset + new_np->d_len - 1);
			__list_add(&new_np->d_list, np->d_list.prev, &np->d_list);
			did_it = 1;
			break;
		}
	}

	if (!did_it)
		list_add_tail(&new_np->d_list, &slot->n_ddn_head);
	new_np->d_parent = slot;
	return slot;
}

static void *
_bse_rtree_remove_cb(void *target_slot, void *target_node)
{
	struct btn_node *slot = (struct btn_node *)target_slot;
	struct data_description_node *np = (struct data_description_node *)target_node;
	struct bse_interface *bse_p = (struct bse_interface *)np->d_interface;
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct btn_interface *btn_p = &priv_p->p_btn;

	if (!target_node)
		return NULL;

	assert(np->d_parent == slot);
	list_del(&np->d_list);	// removed from slot->n_ddn_head
	np->d_parent = NULL;
	if (list_empty(&slot->n_ddn_head)) {
		btn_p->bt_op->bt_put_node(btn_p, (struct btn_node *)slot);
		slot = NULL;
	}
	return slot;
}

static void
__bse_rtree_shrink_to_root_cb(void *node )
{
  	assert ( node );
  	struct btn_node* btn_p = (struct btn_node*)( node );
	btn_p->n_parent = NULL;
}

/*
 * Algorithm:
 * 	return a node on the left side of the search offset (soffset)
 * 	But just a try best, but no guarrentee the nearest one
 */
static struct list_head *
__bse_search_pre_node(struct bse_interface *bse_p, off_t soffset)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct btn_node *slot;
	struct list_head *pre_list;
	struct data_description_node *np;
	uint64_t index_key = (soffset >> BSE_SLOT_SHIFT);

	slot = (struct btn_node *)rtree_p->rt_op->rt_search_around(rtree_p, index_key);
	if (!slot) {
		if (!list_empty(&priv_p->p_ddn_head)) {
			pre_list = priv_p->p_ddn_head.prev;
			np = list_entry(pre_list, struct data_description_node, d_slist);
			if (np->d_offset > soffset)
				pre_list = &priv_p->p_ddn_head;
		} else {
			pre_list = &priv_p->p_ddn_head;
		}
		goto out;
	}

	assert(!list_empty(&slot->n_ddn_head));
	pre_list = slot->n_ddn_head.next;
	np = list_entry(pre_list, struct data_description_node, d_list);
	pre_list = &np->d_slist;
	do {
		np = list_entry(pre_list, struct data_description_node, d_slist);
		if (np->d_offset + np->d_len - 1 < soffset)
			break;
		pre_list = pre_list->prev;
	} while (pre_list != &priv_p->p_ddn_head);

out:
	return pre_list;
}

inline static void
__fill_buffer_content(struct bse_private *priv_p, char *dest, int64_t src,
	size_t sz, struct data_description_node *ddn, char** to_read_databuf,
	off_t *offset, size_t *size, int *to_read_count,
	struct data_description_node **to_read_ddn)
{
	(void)priv_p;
	if (ddn->d_location_disk) {
		__sync_add_and_fetch(&ddn->d_user_counter, 1);
		to_read_databuf[*to_read_count] = dest;
		size[*to_read_count] = sz;
		to_read_ddn[*to_read_count] = ddn;
		offset[(*to_read_count)++] = (off_t)src;
	} else {
		memcpy(dest, (char *)src, sz);
	}
}

/*
 * Algorithm:
 * 	search a range, fill all matched parts in given buffer at
 * 	correspondent positions.
 * 	And find out all non-matched parts.
 * Output:
 * 	case 1> sbuf is fully filled if all search range found.
 * 		not_found_head won't be touched
 * 	case 2> sbuf is partially filled (not necessary contigeous) if only
 *	 	part of search range found.
 * 	        all not found parts will be inserted into not_found_head
 */
static void
__bse_search_range(struct bse_interface *bse_p, char *sbuf, uint32_t slen,
	off_t soffset, struct list_head *not_found_head, char** to_read_databuf,
	off_t *to_read_offset, size_t *to_read_size, int *to_read_count,
	struct data_description_node **to_read_ddn)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct data_description_node *np, *first_np, *last_np, *m_np, *pre_m_np;
	struct list_head *pre_list = &priv_p->p_ddn_head,
		*post_list = &priv_p->p_ddn_head,
		*first_list, *last_list, *m_list, *pre_m_list, *cur_list;
	off_t srange_end = soffset + slen - 1,
		first_end, last_end, pre_m_end;	// end point offfset
	struct sub_request_node *sreq_p, *sreq_p2;
	uint32_t sub_len, pre_done = 0;

	pre_list = __bse_search_pre_node(bse_p, soffset);
	for (cur_list = pre_list->next; cur_list != &priv_p->p_ddn_head; cur_list = cur_list->next) {
		assert(cur_list);
		np = list_entry(cur_list, struct data_description_node, d_slist);
		if (np->d_offset + np->d_len - 1 < soffset) {
			assert(!pre_done);
			pre_list = &np->d_slist;
			continue;
		} else if (srange_end < np->d_offset) {
			pre_done = 1;
			post_list = &np->d_slist;
			break;
		}
		pre_done = 1;
	}
	assert(pre_done || ((pre_list->next == post_list) && (post_list == &priv_p->p_ddn_head)));

	if (pre_list->next == post_list) {
		/*
		 * There is no node overlapped with the search range
		 * There three 3 cases:
		 * 1> pre_list == post_list == &priv_p->p_ddn_head
		 * 2> pre_list == &priv_p->p_ddn_head
		 *    post_list != &priv_p->p_ddn_head
		 *    post_list = pre_list->next
		 * 3> pre_list != &priv_p->p_ddn_head
		 *    post_list == &priv_p->p_ddn_head
		 *    pre_list->next = post_list
		 */
		first_np = NULL;
		last_np = NULL;
	} else {
		/*
		 * There is at least one node overlapped with the search range
		 */
		first_list = pre_list->next;
		first_np = list_entry(first_list, struct data_description_node, d_slist);
		first_end = first_np->d_offset + first_np->d_len - 1;
		last_list = post_list->prev;
		last_np = list_entry(last_list, struct data_description_node, d_slist);
		last_end = last_np->d_offset + last_np->d_len - 1;
	}

	if (first_np == NULL) {
		/*
		 * There is no node overlapped with the search range,
		 * so the whole range is not found. Need a new sub-request node
		 */
		sreq_p = srn_p->sr_op->sr_get_node(srn_p);
		sreq_p->sr_buf = sbuf;
		sreq_p->sr_offset = soffset;
		sreq_p->sr_len = slen;
		list_add_tail(&sreq_p->sr_list, not_found_head);
	} else if (first_np == last_np) {
		/*
		 * There is only one node overlapped with the search range
		 */
		if (soffset < first_np->d_offset) {
			/*
			 * partial of left part of the search range is not found
			 */
			sub_len = first_np->d_offset - soffset;
			sreq_p = srn_p->sr_op->sr_get_node(srn_p);
			sreq_p->sr_buf = sbuf;
			sreq_p->sr_offset = soffset;
			sreq_p->sr_len = sub_len;
			list_add_tail(&sreq_p->sr_list, not_found_head);

			if (srange_end <= first_end) {
				/*
				 * the whole right part of the search range is covered by first_np
				 */
				assert(srange_end >= first_np->d_offset);
				assert(srange_end <= first_np->d_offset + first_np->d_len - 1);
//				memcpy(sbuf + sub_len, first_np->d_pos, slen - sub_len);
				__fill_buffer_content(priv_p, sbuf + sub_len, first_np->d_pos, slen - sub_len, first_np,
					to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
			} else {
				/*
				 * only middle part of the search rang is coverd by first_np.
				 * neither left nor right part is covered.
				 */
				assert(srange_end > first_np->d_offset + first_np->d_len - 1);
//				memcpy(sbuf + sub_len, first_np->d_pos, first_np->d_len);
				__fill_buffer_content(priv_p, sbuf + sub_len, first_np->d_pos, first_np->d_len, first_np,
					to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
				sub_len += first_np->d_len;
				sreq_p2 = srn_p->sr_op->sr_get_node(srn_p);
				sreq_p2->sr_buf = sbuf + sub_len;
				sreq_p2->sr_offset = soffset + sub_len;
				sreq_p2->sr_len = slen - sub_len;
				list_add_tail(&sreq_p2->sr_list, not_found_head);
			}
		} else {
			/*
			 * the left part of the search range is covered by first_np
			 */
			if (srange_end <= first_end) {
				/*
				 * the whole search range is covered by the first_np
				 */
				assert(srange_end <= first_np->d_offset + first_np->d_len - 1);
//				memcpy(sbuf, first_np->d_pos + (soffset - first_np->d_offset), slen);
				__fill_buffer_content(priv_p, sbuf, first_np->d_pos + (soffset - first_np->d_offset), slen, first_np,
					to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
			} else {
				/*
				 * the right part of the  search range is not found/coverd
				 */
				assert(srange_end > first_np->d_offset + first_np->d_len - 1);
				sub_len = first_end + 1 - soffset;
//				memcpy(sbuf, first_np->d_pos + (soffset - first_np->d_offset), sub_len);
				__fill_buffer_content(priv_p, sbuf, first_np->d_pos + (soffset - first_np->d_offset), sub_len, first_np,
					to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
				sreq_p2 = srn_p->sr_op->sr_get_node(srn_p);
				sreq_p2->sr_buf = sbuf + sub_len;
				sreq_p2->sr_offset = soffset + sub_len;
				sreq_p2->sr_len = slen - sub_len;
				list_add_tail(&sreq_p2->sr_list, not_found_head);
			}
		}
	} else {
		/*
		 * there are multiple nodes overlapped with the search range
		 */
		if (soffset < first_np->d_offset) {
			/*
			 * the left part of the search range is not found
			 */
			sub_len = first_np->d_offset - soffset;
			sreq_p = srn_p->sr_op->sr_get_node(srn_p);
			sreq_p->sr_buf = sbuf;
			sreq_p->sr_offset = soffset;
			sreq_p->sr_len = sub_len;
			list_add_tail(&sreq_p->sr_list, not_found_head);
//			memcpy(sbuf + sub_len, first_np->d_pos, first_np->d_len);
			__fill_buffer_content(priv_p, sbuf + sub_len, first_np->d_pos, first_np->d_len, first_np,
				to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
		} else {
			/*
			 * the left end of the search range is covered by first_np
			 */
			sub_len = soffset - first_np->d_offset;
//			memcpy(sbuf, first_np->d_pos + sub_len, first_np->d_len - sub_len);
			__fill_buffer_content(priv_p, sbuf, first_np->d_pos + sub_len,
				first_np->d_len - sub_len, first_np,
				to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
		}

		pre_m_list = first_list;
		m_list = pre_m_list->next;
		do {
			pre_m_np = list_entry(pre_m_list, struct data_description_node, d_slist);
			pre_m_end = pre_m_np->d_offset + pre_m_np->d_len - 1;
			m_np = list_entry(m_list, struct data_description_node, d_slist);
			sub_len = m_np->d_offset - (pre_m_end + 1);
			if (sub_len > 0) {
				/*
				 * there is a gap between pre_m_np and m_np
				 */
				sreq_p2 = srn_p->sr_op->sr_get_node(srn_p);
				sreq_p2->sr_buf = sbuf + (pre_m_end + 1 - soffset);
				sreq_p2->sr_offset = soffset + (pre_m_end + 1 - soffset);
				sreq_p2->sr_len = sub_len;
				list_add_tail(&sreq_p2->sr_list, not_found_head);
			}
			if (m_list != last_list)
//				memcpy(sbuf + (m_np->d_offset - soffset), m_np->d_pos, m_np->d_len);
				__fill_buffer_content(priv_p, sbuf + (m_np->d_offset - soffset),
					m_np->d_pos, m_np->d_len, m_np,
					to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
			pre_m_list = m_list;
			m_list = m_list->next;
		} while (pre_m_list != last_list);

		sub_len = last_np->d_offset - soffset;
		if (srange_end <= last_end) {
			/*
			 * the right part of the search range is covered the last_np
			 */
			assert(srange_end <= last_np->d_offset + last_np->d_len - 1);
//			memcpy(sbuf + sub_len, last_np->d_pos, slen - sub_len);
			__fill_buffer_content(priv_p, sbuf + sub_len, last_np->d_pos, slen - sub_len, last_np,
				to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
		} else {
			/*
			 * the right part of the  search range is not found/coverd
			 */
			assert(srange_end > last_np->d_offset + last_np->d_len - 1);
//			memcpy(sbuf + sub_len, last_np->d_pos, last_np->d_len);
			__fill_buffer_content(priv_p, sbuf + sub_len, last_np->d_pos, last_np->d_len, last_np,
				to_read_databuf, to_read_offset, to_read_size, to_read_count, to_read_ddn);
			sreq_p2 = srn_p->sr_op->sr_get_node(srn_p);
			sub_len += last_np->d_len;
			sreq_p2->sr_buf = sbuf + sub_len;
			sreq_p2->sr_offset = soffset + sub_len;
			sreq_p2->sr_len = slen - sub_len;
			list_add_tail(&sreq_p2->sr_list, not_found_head);
		}
	}
}

static void
bse_search(struct bse_interface *bse_p, struct request_node *rn_p)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct list_head not_found_head, not_found_head2;
	struct sub_request_node *sreq_p, *sreq_p2;
	struct bsn_node *np;
	struct ddn_interface *ddn_p = priv_p->p_ddn;
	struct data_description_node *to_read_ddn[BSE_MAX_TO_READ_DDN];
	char* to_read_databuf[BSE_MAX_TO_READ_DDN];
	off_t to_read_offset[BSE_MAX_TO_READ_DDN];
	size_t to_read_size[BSE_MAX_TO_READ_DDN];
	int to_read_count = 0, i, ret, left;


	INIT_LIST_HEAD(&not_found_head);
	INIT_LIST_HEAD(&not_found_head2);
	INIT_LIST_HEAD(&rn_p->r_head);

	lck_p->l_op->l_rlock(lck_p, &priv_p->p_lnode);

	__bse_search_range(bse_p, rn_p->r_resp_buf_1, rn_p->r_resp_len_1, rn_p->r_offset, &rn_p->r_head,
		to_read_databuf, to_read_offset, to_read_size, &to_read_count, to_read_ddn);

	if (!list_empty(&priv_p->p_snapshot_head) && !list_empty(&rn_p->r_head)) {
		list_for_each_entry(np, struct bsn_node, &priv_p->p_snapshot_head, bn_list) {
			list_for_each_entry_safe(sreq_p, sreq_p2, struct sub_request_node, &rn_p->r_head, sr_list) {
				list_del(&sreq_p->sr_list);

				// TODO: search

				list_splice_init(&rn_p->r_head, &not_found_head);
			}
		}
	}

	if (rn_p->r_resp_len_1 < rn_p->r_len) {
		__bse_search_range(bse_p, rn_p->r_resp_buf_2, rn_p->r_len - rn_p->r_resp_len_1,
			rn_p->r_offset + rn_p->r_resp_len_1, &rn_p->r_head,
			to_read_databuf, to_read_offset, to_read_size, &to_read_count, to_read_ddn);
		if (!list_empty(&priv_p->p_snapshot_head) && !list_empty(&rn_p->r_head)) {
			list_for_each_entry(np, struct bsn_node, &priv_p->p_snapshot_head, bn_list) {
				list_for_each_entry_safe(sreq_p, sreq_p2, struct sub_request_node, &rn_p->r_head, sr_list) {
					list_del(&sreq_p->sr_list);

					// TODO: search

					list_splice_init(&rn_p->r_head, &not_found_head2);
				}
			}

		}
	}

	if (!list_empty(&not_found_head))
		list_splice(&not_found_head, &rn_p->r_head);
	if (!list_empty(&not_found_head2))
		list_splice(&not_found_head2, &rn_p->r_head);

	lck_p->l_op->l_runlock(lck_p, &priv_p->p_lnode);

	assert(to_read_count <= BSE_MAX_TO_READ_DDN);
	for (i = 0; i < to_read_count; i++) {
		ret = pread(priv_p->p_fhandle, to_read_databuf[i], to_read_size[i], to_read_offset[i]);
		assert(ret >= 0);
		left = __sync_sub_and_fetch(&to_read_ddn[i]->d_user_counter, 1);
		if (left == 0) {
			// I'm last use one, put this node back to ddn_p;
			ddn_p->d_op->d_put_node(ddn_p, to_read_ddn[i]);
		}
		assert(left > 0);
	}
	if (to_read_count)
		sem_post(&priv_p->p_ddn_sem);

}

static int
__bse_right_extend(struct bse_interface *bse_p, struct data_description_node *np, struct data_description_node *new_np)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct bfe_interface *bfe_p = priv_p->p_bfe;
	struct lck_interface *lck_p = &priv_p->p_lck;
	uint32_t np_end;
	int rc = -1;

	if (!np)
		return -1;

	np_end = np->d_offset + np->d_len - 1;
	if ((np_end + 1 ==  new_np->d_offset) &&
	    (np->d_page_node == new_np->d_page_node) &&
	    (np->d_pos + np->d_len == new_np->d_pos)) {
		/*
		 * Need uplock here since bf_right_extend_node may change an existing node np
		 */
		lck_p->l_op->l_uplock(lck_p, &priv_p->p_lnode);
		rc = bfe_p->bf_op->bf_right_extend_node(bfe_p, np, new_np);
		lck_p->l_op->l_downlock(lck_p, &priv_p->p_lnode);
	}
	return rc;
}

static int
__bse_right_cut_adjust(struct bse_interface *bse_p, struct data_description_node *np,
	uint32_t cut_len, struct data_description_node *a_np, struct data_description_node *new_np)
{
	char *log_header = "__bse_right_cut_adjust";
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct bfe_interface *bfe_p = priv_p->p_bfe;
	struct lck_interface *lck_p = &priv_p->p_lck;
	int rc = 0;

	nid_log_debug("%s: np:%p, cut_len:%d", log_header, np, cut_len);
	lck_p->l_op->l_uplock(lck_p, &priv_p->p_lnode);
	if (a_np)
		rc = bfe_p->bf_op->bf_right_cut_node_adjust(bfe_p, np, cut_len, a_np, new_np);
	else
		bfe_p->bf_op->bf_right_cut_node(bfe_p, np, cut_len, new_np);
	lck_p->l_op->l_downlock(lck_p, &priv_p->p_lnode);
	return rc;
}

static void
__bse_left_cut_adjust(struct bse_interface *bse_p, struct data_description_node *np,
	uint32_t cut_len, struct data_description_node *new_np)
{
	char *log_header = "__bse_left_cut_adjust";
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct bfe_interface *bfe_p = priv_p->p_bfe;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct btn_node *slot;
	uint64_t new_index_key, old_index_key;

	nid_log_debug("%s: np:%p, cut_len:%d", log_header, np, cut_len);
	lck_p->l_op->l_uplock(lck_p, &priv_p->p_lnode);
	bfe_p->bf_op->bf_left_cut_node(bfe_p, np, cut_len, new_np);
	new_index_key = (np->d_offset >> BSE_SLOT_SHIFT);
	old_index_key = ((np->d_offset - cut_len) >> BSE_SLOT_SHIFT);
	if (new_index_key == old_index_key)
		goto out;

	slot = (struct btn_node *)np->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, old_index_key, np, slot->n_parent);
	rtree_p->rt_op->rt_insert(rtree_p, new_index_key, np);
out:
	lck_p->l_op->l_downlock(lck_p, &priv_p->p_lnode);
}

static void
__bse_remove_ddn(struct bse_interface *bse_p, struct data_description_node *np, struct data_description_node *new_np)
{
	char *log_header = "__bse_remove_ddn";
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct bfe_interface *bfe_p = priv_p->p_bfe;
	struct btn_node *slot;
	uint64_t index_key;

	nid_log_debug("%s: np:%p, slist:%p", log_header, np, &np->d_slist);
	index_key = (np->d_offset >> BSE_SLOT_SHIFT);
	slot = (struct btn_node *)np->d_parent;
	lck_p->l_op->l_uplock(lck_p, &priv_p->p_lnode);
	rtree_p->rt_op->rt_hint_remove(rtree_p, index_key, np, slot->n_parent);
	bfe_p->bf_op->bf_del_node(bfe_p, np, new_np);	// removing an existing node need uplock
	list_del(&np->d_slist);				// removed from the search list
	lck_p->l_op->l_downlock(lck_p, &priv_p->p_lnode);
}

static void
__bse_add_ddn(struct bse_interface *bse_p, struct data_description_node *np, struct list_head *pre_list)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct data_description_node *pre_np = NULL, *post_np = NULL;
	struct list_head *post_list = pre_list->next;
	uint64_t index_key;

	if (pre_list != &priv_p->p_ddn_head)
		pre_np = list_entry(pre_list, struct data_description_node, d_slist);
	if (post_list != &priv_p->p_ddn_head)
		post_np = list_entry(post_list, struct data_description_node, d_slist);
	if (pre_np)
		assert(pre_np->d_offset + pre_np->d_len - 1 < np->d_offset);
	if (post_np)
		assert(np->d_offset + np->d_len - 1 < post_np->d_offset);

	index_key = (np->d_offset >> BSE_SLOT_SHIFT);
	lck_p->l_op->l_uplock(lck_p, &priv_p->p_lnode);
	__list_add(&np->d_slist, pre_list, pre_list->next);
	rtree_p->rt_op->rt_insert(rtree_p, index_key, np);
	lck_p->l_op->l_downlock(lck_p, &priv_p->p_lnode);
}

static void
__bse_insert_node(struct bse_interface *bse_p, struct data_description_node *new_np)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct ddn_interface *ddn_p = priv_p->p_ddn;
	struct bfe_interface *bfe_p = priv_p->p_bfe;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct data_description_node *np,
		*pre_np = NULL, *first_np, *last_np, *m_np;
	struct list_head *pre_list = &priv_p->p_ddn_head,
		 *post_list = &priv_p->p_ddn_head,
		 *first_list,		// the first node which has overlap with new_np
		 *last_list,		// the last node which has overlap with new_np
		 *m_list = NULL,	// nodes between
		 *cur_list;
	off_t new_end = new_np->d_offset + new_np->d_len - 1,
		first_end, last_end;	// end opoint offfset
	uint32_t cut_len;
	int rc, pre_done = 0;

	lck_p->l_op->l_rlock(lck_p, &priv_p->p_lnode);
	/*
	 * Find the position where this new node should be inserted
	 * Keep in mind that p_ddn_head is offset increasingly ordered
	 */
	pre_list = __bse_search_pre_node(bse_p, new_np->d_offset);
	for (cur_list = pre_list->next; cur_list != &priv_p->p_ddn_head; cur_list = cur_list->next) {
		assert(cur_list);
		np = list_entry(cur_list, struct data_description_node, d_slist);
		if (np->d_offset + np->d_len - 1 < new_np->d_offset) {
			assert(!pre_done);
			pre_list = &np->d_slist;
			continue;
		} else if (new_end < np->d_offset) {
			post_list = &np->d_slist;
			pre_done = 1;
			break;
		}
		pre_done = 1;
	}
	assert(pre_done || ((pre_list->next == post_list) && (post_list == &priv_p->p_ddn_head)));

	/*
	 * Nodes pointed by pre_list and post_list have no overlap with the new node
	 * But all nodes between pre_list and post_list should be ovelapped with the new node
	 */
	if (pre_list != &priv_p->p_ddn_head) {
		pre_np = list_entry(pre_list, struct data_description_node, d_slist);
	}

	if (pre_list->next == post_list) {
		/*
		 * There is no node overlapped with this new node
		 */
		first_np = NULL;
		last_np = NULL;
	} else {
		/*
		 * There is at least one node overlapped with the new node
		 */
		first_list = pre_list->next;
		first_np = list_entry(first_list, struct data_description_node, d_slist);
		first_end = first_np->d_offset + first_np->d_len - 1;
		last_list = post_list->prev;
		last_np = list_entry(last_list, struct data_description_node, d_slist);
		last_end = last_np->d_offset + last_np->d_len - 1;
		assert(first_np);
		assert(last_np);
	}

	if (first_np == NULL) {
		/*
		 * there is no node overlapped with the new node (new_np)
		 * Insert it between pre_list and post_list
		 */
		rc = __bse_right_extend(bse_p, pre_np, new_np);
		if (!rc) {
			// rc == 0 means, pre_np node and new_np node are seamlessly
			// side by side, it will reduce IO number, because two node merged
			// into 1 big node
			if (new_np->d_where == DDN_WHERE_NO)
				ddn_p->d_op->d_put_node(ddn_p, new_np);
			else
				assert(new_np->d_where == DDN_WHERE_SHADOW);
		} else {
			assert(pre_list->next == post_list);
			__bse_add_ddn(bse_p, new_np, pre_list);
			bfe_p->bf_op->bf_add_node(bfe_p, new_np);
		}
	} else if (first_np == last_np) {
		/*
		 * there is only one node overlapped with the new node
		 */

		if (new_np->d_offset <= first_np->d_offset) {
			if (new_end < first_end) {
				/* first_np is partially covered by new_np */
				cut_len = new_end - first_np->d_offset + 1;
				__bse_left_cut_adjust(bse_p, first_np, cut_len, new_np);
			} else {
				/* first_np is fully covered by new_np */
				__bse_remove_ddn(bse_p, first_np, new_np);
			}

			rc = __bse_right_extend(bse_p, pre_np, new_np);
			if (!rc) {
				// TODO: if rc == 0, it means success merger pre_np and new_np
				// we must increase continuous index
				if (new_np->d_where == DDN_WHERE_NO)
					ddn_p->d_op->d_put_node(ddn_p, new_np);
				else
					assert(new_np->d_where == DDN_WHERE_SHADOW);
			} else {
				__bse_add_ddn(bse_p, new_np, pre_list);
				bfe_p->bf_op->bf_add_node(bfe_p, new_np);
			}
		} else {
			uint32_t adjust_len = 0;

			cut_len = first_end - new_np->d_offset + 1;
			if (new_end < first_end)
				adjust_len = first_end - new_end;
			np = NULL;
			if (adjust_len) {
				np = ddn_p->d_op->d_get_node(ddn_p);
				np->d_seq = first_np->d_seq;
				np->d_interface = bse_p;
				np->d_page_node = first_np->d_page_node;
				np->d_pos = first_np->d_pos + (new_end - first_np->d_offset + 1);
				np->d_offset = new_end + 1;
				np->d_len = adjust_len;
				np->d_released = first_np->d_released;
				np->d_where = DDN_WHERE_NO;
				np->d_flush_overwritten = 0;
				np->d_callback_arg = NULL;
				np->d_location_disk = first_np->d_location_disk;
			}

			if (np) {
				rc = __bse_right_cut_adjust(bse_p, first_np, cut_len, np, new_np);
				if (rc) {
					assert(0);	// should never be failed
				} else {
					__bse_add_ddn(bse_p, np, first_list);
				}
			} else {
				__bse_right_cut_adjust(bse_p, first_np, cut_len, NULL, new_np);
			}

			__bse_add_ddn(bse_p, new_np, first_list);
			bfe_p->bf_op->bf_add_node(bfe_p, new_np);
		}
	} else {
		int first_removed = 0;
		int overlap_num = 0;
		/*
		 * there are multiple nodes overlapped
		 */

		/*
		 * update the first node overlapped with the new_np
		 */
		overlap_num++;

		m_list = first_list->next;
		if (new_np->d_offset <= first_np->d_offset) {
			/*
			 * the whole first_np got overlapped
			 */
			__bse_remove_ddn(bse_p, first_np, new_np);
			first_removed = 1;
		} else {
			/* partial of first_np got cut */
			cut_len = first_end - new_np->d_offset + 1;
			__bse_right_cut_adjust(bse_p, first_np, cut_len, NULL, new_np);
		}

		/*
		 * delete all nodes between new_np and last_np, if there are
		 * Since the above step may already change first_np
		 */
		while (m_list != last_list) {
			m_np = list_entry(m_list, struct data_description_node, d_slist);
			m_list = m_list->next;	// tricky: don't move this line after m_np deleted
			__bse_remove_ddn(bse_p, m_np, new_np);
			overlap_num++;
		}

		/*
		 * update the last node overlapped with the new_np
		 */
		overlap_num++;

		if (new_end < last_end) {
			/* part of last_np got cut */
			cut_len = new_end - last_np->d_offset + 1;
			__bse_left_cut_adjust(bse_p, last_np, cut_len, new_np);
		} else {
			/* the whole last_np got cut */
			__bse_remove_ddn(bse_p, last_np, new_np);
		}

		rc = __bse_right_extend(bse_p, pre_np, new_np);
		if (!rc) {
			if (new_np->d_where == DDN_WHERE_NO)
				ddn_p->d_op->d_put_node(ddn_p, new_np);
			else
				assert(new_np->d_where == DDN_WHERE_SHADOW);
		} else {
			if (first_removed)
				__bse_add_ddn(bse_p, new_np, pre_list);
			else
				__bse_add_ddn(bse_p, new_np, first_list);
			bfe_p->bf_op->bf_add_node(bfe_p, new_np);
		}
	}
	lck_p->l_op->l_runlock(lck_p, &priv_p->p_lnode);
}

/*
 * Algorithm:
 * 	stop if met a flushed node
 * Input:
 * 	dnp: in flushing state
 * Return:
 * 	a low_dnp: all dnps from the low_dnp to the input dnp will be set flushing status
 * Notice:
 * 	the call should hold p_lnode
 */
static struct data_description_node *
bse_contiguous_low_bound(struct bse_interface *bse_p, struct data_description_node *dnp, int max, int max_size)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct list_head *cur_list, *pre_list;
	struct data_description_node *pre_dnp, *cur_dnp = dnp;
	int cur_max_size = max_size;
	//uint32_t page_sz = priv_p->p_pp->pp_op->pp_get_pagesz(priv_p->p_pp);

	assert(!dnp->d_flushed && dnp->d_flushing);
	cur_list = &dnp->d_slist;

next_node:
	if (!max--)
		goto out;

	/*
	 * Need to confirm the dnp is in the search list
	 * It is necessary to check if the dnp does in the search list (cur_list->prev != NULL) since
	 * the caller may give us a dnp which has not joined the search list yet
	 */
	if (cur_list->prev && cur_list->prev != &priv_p->p_ddn_head) {
		pre_list = cur_list->prev;
		pre_dnp = list_entry(pre_list, struct data_description_node, d_slist);

		/*
		 * make sure this pre_dnp has already joined bfe module
		 * a dnp joins bse before bfe.
		 */
		if (pre_dnp->d_where == DDN_WHERE_NO)
			goto out;

		if (!pre_dnp->d_flushing) {

			if (max_size) {
				if (cur_max_size < (int)pre_dnp->d_len)
					goto out;
				cur_max_size -= pre_dnp->d_len;
			}

			if (!cur_dnp->d_flushing) {
				if (pre_dnp->d_offset + pre_dnp->d_len == cur_dnp->d_offset) {
					cur_list = pre_list;
					cur_dnp = list_entry(cur_list, struct data_description_node, d_slist);
					goto next_node;
				}
			} else {
				if (pre_dnp->d_offset + pre_dnp->d_len == cur_dnp->d_flush_offset) {
					cur_list = pre_list;
					cur_dnp = list_entry(cur_list, struct data_description_node, d_slist);
						goto next_node;
				}
			}

		} else if (!pre_dnp->d_flushed) {
			if (max_size) {
				if (cur_max_size < (int)pre_dnp->d_flush_len)
					goto out;
				cur_max_size -= pre_dnp->d_flush_len;
			}

			if (!cur_dnp->d_flushing) {
				if (pre_dnp->d_flush_offset + pre_dnp->d_flush_len == cur_dnp->d_offset) {
					cur_list = pre_list;
					cur_dnp = list_entry(cur_list, struct data_description_node, d_slist);
					goto next_node;
				}
			} else {
				if (pre_dnp->d_flush_offset + pre_dnp->d_flush_len == cur_dnp->d_flush_offset) {
					cur_list = pre_list;
					cur_dnp = list_entry(cur_list, struct data_description_node, d_slist);
					goto next_node;
				}
			}
		}
	}

out:
	return cur_dnp;
}

/*
 * Algorithm:
 * 	stop if met a flushed node
 * Input:
 * 	dnp: in flushing state
 * Return:
 * 	a low_dnp: all dnps from the low_dnp to the input dnp will be set flushing status
 * Notice:
 * 	the call should hold p_lnode
 */
static struct data_description_node *
bse_contiguous_low_bound_async(struct bse_interface *bse_p, struct data_description_node *dnp, int max)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct list_head *cur_list, *pre_list;
	struct data_description_node *pre_dnp, *cur_dnp = dnp;
	uint32_t page_sz = priv_p->p_pp->pp_op->pp_get_pagesz(priv_p->p_pp);

	assert(!dnp->d_flushed && dnp->d_flushing);
	cur_list = &dnp->d_slist;

next_node:
	if (!max--)
		goto out;

	/*
	 * Need to confirm the dnp is in the search list
	 * It is necessary to check if the dnp does in the search list (cur_list->prev != NULL) since
	 * the caller may give us a dnp which has not joined the search list yet
	 */
	if (cur_list->prev && cur_list->prev != &priv_p->p_ddn_head) {
		pre_list = cur_list->prev;
		pre_dnp = list_entry(pre_list, struct data_description_node, d_slist);

		/*
		 * make sure this pre_dnp has already joined bfe module
		 * a dnp joins bse before bfe.
		 */
		if (pre_dnp->d_where == DDN_WHERE_NO)
			goto out;

		/*
		 * If pre_dnp is waiting for some other dnp to flushed back, stop
		 */
		if (pre_dnp->d_flush_overwritten) {
			goto out;
		}

		/* Limit traverse in a page */
		if ((!priv_p->p_traverse_cross_page) && pre_dnp->d_page_node != dnp->d_page_node) {
			goto out;
		}

#if 1
		/* If the node occupied number not equal page size, can't join traverse write */
		if (pre_dnp->d_page_node->pp_occupied != page_sz) {
			goto out;
		}
#endif

		if (!pre_dnp->d_flushing) {
			if (!cur_dnp->d_flushing) {
				if (pre_dnp->d_offset + pre_dnp->d_len == cur_dnp->d_offset) {
					cur_list = pre_list;
					cur_dnp = list_entry(cur_list, struct data_description_node, d_slist);
					goto next_node;
				}
			} else {
				if (pre_dnp->d_offset + pre_dnp->d_len == cur_dnp->d_flush_offset) {
					cur_list = pre_list;
					cur_dnp = list_entry(cur_list, struct data_description_node, d_slist);
						goto next_node;
				}
			}

		} else if (!pre_dnp->d_flushed) {
			if (!cur_dnp->d_flushing) {
				if (pre_dnp->d_flush_offset + pre_dnp->d_flush_len == cur_dnp->d_offset) {
					cur_list = pre_list;
					cur_dnp = list_entry(cur_list, struct data_description_node, d_slist);
					goto next_node;
				}
			} else {
				if (pre_dnp->d_flush_offset + pre_dnp->d_flush_len == cur_dnp->d_flush_offset) {
					cur_list = pre_list;
					cur_dnp = list_entry(cur_list, struct data_description_node, d_slist);
					goto next_node;
				}
			}
		}
	}

out:
	return cur_dnp;
}

static void
bse_traverse_start(struct bse_interface *bse_p, struct data_description_node *dnp)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	assert(!priv_p->p_traverse_start);
	priv_p->p_traverse_start = dnp;
	priv_p->p_traverse_cur = priv_p->p_traverse_start;
	lck_p->l_op->l_rlock(lck_p, &priv_p->p_lnode);
}

static void
bse_traverse_set(struct bse_interface *bse_p, struct data_description_node *start_dnp)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	priv_p->p_traverse_start = start_dnp;
	priv_p->p_traverse_cur = priv_p->p_traverse_start;
}

static void
bse_traverse_end(struct bse_interface *bse_p)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	priv_p->p_traverse_start = NULL;
	priv_p->p_traverse_cur = NULL;
	lck_p->l_op->l_runlock(lck_p, &priv_p->p_lnode);
}

static struct data_description_node *
bse_traverse_next(struct bse_interface *bse_p)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct data_description_node *cur_dnp = priv_p->p_traverse_cur, *next_dnp;
	struct list_head *next_list;

	if (!cur_dnp)
		goto out;

	/*
	 * before return, need to setup p_traverse_cur
	 */
	next_list = cur_dnp->d_slist.next;
	if (next_list && next_list != &priv_p->p_ddn_head) {
		next_dnp = list_entry(next_list, struct data_description_node, d_slist);

		/*
		 * make sure the next dnp has already joined bfe module
		 * a dnp joins bse before bfe.
		 */
		if (next_dnp->d_where == DDN_WHERE_NO)
			priv_p->p_traverse_cur = NULL;
		else
			priv_p->p_traverse_cur = list_entry(next_list, struct data_description_node, d_slist);

	} else {
		priv_p->p_traverse_cur = NULL;
	}

out:
	return cur_dnp;
}

static struct data_description_node *
bse_traverse_next_async(struct bse_interface *bse_p)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct data_description_node *cur_dnp, *next_dnp;
	struct list_head *next_list;
	uint32_t page_sz = priv_p->p_pp->pp_op->pp_get_pagesz(priv_p->p_pp);

	if (!priv_p->p_traverse_cur)
		return NULL;

	cur_dnp = priv_p->p_traverse_cur;

	/*
	 * before return, need to setup p_traverse_cur for next traverse
	 */
	next_list = cur_dnp->d_slist.next;
	if (next_list && next_list != &priv_p->p_ddn_head) {
		next_dnp = list_entry(next_list, struct data_description_node, d_slist);

		/*
		 * make sure the next dnp has already joined bfe module
		 * a dnp joins bse before bfe.
		 */
		if (!next_dnp->d_flist.next) {
			assert(!next_dnp->d_flist.prev);
			priv_p->p_traverse_cur = NULL;
		} else {
			// Limit traverse in a page
			if ((!priv_p->p_traverse_cross_page) && priv_p->p_traverse_cur->d_page_node != cur_dnp->d_page_node) {
				priv_p->p_traverse_cur = NULL;
			} else if (priv_p->p_traverse_cur->d_flush_overwritten) {
				/*
				 * If pre_dnp is waiting for some other dnp to flushed back, stop
				 */
				priv_p->p_traverse_cur = NULL;
#if 1
			} else if (priv_p->p_traverse_cur && priv_p->p_traverse_cur->d_page_node->pp_occupied != page_sz) {
				/*
				 * If the node occupied number not equal page size, stop traverse write
				 */
				priv_p->p_traverse_cur = NULL;
#endif
			} else {
				priv_p->p_traverse_cur = list_entry(next_list, struct data_description_node, d_slist);
			}
		}

	} else {
			priv_p->p_traverse_cur = NULL;
	}

	return cur_dnp;
}

static void
bse_add_list(struct bse_interface *bse_p, struct list_head *ddn_head)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct data_description_node *np, *np1;

	assert(priv_p);
	list_for_each_entry_safe(np, np1, struct data_description_node, ddn_head, d_slist) {
		list_del(&np->d_slist);
		np->d_interface = bse_p;
		__bse_insert_node(bse_p, np);
	}
}

static void
bse_del_node(struct bse_interface *bse_p, struct data_description_node *np)
{
	char *log_header = "bse_del_node";
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	uint64_t index_key;
	struct btn_node *slot;

	nid_log_debug("%s: np:%p, next:%p", log_header, np, np->d_slist.next);

	/*
	 * First of all, remove the ddn from the search rtree, so no body else
	 * can use it any more
	 */
	lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
	if (np->d_slist.next) {
		list_del(&np->d_slist);
		index_key = (np->d_offset >> BSE_SLOT_SHIFT);
		slot = (struct btn_node *)np->d_parent;
		rtree_p->rt_op->rt_hint_remove(rtree_p, index_key, np, slot->n_parent);
	}
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);

	while (np->d_user_counter != 1) {
		/*
		 * somebody else are using this node, wait until them free this
		 * ddn Once we get here, it is guaranteed that no one else
		 * will increase the user_counter any more since it's has already
		 * been removed from the searching rtree.
		 */
		struct timeval now;
		struct timespec ts;

		gettimeofday(&now, NULL);
		__suseconds_t wait_time_ms = 100000; // Wait at most 100ms
		if (now.tv_usec + wait_time_ms < 1000000) {
			ts.tv_sec = now.tv_sec;
			ts.tv_nsec = (now.tv_usec + wait_time_ms) * 1000;
		} else {
			ts.tv_sec = now.tv_sec + 1;
			ts.tv_nsec = (now.tv_usec + wait_time_ms - 1000000) * 1000;
		}
		sem_timedwait(&priv_p->p_ddn_sem, &ts);
	}
}

static void
bse_close(struct bse_interface *bse_p)
{
	char *log_header = "bse_close";
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct btn_interface *btn_p = &priv_p->p_btn;

	nid_log_notice("%s: start ...", log_header);
	rtree_p->rt_op->rt_close(rtree_p);
	btn_p->bt_op->bt_destroy(btn_p);
	lck_p->l_op->l_destroy(lck_p);
	if (priv_p->p_fhandle > 0)
		close(priv_p->p_fhandle);
	free(priv_p);
	bse_p->bs_private = NULL;
}

static void
bse_get_chan_stat(struct bse_interface *bse_p, struct io_chan_stat *stat)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct rtree_stat rt_stat;
	rtree_p->rt_op->rt_get_stat(rtree_p, &rt_stat);
	stat->s_rtree_nseg = rt_stat.s_rtn_nseg;
	stat->s_rtree_segsz = rt_stat.s_rtn_segsz;
	stat->s_rtree_nfree = rt_stat.s_rtn_nfree;
	stat->s_rtree_nused = rt_stat.s_rtn_nused;
}

/*
 * Input:
 * 	sid: snapshot id
 */
static void
bse_create_snapshot(struct bse_interface *bse_p)
{
	struct bse_private *priv_p = (struct bse_private *)bse_p->bs_private;
	struct lck_interface *lck_p = &priv_p->p_lck;
	struct bsn_interface *bsn_p = &priv_p->p_bsn;
	struct bsn_node *np;
	struct rtree_setup rtree_setup;

	/*
	 * create a new master branch
	 */
	np = bsn_p->bn_op->bn_get_node(bsn_p);
	rtree_setup.allocator = priv_p->p_allocator;
	rtree_setup.extend_cb = _bse_rtree_extend_cb;
	rtree_setup.insert_cb = _bse_rtree_insert_cb;
	rtree_setup.remove_cb = _bse_rtree_remove_cb;
	rtree_initialization(&np->bn_rtree, &rtree_setup);
	INIT_LIST_HEAD(&np->bn_ddn_head);

	lck_p->l_op->l_wlock(lck_p, &priv_p->p_lnode);
	/* current master branch become a snapshot */
	list_add_tail(&priv_p->p_master->bn_list, &priv_p->p_snapshot_head);
	/* a new, empty master branch */
	priv_p->p_master = np;
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_lnode);
}

struct bse_operations bse_op = {
	.bs_traverse_start = bse_traverse_start,
	.bs_traverse_set = bse_traverse_set,
	.bs_traverse_end = bse_traverse_end,
	.bs_traverse_next = bse_traverse_next,
	.bs_traverse_next_async = bse_traverse_next_async,
	.bs_contiguous_low_bound = bse_contiguous_low_bound,
	.bs_contiguous_low_bound_async = bse_contiguous_low_bound_async,
	.bs_add_list = bse_add_list,
	.bs_del_node = bse_del_node,
	.bs_search = bse_search,
	.bs_close = bse_close,
	.bs_get_chan_stat = bse_get_chan_stat,
	.bs_create_snapshot = bse_create_snapshot,
};

int
bse_initialization(struct bse_interface *bse_p, struct bse_setup *setup)
{
	struct bse_private *priv_p;
	struct lck_setup lck_setup;
	struct rtree_setup rtree_setup;
	struct btn_setup btn_setup;
	mode_t mode;
	int flags;
	char *log_header = "bse_initialization";

	nid_log_info("bse_initialization start ...");
	assert(setup);
	bse_p->bs_op = &bse_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bse_p->bs_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_pp = setup->pp;
	priv_p->p_ddn = setup->ddn;
	priv_p->p_srn = setup->srn;
	priv_p->p_bfe = setup->bfe;
	priv_p->p_traverse_cross_page = 1;
	sem_init(&priv_p->p_ddn_sem, 0, 0);

	INIT_LIST_HEAD(&priv_p->p_ddn_head);
	INIT_LIST_HEAD(&priv_p->p_snapshot_head);
	lck_initialization(&priv_p->p_lck, &lck_setup);
	lck_node_init(&priv_p->p_lnode);

	btn_setup.allocator = priv_p->p_allocator;
	btn_setup.set_id = ALLOCATOR_SET_BSE_BTN;
	btn_setup.seg_size = 1024;
	btn_initialization(&priv_p->p_btn, &btn_setup);

	rtree_setup.extend_cb = _bse_rtree_extend_cb;
	rtree_setup.insert_cb = _bse_rtree_insert_cb;
	rtree_setup.remove_cb = _bse_rtree_remove_cb;
	rtree_setup.shrink_to_root_cb = __bse_rtree_shrink_to_root_cb;
	rtree_setup.allocator = priv_p->p_allocator;
	rtree_initialization(&priv_p->p_rtree, &rtree_setup);

	priv_p->p_devsz = setup->devsz;
	priv_p->p_cachesz = 1024*1024;
	if (setup->bufdevice) {
		priv_p->p_bufdevice = setup->bufdevice;
		mode = 0600;
		flags = O_RDONLY;
		priv_p->p_fhandle = open(priv_p->p_bufdevice, flags, mode);
		if (priv_p->p_fhandle < 0) {
			nid_log_error("%s: cannot open %s", log_header, priv_p->p_bufdevice);
			assert(0);
		}
	} else {
		priv_p->p_fhandle = -1;
	}

	return 0;
}
