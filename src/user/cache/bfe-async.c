/*
 * bfe.c
 * 	Implementation of BIO Flush Engine Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "nid_log.h"
#include "list.h"
#include "lck_if.h"
#include "allocator_if.h"
#include "pp_if.h"
#include "d2cn_if.h"
#include "d1an_if.h"
#include "ddn_if.h"
#include "io_if.h"
#include "rw_if.h"
#include "wc_if.h"
#include "rc_if.h"
#include "bse_if.h"
#include "bre_if.h"
#include "bfe_if.h"
#include "fpn_if.h"
#include "lstn_if.h"
#include "sac_if.h"

/* Retain enough memory for avoid use get_node_forcebly, which may cause memory leaking.*/
#define BFE_FLUSH_POOL_SIZE	16

struct bfe_private {
	struct allocator_interface	*p_allocator;
	struct wc_interface		*p_wc;
	void				*p_wc_chain_handle;
	struct rc_interface		*p_rc;
	void				*p_rc_handle;
	struct rw_interface		*p_rw;
	void				*p_rw_handle;
	struct lstn_interface		*p_lstn;
	struct ddn_interface		*p_ddn;
	struct bse_interface		*p_bse;
	struct bre_interface		*p_bre;
	struct pp_interface		*p_pp;
	char				*p_bufdevice;
	pthread_mutex_t			p_flush_lck;
	pthread_cond_t			p_flush_cond;
	pthread_cond_t			p_not_ready_cond;
	pthread_cond_t			p_overwritten_cond;
	pthread_cond_t			p_flush_page_cond;	// condition wait for p_flush_page_counter to be less than p_flush_page_nwait
	pthread_mutex_t			p_flush_finish_lck;
	pthread_cond_t			p_flush_post_cond;
	pthread_mutex_t			p_trim_lck;
	struct list_head		p_flush_page_head;
	struct list_head		p_not_ready_head;
	struct list_head		p_to_not_ready_head;
	struct list_head		p_flush_finished_head;
	struct list_head		p_flushing_head;	// list of pages which are flushing
	struct d2cn_interface		p_d2cn;
	struct d1an_interface		p_d1an;
	struct pp_interface 		p_pp_flush;
	uint64_t			p_seq_to_flush;
	uint64_t			p_last_seq_flushed;
	uint64_t			p_last_seq_flushing;
	uint64_t			p_seq_updated;
	uint64_t			p_total_vecnum;
	uint64_t			p_total_flushsize;
	uint64_t			p_overwritten_counter;
	uint64_t			p_overwritten_back_counter;
	uint64_t			p_flush_counter;
	uint64_t			p_flush_back_counter;
	uint64_t			p_coalesce_flush_counter;
	uint64_t			p_coalesce_flush_back_counter;
	uint64_t			p_coalesce_index;
	uint64_t			p_not_ready_counter;
	uint32_t			p_pagesz;
	uint32_t			p_flush_num;
	uint32_t			p_flush_page_counter;	// number of pages in p_flush_page_head
	int				p_vec_stat;
	int				p_write_max_retry_time;
	int				p_fhandle;
	uint32_t			p_flush_page_nwait;	// the value of p_flush_page_counter we want to wait for
	uint32_t			p_flush_pp_page_size;
	uint8_t				p_do_fp;
	uint8_t				p_do_coalescing;
	uint8_t				p_ssd_mode;
	uint8_t				p_check_not_ready;
	uint8_t				p_stop;
	uint8_t				p_flush_stop;
	uint8_t				p_post_flush_stop;
	uint8_t				p_triming;
};

static void
__rw_callback(int errorcode, struct rw_callback_arg *arg)
{
	struct d1a_node *fan;
	struct d2c_node *tfn;		// to flush node
	struct bfe_private *priv_p;
	struct rw_alignment_write_arg *awa;
	struct rc_interface *rc_p;
	void *rc_handle;
	struct rw_interface *rw_p;
	int do_fp;

	if (arg->ag_type == RW_CB_TYPE_ALIGNMENT) {
		awa = (struct rw_alignment_write_arg *) arg;
		fan = (struct d1a_node *)awa->arg_packaged;
		tfn = fan->d1_data;
		priv_p = (struct bfe_private *)tfn->d2_data[BFE_PRIV_INDEX];

		/*
		 * As the alignment buffer will be free after call this function,
		 * We have to do read cache update here.
		 */
		rc_p = priv_p->p_rc;
		rc_handle = priv_p->p_rc_handle;
		if (rc_p) {
			rc_p->rc_op->rc_update(rc_p, rc_handle, awa->arg_buffer_align,
				awa->arg_count_align, awa->arg_offset_align, &awa->arg_this.ag_fp_head);
		}
		fan->d1_flag = 1;

	} else {
		fan = (struct d1a_node *)arg;
		tfn = fan->d1_data;
		priv_p = (struct bfe_private *)tfn->d2_data[BFE_PRIV_INDEX];
		fan->d1_flag = 0;
	}
	fan->d1_ecode = errorcode;
	if (errorcode) {
		tfn = fan->d1_data;
		priv_p = (struct bfe_private *)tfn->d2_data[BFE_PRIV_INDEX];
		if (fan->d1_retry_time++ < priv_p->p_write_max_retry_time) {
			rw_p = priv_p->p_rw;
			do_fp = priv_p->p_do_fp;
			if (do_fp) {
		    		priv_p->p_flush_counter++;
				rw_p->rw_op->rw_pwritev_async_fp(rw_p, priv_p->p_rw_handle, fan->d1_iov,
					fan->d1_counter, fan->d1_offset, __rw_callback, &fan->d1_arg);
			} else {
				rw_p->rw_op->rw_pwritev_async(rw_p, priv_p->p_rw_handle, fan->d1_iov,
						fan->d1_counter, fan->d1_offset, __rw_callback, &fan->d1_arg);
			}
			return;
		} else {
			nid_log_error("Error: Write failed. Error code: %d. ", errorcode);
		}
	}

	pthread_mutex_lock(&priv_p->p_flush_finish_lck);
	list_add_tail(&fan->d1_list, &priv_p->p_flush_finished_head);
	pthread_cond_broadcast(&priv_p->p_flush_post_cond);
	pthread_mutex_unlock(&priv_p->p_flush_finish_lck);
}

inline static void
__update_seq_flushed(struct bfe_private *priv_p, uint64_t seq)
{
	struct bre_interface *bre_p = priv_p->p_bre;
	if (priv_p->p_ssd_mode) {
		bre_p->br_op->br_update_sequence(bre_p, seq);
	} else {
		struct wc_interface *wc_p = priv_p->p_wc;
		wc_p->wc_op->wc_flush_update(wc_p, priv_p->p_wc_chain_handle, seq);
	}
	priv_p->p_seq_updated = seq;
}

/*
 * Note:
 * 	The caller should hold p_flush_lck
 */
static void
__extend_write_request_async(struct bfe_private *priv_p, struct data_description_node *dnp, struct d2c_node *tfn, char is_update_fl_cnt)
{
	char *log_header = "__extend_write_request_async";
	struct bse_interface *bse_p = priv_p->p_bse;
	struct rw_interface *rw_p = priv_p->p_rw;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct d1an_interface *d1an_p = &priv_p->p_d1an;
	struct pp_interface *pp_flush = &priv_p->p_pp_flush;
	int do_fp = priv_p->p_do_fp;
	struct data_description_node *low_dnp, *cur_dnp, *pre_dnp = NULL;
	int got_me = 0, i, pp_page_len, write_size;
	struct d1a_node *fan;
	char *data_buf;
	uint8_t setflushing = 0;
	uint32_t data_buf_len;
	struct pp_page_node* pp_flush_nd[BFE_MAX_IOV];

	write_size = 0;

	pp_page_len = 0;
	pp_flush_nd[pp_page_len] = NULL;

	assert(rw_p);
	nid_log_debug("%s: start (bse_p:%p, dnp:%p)...", log_header, bse_p, dnp);
	if (!priv_p->p_do_coalescing || tfn->d2_partial_flush) {
   	 	pthread_mutex_lock(&priv_p->p_flush_lck);
		if (!(fan = (struct d1a_node *)dnp->d_callback_arg)) {
			fan = d1an_p->d1_op->d1_get_node(d1an_p);
			fan->d1_data = (void *)tfn;
			fan->d1_counter = 0;
			fan->d1_arg.ag_type = RW_CB_TYPE_NORMAL;
			INIT_LIST_HEAD(&fan->d1_arg.ag_coalesce_head);
			INIT_LIST_HEAD(&fan->d1_arg.ag_overwritten_head);
			fan->d1_arg.ag_request = dnp;
			dnp->d_callback_arg = fan;
		} else {
			/*
			 * Already been set by __bfe_overwritten_wait
			 */
			fan->d1_data = (void *)tfn;
		}
		dnp->d_flushed = 1;
		if (priv_p->p_ssd_mode || dnp->d_location_disk) {
			if (pp_flush_nd[0] == NULL) {
				assert(pp_page_len == 0);
				pp_flush_nd[pp_page_len] = pp_flush->pp_op->pp_get_node(pp_flush);
				data_buf = (char *)pp_flush_nd[pp_page_len++]->pp_page;
				data_buf_len = 0;
			}
			pread(priv_p->p_fhandle, data_buf, dnp->d_flush_len, dnp->d_flush_pos);
			fan->d1_iov[fan->d1_counter].iov_base = data_buf;
		} else {
			fan->d1_iov[fan->d1_counter].iov_base = (char *)dnp->d_flush_pos;
		}

		fan->d1_iov[fan->d1_counter++].iov_len = dnp->d_flush_len;
		write_size += dnp->d_flush_len;
		low_dnp = dnp;
		if (is_update_fl_cnt) tfn->d2_fl_counter++;
    		pthread_mutex_unlock(&priv_p->p_flush_lck);
		goto write_out;
	}

	bse_p->bs_op->bs_traverse_start(bse_p, NULL);	// just hold lock
    	pthread_mutex_lock(&priv_p->p_flush_lck);
	if (!(fan = (struct d1a_node *)dnp->d_callback_arg)) {
		fan = d1an_p->d1_op->d1_get_node(d1an_p);
		fan->d1_data = (void *)tfn;
		fan->d1_counter = 0;
		fan->d1_arg.ag_type = RW_CB_TYPE_NORMAL;
		INIT_LIST_HEAD(&fan->d1_arg.ag_coalesce_head);
		INIT_LIST_HEAD(&fan->d1_arg.ag_overwritten_head);
		fan->d1_arg.ag_request = dnp;
		dnp->d_callback_arg = fan;
	} else {
		/*
		 * Already been set by __bfe_overwritten_wait
		 */
		fan->d1_data = (void *)tfn;
	}

	pthread_cond_signal(&priv_p->p_overwritten_cond);
	low_dnp = bse_p->bs_op->bs_contiguous_low_bound_async(bse_p, dnp, BFE_MAX_IOV - 1);
	assert(!low_dnp->d_flushed && !low_dnp->d_flushed_back && !low_dnp->d_flush_done);
	bse_p->bs_op->bs_traverse_set(bse_p, low_dnp);

	while (fan->d1_counter < BFE_MAX_IOV && (cur_dnp = bse_p->bs_op->bs_traverse_next_async(bse_p))) {
		//nid_log_debug("%s: pre_dnp:%p, cur_dnp:%p, cur_flushing:%d, cur_offset:%ld, vec_counter:%d",
		//	log_header, pre_dnp, cur_dnp, cur_dnp->d_flushing, cur_dnp->d_offset, vec_counter);

		if (!got_me)
			assert(!cur_dnp->d_flushed && !cur_dnp->d_flushed_back && !cur_dnp->d_flush_done);

		if (cur_dnp == dnp)
			got_me = 1;

		if (cur_dnp->d_flushed) {
			assert(got_me);
			break;
		}

		assert(!cur_dnp->d_flushed_back && !cur_dnp->d_flush_done);
		if (cur_dnp->d_flushing) {
			if (pre_dnp && (pre_dnp->d_flush_offset + pre_dnp->d_flush_len != cur_dnp->d_flush_offset))
				break;
		} else {
			if (pre_dnp && (pre_dnp->d_flush_offset + pre_dnp->d_flush_len != cur_dnp->d_offset))
				break;
			setflushing = 1;
			cur_dnp->d_flushing = 1;
			cur_dnp->d_flush_offset = cur_dnp->d_offset;
			cur_dnp->d_flush_len = cur_dnp->d_len;
			cur_dnp->d_flush_pos = cur_dnp->d_pos;
		}

		if (priv_p->p_ssd_mode || cur_dnp->d_location_disk) {
			assert(priv_p->p_flush_pp_page_size >= cur_dnp->d_flush_len);
			if (pp_flush_nd[0] == NULL) {
				assert(pp_page_len == 0);
				pp_flush_nd[pp_page_len] = pp_flush->pp_op->pp_get_node(pp_flush);
				data_buf = (char *)pp_flush_nd[pp_page_len++]->pp_page;
				data_buf_len = 0;
			} else if (data_buf_len + cur_dnp->d_flush_len > priv_p->p_flush_pp_page_size) {
				if (pp_page_len >=  BFE_FLUSH_POOL_SIZE) {
					// Out of hole pp size, we need make decision if stop coalesce or not
					if (got_me == 1 && cur_dnp != dnp) {
						if (setflushing) {
							cur_dnp->d_flushing = 0;
						}
						// Found me, coalesce too much, so get out.
						break;
					} else {
						// Not found me, we have to continue do the coalesce
						pp_flush_nd[pp_page_len] = pp_flush->pp_op->pp_get_node_forcibly(pp_flush);
						nid_log_warning("__extend_write_request: use pp_get_node_forcibly: pp_page_len:%d write_total_len:%u",
								pp_page_len, write_size);
					}
				} else {
					pp_flush_nd[pp_page_len] = pp_flush->pp_op->pp_get_node(pp_flush);
				}
				data_buf = pp_flush_nd[pp_page_len++]->pp_page;
				data_buf_len = 0;
			}
			pread(priv_p->p_fhandle, data_buf, cur_dnp->d_flush_len, cur_dnp->d_pos);
			fan->d1_iov[fan->d1_counter].iov_base = data_buf;
			data_buf += cur_dnp->d_flush_len;
			data_buf_len += cur_dnp->d_flush_len;
			assert(data_buf_len <= priv_p->p_flush_pp_page_size);
			setflushing = 0;
		} else {
			fan->d1_iov[fan->d1_counter].iov_base = (char *)cur_dnp->d_flush_pos;
		}
		fan->d1_iov[fan->d1_counter++].iov_len = cur_dnp->d_flush_len;

		if (cur_dnp != dnp) {
			struct list_node *lnp;

			// update bfe coalesce index
			__sync_add_and_fetch(&priv_p->p_coalesce_index, 1);
			__sync_add_and_fetch(&cur_dnp->d_page_node->pp_coalesced, 1);
			lnp = lstn_p->ln_op->ln_get_node(lstn_p);
			lnp->ln_data = (void *)cur_dnp;
			list_add_tail(&lnp->ln_list, &fan->d1_arg.ag_coalesce_head);
			priv_p->p_coalesce_flush_counter++;
		}

		cur_dnp->d_flushed = 1;
		pre_dnp = cur_dnp;
		write_size += cur_dnp->d_flush_len;
	}
	if (is_update_fl_cnt) tfn->d2_fl_counter++;
    	pthread_mutex_unlock(&priv_p->p_flush_lck);
	bse_p->bs_op->bs_traverse_end(bse_p);

write_out:
	/*
	 * Now, we are doing buffer write without holding lock (p_flush_lck)
	 */
	if (priv_p->p_vec_stat == 1) {
		priv_p->p_total_flushsize += write_size;
		priv_p->p_total_vecnum += fan->d1_counter - 1;
		priv_p->p_flush_num += 1;
	}

	fan->d1_offset = low_dnp->d_flush_offset;
	fan->d1_retry_time = 0;
	priv_p->p_flush_counter++;
	if (do_fp) {
		rw_p->rw_op->rw_pwritev_async_fp(rw_p, priv_p->p_rw_handle, fan->d1_iov,
			fan->d1_counter, fan->d1_offset, __rw_callback, &fan->d1_arg);
	} else {
		INIT_LIST_HEAD(&fan->d1_arg.ag_fp_head);
		rw_p->rw_op->rw_pwritev_async(rw_p, priv_p->p_rw_handle, fan->d1_iov,
			fan->d1_counter, fan->d1_offset, __rw_callback, &fan->d1_arg);
	}

	/*Free temporary malloced memory.*/
	for (i=0; i< pp_page_len; i++)
		pp_flush->pp_op->pp_free_node(pp_flush, pp_flush_nd[i]);

}

/*
 * Algorithm:
 * 	The new_np trying to overwrite an old np, but the np has been in flushing status, not done yet.
 * 	So the new_np has to wait for np flushed back before the new_np can be flushed
 *
 * Note:
 * 	The caller should hold p_flush_lck
 */
static void
__bfe_overwritten_wait(struct bfe_private *priv_p, struct data_description_node *np,
	struct data_description_node *new_np)
{
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct d1an_interface *d1an_p = &priv_p->p_d1an;
	struct list_node *lnp;
	struct d1a_node *fan;
	struct pp_page_node *pnp;
	struct d2c_node *tfn;

	/*
	 * There may not d_callback_arg associated with this np since it could be
	 * coalesced by another ddn
	 */
	if (!(fan = (struct d1a_node *)np->d_callback_arg)) {
		fan = d1an_p->d1_op->d1_get_node(d1an_p);
		fan->d1_data = np->d_page_node->pp_tfn;
		fan->d1_counter = 0;
		fan->d1_arg.ag_type = RW_CB_TYPE_NORMAL;
		INIT_LIST_HEAD(&fan->d1_arg.ag_coalesce_head);
		INIT_LIST_HEAD(&fan->d1_arg.ag_overwritten_head);
		fan->d1_arg.ag_request = (void *)np;
		np->d_callback_arg = (void *)fan;
	}

	/*
	 * the new_np should wait for this np flushed back before it can be flushed
	 */
	__sync_add_and_fetch(&new_np->d_flush_overwritten, 1);
	pnp = new_np->d_page_node;
	tfn = (struct d2c_node *)pnp->pp_tfn;
	tfn->d2_ow_counter++;
	priv_p->p_overwritten_counter++;
	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)new_np;
	list_add_tail(&lnp->ln_list, &fan->d1_arg.ag_overwritten_head);
}

static inline int
__bfe_pre_finish_flush(struct bfe_private *priv_p, struct pp_page_node *pnp)
{
	struct d2c_node *tfn = (struct d2c_node *)pnp->pp_tfn;
	return (!tfn->d2_acounter &&
		!tfn->d2_nr_counter &&
		!tfn->d2_ow_counter &&
		pnp->pp_occupied == priv_p->p_pagesz &&
		tfn->d2_partial_flush == 0 &&
		tfn->d2_fl_counter == 0);
}

/*
 * Note:
 * 	Must be procected by p_flush_lck
 */
static void
__bfe_finish_flush(struct bfe_private *priv_p, struct pp_page_node *pnp)
{
	char *log_header = "__bfe_finish_flush";
	struct d2cn_interface *d2cn_p = &priv_p->p_d2cn;
	struct d2c_node *tfn = (struct d2c_node *)pnp->pp_tfn;
	uint32_t page_sz = priv_p->p_pagesz;
	struct bre_interface *bre_p = priv_p->p_bre;

	nid_log_debug("%s: start (pnp:%p) ...", log_header, pnp);
	assert(!tfn->d2_acounter && !tfn->d2_nr_counter && !tfn->d2_ow_counter);
	assert(!pnp->pp_coalesced);
	assert(pnp->pp_where == PP_WHERE_FLUSHING);
	assert(tfn->d2_fl_counter == 0);

	pnp->pp_where = PP_WHERE_FLUSHING_DONE;
	if (pnp->pp_rlist.prev != &priv_p->p_flushing_head)
		return;

release_next:
	/*
	 * All pages before pnp have already been flush_done and released
	 * this pnp has also been flush_done so it's time to release it
	 */
	list_del(&pnp->pp_rlist);	// removed from p_flushing_head

	if (tfn->d2_seq) {
		priv_p->p_last_seq_flushed = tfn->d2_seq;
	} else {
		/**
		 * When tfn->d2_seq == 0, means the system is very busy,
		 * and the agent reconnect at new page used, so end_page called,
		 * which cause this new page release at once.
		 * So at this point, the flush_head and release_head should empty.
		 */
		assert(list_empty(&pnp->pp_flush_head) && list_empty(&pnp->pp_release_head));
	}
	/*
	 * Release the page pnp to bre if the whole page got flushed
	 * Notice: the last page in the page list (p_flush_page_head) may not be fully occupied,
	 * i.e pp_occupied < page_sz, don't release this page since it is still being used by ds
	 */
	assert(pnp->pp_flushed == page_sz && pnp->pp_occupied == page_sz);
	list_splice_tail_init(&pnp->pp_flush_head, &pnp->pp_release_head);
	priv_p->p_flush_page_counter--;
	nid_log_debug("%s: release a page (%p) ...", log_header, pnp);

	d2cn_p->d2_op->d2_put_node(d2cn_p, tfn);
	pnp->pp_tfn = NULL;
	if (priv_p->p_ssd_mode) {
		bre_p->br_op->br_release_page_sequence(bre_p, pnp);
	} else {
		bre_p->br_op->br_release_page_async(bre_p, pnp);
	}

	if (!list_empty(&priv_p->p_flushing_head)) {
		pnp = list_first_entry(&priv_p->p_flushing_head, struct pp_page_node, pp_rlist);
		if (pnp->pp_where == PP_WHERE_FLUSHING_DONE) {
			tfn = (struct d2c_node *)pnp->pp_tfn;
			goto release_next;
		}
	}
	pthread_cond_broadcast(&priv_p->p_flush_cond);
}

// apply trim command on target device
static void
bfe_trim_list(struct bfe_interface *bfe_p, struct list_head *trim_head)
{
	char *log_header = "bfe_trim_list";
	struct bfe_private *priv_p = bfe_p->bf_private;
	struct rw_interface *rw_p = priv_p->p_rw;
	struct list_head to_trim_head;
	struct request_node *rn_p, *rn_p2;

	nid_log_warning("%s: start ...", log_header);
	INIT_LIST_HEAD(&to_trim_head);

check_triming:
	// make sure previouse trim call not in progress
	for(;;) {
		pthread_mutex_lock(&priv_p->p_trim_lck);
		if(priv_p->p_triming) {
			pthread_mutex_unlock(&priv_p->p_trim_lck);
			goto check_triming;
		}
		priv_p->p_triming = 1;
		list_splice_tail_init(trim_head, &to_trim_head);
		pthread_mutex_unlock(&priv_p->p_trim_lck);
		break;
	}
	// prepare trim request data
	list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &to_trim_head, r_trim_list) {
		list_del(&rn_p->r_trim_list);

		if(rw_p->rw_op->rw_trim(rw_p, priv_p->p_rw_handle, rn_p->r_offset, rn_p->r_len)) {
			nid_log_error("%s: trim failed on %s with offset: %lu len: %u",
					log_header, rw_p->rw_op->rw_get_exportname(rw_p), rn_p->r_offset, rn_p->r_len);
		}
	}
	pthread_mutex_lock(&priv_p->p_trim_lck);
	priv_p->p_triming = 0;
	pthread_mutex_unlock(&priv_p->p_trim_lck);

	nid_log_warning("%s: done ...", log_header);
}

/*
 * Algorithm:
 * 	Add a page (fn_pnp) to flush
 */
static void
bfe_flush_page(struct bfe_interface *bfe_p, struct pp_page_node *pnp)
{
	char *log_header = "bfe_flush_page";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct d2cn_interface *d2cn_p = &priv_p->p_d2cn;
	struct d2c_node *tfn;

	nid_log_debug("%s: adding page %p", log_header, pnp);
	pthread_mutex_lock(&priv_p->p_flush_lck);

	tfn = d2cn_p->d2_op->d2_get_node(d2cn_p);
	pnp->pp_tfn = (void *)tfn;
	tfn->d2_data[BFE_PRIV_INDEX] = (void *)priv_p;
	tfn->d2_data[BFE_PAGE_INDEX] = (void *)pnp;
	tfn->d2_seq = 0;
	INIT_LIST_HEAD(&tfn->d2_head);
	tfn->d2_acounter = 1;
	tfn->d2_nr_counter = 0;
	tfn->d2_ow_counter = 0;
	tfn->d2_fl_counter = 0;
	tfn->d2_partial_flush = 1;

 	list_add_tail(&pnp->pp_rlist, &priv_p->p_flush_page_head);
	priv_p->p_flush_page_counter++;
	assert(pnp->pp_where != PP_WHERE_FLUSH);
	pnp->pp_where = PP_WHERE_FLUSH;
	pthread_cond_broadcast(&priv_p->p_flush_cond);
	pthread_mutex_unlock(&priv_p->p_flush_lck);
}

/*
 * Algorithm:
 * 	Don't extend if already in flushing status
 * Input:
 * 	new_np->d_where must be DDN_WHERE_NO
 */
static int
bfe_right_extend_node(struct bfe_interface *bfe_p, struct data_description_node *np,
	struct data_description_node *new_np)
{
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	int rc = 1;

	assert(new_np->d_where == DDN_WHERE_NO);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	if (!np->d_flushing) {
		np->d_len += new_np->d_len;
		if (new_np->d_flush_overwritten) {
			/*
			 * np should wait for new_np
			 */
			__bfe_overwritten_wait(priv_p, new_np, np);
			new_np->d_where = DDN_WHERE_SHADOW;
		}
		rc = 0;
	}
	pthread_mutex_unlock(&priv_p->p_flush_lck);
	return rc;
}

/*
 * Algorithm:
 * 	Add a new node
 *
 * Input:
 * 	new_np->d_where must be DDN_WHERE_NO
 *
 * Notice:
 * 	The caller should guarantee that this node would not be "freed" before
 * 	the end of this call
 *	The caller should make sure that the page is not in p_page_head yet
 */
static void
bfe_add_node(struct bfe_interface *bfe_p, struct data_description_node *new_np)
{
	char *log_header = "bfe_add_node";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	uint32_t page_sz = priv_p->p_pagesz;
	struct pp_page_node *pnp = new_np->d_page_node;
	struct d2c_node *tfn = (struct d2c_node *)pnp->pp_tfn;
	struct data_description_node *pre_np;

	nid_log_debug("%s: start (new_np:%p) ...", log_header, new_np);
	assert(new_np->d_where == DDN_WHERE_NO);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	list_add_tail(&new_np->d_flist, &pnp->pp_flush_head);
	assert(new_np->d_seq!=0);
	tfn->d2_seq = new_np->d_seq;
	__sync_add_and_fetch(&tfn->d2_acounter, 1);
	nid_log_debug("%s: add acounter %p", log_header, new_np);
	new_np->d_where = DDN_WHERE_FLUSH;
	if (new_np->d_flist.prev != &pnp->pp_flush_head) {
		pre_np = list_entry(new_np->d_flist.prev, struct data_description_node, d_flist);
		assert(pre_np->d_seq <= new_np->d_seq);
	}
	assert(pnp->pp_flushed + new_np->d_len <= page_sz);
	pthread_mutex_unlock(&priv_p->p_flush_lck);
}

/*
 * Algorithm:
 * 	Cut the left part of an existing node which has not been flushed yet
 *
 * Input:
 * 	new_np->d_where must be DDN_WHERE_NO
 *
 * Notice:
 * 	The caller should guarantee that this node would not be "freed" before
 * 	the end of this call
 */
static void
bfe_left_cut_node(struct bfe_interface *bfe_p, struct data_description_node *np,
	uint32_t cut_len, struct data_description_node *new_np)
{
	char *log_header = "bfe_left_cut_node";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	uint32_t page_sz = priv_p->p_pagesz;
	struct pp_page_node *pnp = np->d_page_node;

	nid_log_debug("%s: start (np:%p) ...", log_header, np);
	assert(new_np->d_where == DDN_WHERE_NO);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	np->d_pos += cut_len;
	np->d_offset += cut_len;
	np->d_len -= cut_len;
	assert(np->d_len > 0);

	if (!np->d_flush_done) {
		pnp->pp_flushed += cut_len;
		assert(pnp->pp_flushed < page_sz);
	}

	if (np->d_flushing && !np->d_flushed_back) {
		/*
		 * Scenario 1>
		 * 	the np is in flushing status, not flushed back yet,
		 * 	the new_np should be recursivly waiting on np
		 */
		__bfe_overwritten_wait(priv_p, np, new_np);
		nid_log_debug ("%s: call_overwritten  new_np: %p wait np: %p", log_header, new_np, np);
	} else if (np->d_flush_overwritten) {
		/*
		 * Scenario 2>
		 * 	the np is in overwritten waiting status
		 * 	the new_np should be recursivly waiting on np
		 */
		assert(!np->d_flushed || np->d_flush_done);
		__bfe_overwritten_wait(priv_p, np, new_np);
		nid_log_debug ("%s: call_overwritten  new_np: %p wait np: %p", log_header, new_np, np);
	}
	pthread_mutex_unlock(&priv_p->p_flush_lck);
}

/*
 * Algorithm:
 *
 * Input:
 * 	new_np->d_where must be DDN_WHERE_NO
 *
 * Notice:
 * 	The caller should guarantee that this node would not be released before
 * 	the end of this call
 */
static void
bfe_right_cut_node(struct bfe_interface *bfe_p, struct data_description_node *np,
	uint32_t cut_len, struct data_description_node *new_np)
{
	char *log_header = "bfe_right_cut_node";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	uint32_t page_sz = priv_p->p_pagesz;
	struct pp_page_node *pnp = np->d_page_node;

	nid_log_debug("%s: start (np:%p) ...", log_header, np);
	assert(new_np->d_where == DDN_WHERE_NO);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	np->d_len -= cut_len;
	assert(np->d_len > 0);
	if (!np->d_flush_done) {
		pnp->pp_flushed += cut_len;
		assert(pnp->pp_flushed < page_sz);
	}

	if (np->d_flushing && !np->d_flushed_back) {
		/*
		 * Scenario 1>
		 * 	the np is in flushing status, not flushed back yet,
		 * 	the new_np should be recursivly waiting on np
		 */
		__bfe_overwritten_wait(priv_p, np, new_np);
		nid_log_debug("%s: call_overwritten  new_np: %p wait np: %p", log_header, new_np, np);
	} else if (np->d_flush_overwritten) {
		/*
		 * Scenario 2>
		 * 	the np is in overwritten waiting status
		 * 	the new_np should be recursivly waiting on np
		 */
		assert(!np->d_flushed || np->d_flush_done);
		__bfe_overwritten_wait(priv_p, np, new_np);
		nid_log_debug("%s: call_overwritten  new_np: %p wait np: %p", log_header, new_np, np);
	}
	pthread_mutex_unlock(&priv_p->p_flush_lck);
}

/*
 * Algorithm:
 *
 * Input:
 * 	new_np->d_where must be DDN_WHERE_NO
 *
 * Notice:
 * 	The caller should guarantee that this node would not be released before
 * 	the end of this call
 */
static int
bfe_right_cut_node_adjust(struct bfe_interface *bfe_p, struct data_description_node *np,
	uint32_t cut_len, struct data_description_node *a_np, struct data_description_node *new_np)
{
	char *log_header = "bfe_right_cut_node_adjust";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	uint32_t page_sz = priv_p->p_pagesz;
	struct pp_page_node *pnp = np->d_page_node;
	uint32_t adjust_len = a_np->d_len;
	int rc = 0;

	nid_log_debug("%s: start (np:%p, a_np:%p) ...", log_header, np, a_np);
	assert(new_np->d_where == DDN_WHERE_NO);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	np->d_len -= cut_len;
	assert(np->d_len > 0);
	assert(a_np->d_seq == np->d_seq);
	if (!np->d_flushing) {
		assert(!np->d_flushed && !np->d_flush_done);
		a_np->d_flushing = 0;
		a_np->d_flushed = 0;
		a_np->d_flushed_back = 0;
		a_np->d_flush_done = 0;
		pnp->pp_flushed += cut_len - adjust_len;
		assert(pnp->pp_flushed < page_sz);

		/*
		 * insert the a_np to the flush list. It will be flushed later
		 */
		__list_add(&a_np->d_flist, &np->d_flist, np->d_flist.next);
		a_np->d_where = DDN_WHERE_FLUSH;
		struct d2c_node *tfn = (struct d2c_node *)pnp->pp_tfn;
		__sync_add_and_fetch(&tfn->d2_acounter, 1);
		nid_log_debug("%s: add acounter %p", log_header, np);

	} else {
		a_np->d_flushing = 1;
		a_np->d_flushed = 1;	// don't flush it
		a_np->d_flushed_back = 1;
		a_np->d_flush_done = 1;	// don't count it in pp_flushed
		if (!np->d_flush_done) {
			/*
			 * the adjust np (a_np) won't be flushed. the np will got be flushed w/o cut.
			 * But it's len already got reduced, its reduced part won't be added to pp_flushed.
			 * So add it here.
			 */
			pnp->pp_flushed += cut_len;
			assert(pnp->pp_flushed < page_sz);
		}

		/*
		 * Insert the a_np in the release list instead of flush list, since it shouldn't
		 * be flushed later.
		 */
		list_add_tail(&a_np->d_flist, &pnp->pp_release_head);
		a_np->d_where = DDN_WHERE_RELEASE;
	}

	if (np->d_flushing && !np->d_flushed_back) {
		/*
		 * Scenario 1>
		 * 	the np is in flushing status, not flushed back yet,
		 * 	both new_np and a_np should be waiting on np even a_np won't be flushed since
		 * 	we want to make sure all later new nodes which are overwritten with a_np should
		 * 	be waiting on np
		 */
		__bfe_overwritten_wait(priv_p, np, new_np);
		__bfe_overwritten_wait(priv_p, np, a_np);
		nid_log_debug ("%s: call_overwritten  new_np: %p wait np: %p", log_header, new_np, np);
	} else if (np->d_flush_overwritten) {
		/*
		 * Scenario 2>
		 * 	the np is in overwritten waiting status
		 * 	both new_np and a_np should be waiting on np even a_np won't be flushed since
		 * 	we want to make sure all later new nodes which are overwritten with a_np should
		 * 	be waiting on np
		 */
		assert(!np->d_flushed || np->d_flush_done);
		__bfe_overwritten_wait(priv_p, np, new_np);
		__bfe_overwritten_wait(priv_p, np, a_np);
		nid_log_debug ("%s: call_overwritten  new_np: %p wait np: %p", log_header, new_np, np);
	}
	pthread_mutex_unlock(&priv_p->p_flush_lck);
	return rc;
}

/*
 * Algorithm:
 * 	If node still in the flush list and not flushing yet, remove it from the flush list
 *
 * Input:
 * 	new_np->d_where must be DDN_WHERE_NO
 *
 * Notice:
 * 	The caller should guarantee that this node would not be "freed" before
 * 	the end of this call
 */
static void
bfe_del_node(struct bfe_interface *bfe_p, struct data_description_node *np,
	struct data_description_node *new_np)
{
	char *log_header = "bfe_del_node";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	uint32_t page_sz = priv_p->p_pagesz;
	struct pp_page_node *pnp = np->d_page_node;

	nid_log_debug("%s: start (np:%p) ...", log_header, np);
	assert(new_np->d_where == DDN_WHERE_NO);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	if (!np->d_flushing) {
		assert(!np->d_flushed && !np->d_flush_done);
		assert(np->d_where == DDN_WHERE_FLUSH);
		pnp->pp_flushed += np->d_len;	// this portion will never be added to pp_flush later, so do it now.
		assert(pnp->pp_flushed <= page_sz);
		np->d_flushing = 1;
		np->d_flushed = 1;
		np->d_flushed_back = 1;
		np->d_flush_done = 1;
		list_del(&np->d_flist);		// removed from pnp->pp_flush_head
		list_add_tail(&np->d_flist, &pnp->pp_release_head);
		np->d_where = DDN_WHERE_RELEASE;

		struct d2c_node *tfn = (struct d2c_node *)pnp->pp_tfn;

		__sync_sub_and_fetch(&tfn->d2_acounter, 1);
		if (__bfe_pre_finish_flush(priv_p, pnp)) {
			__bfe_finish_flush(priv_p, pnp);
		}
	}

	if (np->d_flushing && !np->d_flushed_back) {
		/*
		 * Scenario 1>
		 * 	the np is in flushing status, not flushed back yet,
		 * 	the new_np should be recursivly waiting on np
		 */
		__bfe_overwritten_wait(priv_p, np, new_np);
		nid_log_debug ("%s: call_overwritten  new_np: %p wait np: %p", log_header, new_np, np);
	} else if (np->d_flush_overwritten) {
		/*
		 * Scenario 2>
		 * 	the np is in overwritten waiting status
		 * 	the new_np should be recursivly waiting on np
		 */
		assert(!np->d_flushed || np->d_flush_done);
		__bfe_overwritten_wait(priv_p, np, new_np);
		nid_log_debug ("%s: call_overwritten  new_np: %p wait np: %p", log_header, new_np, np);
	}
	pthread_mutex_unlock(&priv_p->p_flush_lck);
}

static void
bfe_flush_seq(struct bfe_interface *bfe_p, uint64_t seq)
{
	char *log_header = "bfe_flush_seq";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	nid_log_debug("%s: start (seq:%lu)", log_header, seq);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	if (seq > priv_p->p_seq_to_flush) {
		priv_p->p_seq_to_flush = seq;
		pthread_cond_broadcast(&priv_p->p_flush_cond);
	}
	pthread_mutex_unlock(&priv_p->p_flush_lck);
}

static void
bfe_close(struct bfe_interface *bfe_p)
{
	char *log_header = "bfe_close";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct d1an_interface *d1an_p = &priv_p->p_d1an;
	struct d2cn_interface *d2cn_p = &priv_p->p_d2cn;
	struct pp_interface *pp_flush_p = &priv_p->p_pp_flush;

	nid_log_notice("%s: start ...", log_header);
	priv_p->p_stop = 1;
	pthread_cond_broadcast(&priv_p->p_flush_cond);
	pthread_cond_broadcast(&priv_p->p_flush_post_cond);

	while (!priv_p->p_flush_stop || !priv_p->p_post_flush_stop)
		sleep(1);

	d1an_p->d1_op->d1_destroy(d1an_p);
	d2cn_p->d2_op->d2_destroy(d2cn_p);
	pthread_mutex_destroy(&priv_p->p_flush_lck);
	pthread_mutex_destroy(&priv_p->p_flush_finish_lck);
	pthread_mutex_destroy(&priv_p->p_trim_lck);
	pthread_cond_destroy(&priv_p->p_flush_cond);
	pthread_cond_destroy(&priv_p->p_overwritten_cond);
	pthread_cond_destroy(&priv_p->p_flush_post_cond);
	pthread_cond_destroy(&priv_p->p_flush_page_cond);
	close(priv_p->p_fhandle);
	priv_p->p_fhandle = -1;
	pp_flush_p->pp_op->pp_destroy(pp_flush_p);
	free(priv_p);
	bfe_p->bf_private = NULL;
}

static void
bfe_end_page(struct bfe_interface *bfe_p, struct pp_page_node *pnp)
{
	char *log_header = "bfe_end_page";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;

	assert(pnp->pp_occupied <= priv_p->p_pagesz);
	if (pnp->pp_occupied < priv_p->p_pagesz) {
		pthread_mutex_lock(&priv_p->p_flush_lck);
		if (pnp->pp_occupied < priv_p->p_pagesz) {
			nid_log_notice("%s: fill the page", log_header);
			pnp->pp_flushed += priv_p->p_pagesz - pnp->pp_occupied;
			pnp->pp_occupied = priv_p->p_pagesz;
		}
		pthread_mutex_unlock(&priv_p->p_flush_lck);
	}

	if (priv_p->p_ssd_mode) {
		assert(pnp->pp_occupied == priv_p->p_pagesz);
		priv_p->p_pp->pp_op->pp_free_data(priv_p->p_pp, pnp);
	}
}

static void
bfe_vec_start(struct bfe_interface *bfe_p)
{
	char *log_header = "bfe_vec_start";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	priv_p->p_vec_stat = 1;
	priv_p->p_flush_num = 0;
	priv_p->p_total_vecnum = 0;
	priv_p->p_total_flushsize = 0;
	nid_log_debug("%s", log_header);
}

static void
bfe_vec_stop( struct bfe_interface *bfe_p)
{
	char *log_header = "bfe_vec_stop";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	priv_p->p_vec_stat = 0;
	nid_log_debug("%s", log_header);
}

static void
bfe_get_stat(struct bfe_interface *bfe_p, struct io_vec_stat *stat)
{
	char *log_header = "bfe_get_stat";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	nid_log_debug("%s: start ...", log_header);
	stat->s_stat = priv_p->p_vec_stat;
	stat->s_flush_num = priv_p->p_flush_num;
	stat->s_vec_num = priv_p->p_total_vecnum;
	stat->s_write_size = priv_p->p_total_flushsize;
}

static void
bfe_get_rw_stat(struct bfe_interface *bfe_p, struct bfe_rw_stat *sp)
{
	char *log_header = "bfe_get_rw_stat";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct pp_page_node *pnp;
	struct d2c_node *tfn;

	sp->flush_page_num = priv_p->p_flush_page_counter;
	sp->overwritten_num = priv_p->p_overwritten_counter;
	sp->overwritten_back_num = priv_p->p_overwritten_back_counter;
	sp->coalesce_flush_num = priv_p->p_coalesce_flush_counter;
	sp->coalesce_flush_back_num = priv_p->p_coalesce_flush_back_counter;
	sp->flush_num = priv_p->p_flush_counter;
	sp->flush_back_num = priv_p->p_flush_back_counter;
	sp->not_ready_num = priv_p->p_not_ready_counter;

	nid_log_debug("%s: detail start ...", log_header);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	list_for_each_entry(pnp, struct pp_page_node, &priv_p->p_flushing_head, pp_rlist) {
		tfn = (struct d2c_node *)pnp->pp_tfn;
		nid_log_debug("%s: pnp(%p), d2_acounter:%d, d2_nr_counter:%d, where:%d",
			log_header, pnp, tfn->d2_acounter, tfn->d2_nr_counter, pnp->pp_where);
	}
	nid_log_debug("%s: detail end ...", log_header);
	pthread_mutex_unlock(&priv_p->p_flush_lck);
}

static uint64_t
bfe_get_coalesce_index(struct bfe_interface *bfe_p) {
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;

	return priv_p->p_coalesce_index;
}

static void
bfe_reset_coalesce_index(struct bfe_interface *bfe_p) {
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;

	priv_p->p_coalesce_index = 0;
}

static uint32_t
bfe_get_page_counter(struct bfe_interface *bfe_p)
{
	char *log_header = "bfe_get_page_counter";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;

	nid_log_debug("%s: start ...", log_header);
	return (uint32_t)priv_p->p_flush_page_counter;
}

static struct bfe_operations bfe_op = {
	.bf_flush_page = bfe_flush_page,
	.bf_right_extend_node = bfe_right_extend_node,
	.bf_add_node = bfe_add_node,
	.bf_left_cut_node = bfe_left_cut_node,
	.bf_right_cut_node = bfe_right_cut_node,
	.bf_right_cut_node_adjust = bfe_right_cut_node_adjust,
	.bf_del_node = bfe_del_node,
	.bf_flush_seq = bfe_flush_seq,
	.bf_close = bfe_close,
	.bf_end_page = bfe_end_page,
	.bf_vec_start = bfe_vec_start,
	.bf_vec_stop = bfe_vec_stop,
	.bf_get_stat = bfe_get_stat,
	.bf_get_rw_stat = bfe_get_rw_stat,
	.bf_get_coalesce_index = bfe_get_coalesce_index,
	.bf_reset_coalesce_index = bfe_reset_coalesce_index,
	.bf_get_page_counter = bfe_get_page_counter,
	.bf_trim_list = bfe_trim_list,
};

static void *
_bfe_not_ready_thread(void *p)
{
	char *log_header = "_bfe_not_ready_thread";
	struct bfe_interface *bfe_p = (struct bfe_interface *)p;
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct data_description_node *np, *np2;
	struct pp_page_node *pnp;
	struct d2c_node *tfn;
	struct list_head drop_from_not_ready_head;
	int do_not_ready;

	nid_log_info("%s: start ...", log_header);
	INIT_LIST_HEAD(&drop_from_not_ready_head);

next_try:
	do_not_ready = 0;
	if (!list_empty(&priv_p->p_to_not_ready_head)) {
		pthread_mutex_lock(&priv_p->p_flush_lck);
		list_splice_tail_init(&priv_p->p_to_not_ready_head, &priv_p->p_not_ready_head);
		pthread_mutex_unlock(&priv_p->p_flush_lck);
	}

	list_for_each_entry_safe(np, np2, struct data_description_node, &priv_p->p_not_ready_head, d_flist) {
		assert(np->d_where == DDN_WHERE_NOT_READY_FLUSH);
		assert(np->d_flushing);
		if (!np->d_flush_overwritten) {
			pnp = np->d_page_node;
			tfn = (struct d2c_node *)pnp->pp_tfn;
			if (!np->d_flushed) {
				do_not_ready++;
				list_del(&np->d_flist);	// removed from p_not_ready_head
				priv_p->p_not_ready_counter--;
				tfn->d2_nr_counter--;
				np->d_where = DDN_WHERE_FLUSH;
				nid_log_debug("%s: calling __extend_write_request_async", log_header);
				__extend_write_request_async(priv_p, np, (struct d2c_node *)pnp->pp_tfn, 0);
			} else if (np->d_flush_done) {
				/* already coalesced by other ddn */
				do_not_ready++;
				list_del(&np->d_flist);	// removed from p_not_ready_head
				list_add_tail(&np->d_flist, &drop_from_not_ready_head);
				priv_p->p_not_ready_counter--;
			}
		}
	}

	if (!do_not_ready) {
		pthread_mutex_lock(&priv_p->p_flush_lck);
		while (!priv_p->p_check_not_ready) {
			pthread_cond_wait(&priv_p->p_not_ready_cond, &priv_p->p_flush_lck);
		}
		priv_p->p_check_not_ready = 0;
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		goto next_try;
	}

	while (!list_empty(&drop_from_not_ready_head)) {
		np = list_first_entry(&drop_from_not_ready_head, struct data_description_node, d_flist);
		list_del(&np->d_flist);	// removed from drop_from_not_ready_head
		pnp = np->d_page_node;
		pthread_mutex_lock(&priv_p->p_flush_lck);
		list_add_tail(&np->d_flist, &pnp->pp_release_head);
		np->d_where = DDN_WHERE_RELEASE;
		tfn = (struct d2c_node *)pnp->pp_tfn;
		tfn->d2_nr_counter--;
		if (__bfe_pre_finish_flush(priv_p, pnp)) {
			__bfe_finish_flush(priv_p, pnp);
		}
		pthread_mutex_unlock(&priv_p->p_flush_lck);
	}

	return NULL;
}

/*
 * Algorithm:
 * 	waiting flushing to satisfy p_seq_to_flush
 * 	return immediately if p_seq_to_flush modified
 *
 * Return:
 * 	0: done
 * 	1: stop waiting even not done
 *
 * Note:
 * 	the caller hold flush_lck
 */
static int
__bfe_wait_flushing(struct bfe_private *priv_p)
{
	uint64_t saved_to_flush = priv_p->p_seq_to_flush;

	while ((priv_p->p_seq_to_flush > priv_p->p_last_seq_flushed) &&		// not sure if done
	       (priv_p->p_last_seq_flushing > priv_p->p_last_seq_flushed)) {	// there are some req in flushing

		struct timeval now;
		struct timespec ts;
		gettimeofday(&now, NULL);
		if (now.tv_usec + 10000 < 1000000) {
			ts.tv_sec = now.tv_sec;
			ts.tv_nsec = (now.tv_usec + 10000) * 1000;
		} else {
			ts.tv_sec = now.tv_sec + 1;
			ts.tv_nsec = (now.tv_usec + 10000 - 1000000) * 1000;
		}
		pthread_cond_timedwait(&priv_p->p_flush_cond, &priv_p->p_flush_lck, &ts);
		if (saved_to_flush != priv_p->p_seq_to_flush) {
			/*
			 * p_seq_to_flush could be reset when doing pthread_cond_timedwait
			 */
			pthread_mutex_unlock(&priv_p->p_flush_lck);
			return 1;
		}
	}
	return 0;
}

static void *
_bfe_flush_thread_async(void *p)
{
	char *log_header = "_bfe_flush_thread_async";
	struct bfe_interface *bfe_p = (struct bfe_interface *)p;
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	// struct rw_interface *rw_p = priv_p->p_rw;
	struct d2c_node *tfn;
	uint32_t page_sz = priv_p->p_pagesz;
	struct data_description_node *np, *np2;
	struct pp_page_node *pnp;
	struct list_head to_flush_head2, to_release_head;
	uint64_t seq_to_flush = 0;
	int just_wait_flushing;

	nid_log_warning("%s: start ...", log_header);
	INIT_LIST_HEAD(&to_flush_head2);
	INIT_LIST_HEAD(&to_release_head);

next_try:
	if (priv_p->p_stop)
		goto out;

	pthread_mutex_lock(&priv_p->p_flush_lck);

	while (!priv_p->p_stop && !priv_p->p_seq_to_flush) {
		/*
		 * I'm not required to do any more flushing by the bio module.
		 * so just waiting for requirement from bio
		 */
		pthread_cond_wait(&priv_p->p_flush_cond, &priv_p->p_flush_lck);
	}

	if (priv_p->p_stop) {
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		goto next_try;
	}

	/*
	 * Now, we got flushing requirement from bio, i.e. postive p_seq_to_flush.
	 * First of all, check if the flushing reqirement has already been satisfied
	 */
	assert(priv_p->p_seq_to_flush);
	if (priv_p->p_seq_to_flush <= priv_p->p_last_seq_flushed) {
		/*
		 * the flushing requirement has already been satisfied
		 */
		nid_log_debug("%s: log_header, reset seq_to_flush at step 1", log_header);
		priv_p->p_seq_to_flush = 0;
		seq_to_flush = priv_p->p_last_seq_flushed;
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		__update_seq_flushed(priv_p, seq_to_flush);
		goto next_try;
	}

	just_wait_flushing = 0;

	/*
	 *  If no more page to flush or we are trying to flush enough, don't flush more new page
	 */
	if (list_empty(&priv_p->p_flush_page_head) ||			// no more page left to flush
	    priv_p->p_seq_to_flush <= priv_p->p_last_seq_flushing) {
		just_wait_flushing = 1;
		goto wait_flushing;
	}

	/*
	 * Even we know there is at least one more page in the p_flush_page_head, it doesn't mean
	 * we have to flush it since the first req sequence of this page may be large than p_seq_to_flush.
	 */
	pnp = list_first_entry(&priv_p->p_flush_page_head, struct pp_page_node, pp_rlist);
	assert(pnp->pp_where == PP_WHERE_FLUSH || pnp->pp_where == PP_WHERE_FLUSHING);
	if (list_empty(&pnp->pp_flush_head)) {
		/*
		 * It doesn't mean it is really an empty page. There might be some req of this page are
		 * still in flushing status
		 */
		goto scan_next;
	} else {
		np = list_first_entry(&pnp->pp_flush_head, struct data_description_node, d_flist);
		if (np->d_seq > priv_p->p_seq_to_flush) {
			/*
			 * don't need to flush more, jut wait current flushing done
			 */
			just_wait_flushing = 1;
			goto wait_flushing;
		}
	}

wait_flushing:
	if (just_wait_flushing) {
		if (__bfe_wait_flushing(priv_p)) {
			pthread_mutex_unlock(&priv_p->p_flush_lck);
		} else {
			// Always update bigger sequence
			seq_to_flush =  priv_p->p_last_seq_flushed > priv_p->p_seq_to_flush ?
					priv_p->p_last_seq_flushed : priv_p->p_seq_to_flush;
			priv_p->p_seq_to_flush = 0;	// we are done
			pthread_mutex_unlock(&priv_p->p_flush_lck);
			__update_seq_flushed(priv_p, seq_to_flush);
		}
		goto next_try;
	}

scan_next:
	/*
	 * After all, we know we do need to flush some more
	 */
	tfn = (struct d2c_node *)pnp->pp_tfn;
	nid_log_debug("PP:%p pnp->pp_occupied:%u pnp->pp_flushed:%u tfn->d2_fl_counter:%d page_sz:%u "
		"priv_p->p_seq_to_flush:%lu priv_p->p_last_seq_flushing:%lu priv_p->p_last_seq_flushed:%lu",
		pnp, pnp->pp_occupied,	pnp->pp_flushed, tfn->d2_fl_counter, page_sz,
		priv_p->p_seq_to_flush, priv_p->p_last_seq_flushing, priv_p->p_last_seq_flushed);

	/*
	 * Wait conditions:
	 * 1> Not happen: pnp->pp_occupied == 0;
	 * 2> All occupied size have been flushed, but page not full;
	 * 3> Page is flushing and the flush is partial flush, not full flush.
	 * 4> Previous flushing operation not finished due to IO slow or waiting in not_ready thread
	 */
	if (!pnp->pp_occupied ||
	    (pnp->pp_flushed != page_sz && pnp->pp_occupied == pnp->pp_flushed) ||
	    (tfn->d2_partial_flush && tfn->d2_fl_counter != 0)) {
		struct timeval now;
		struct timespec ts;
		gettimeofday(&now, NULL);
		if (now.tv_usec + 10000 < 1000000) {
			ts.tv_sec = now.tv_sec;
			ts.tv_nsec = (now.tv_usec + 10000) * 1000;
		} else {
			ts.tv_sec = now.tv_sec + 1;
			ts.tv_nsec = (now.tv_usec + 10000 - 1000000) * 1000;
		}
		pthread_cond_timedwait(&priv_p->p_flush_cond, &priv_p->p_flush_lck, &ts);
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		goto next_try;
	}

	if (pnp->pp_occupied == page_sz) {
		tfn->d2_partial_flush = 0;
		list_del(&pnp->pp_rlist);
		list_add_tail(&pnp->pp_rlist, &priv_p->p_flushing_head);
		nid_log_debug("%s: flushing pnp(%p)", log_header, pnp);
	} else {
		tfn->d2_partial_flush = 1;
	}
	pnp->pp_where = PP_WHERE_FLUSHING;

	tfn->d2_fl_counter = 1;
	tfn->d2_fl_seq = tfn->d2_seq;
	priv_p->p_last_seq_flushing = tfn->d2_seq;
	list_for_each_entry_safe(np, np2, struct data_description_node, &pnp->pp_flush_head, d_flist) {
		nid_log_debug("%s: seq_to_flush:%ld, d_seq:%ld", log_header, tfn->d2_seq, np->d_seq);
		assert(np->d_len);			// why could it be 0?
		if (np->d_flushed) {
			/*
			 * coalesced by other ddn
			 * Don't remove it from pp_flush_head here. post_thread should take care
			 */
			continue;
		}

		assert(np->d_where == DDN_WHERE_FLUSH);
		list_del(&np->d_flist);			// removed from pp_flush_head
		assert(!np->d_flushing && !np->d_flushed && !np->d_flushed_back && !np->d_flush_done);
		np->d_flushing = 1;
		np->d_flush_offset = np->d_offset;
		np->d_flush_len = np->d_len;
		np->d_flush_pos = np->d_pos;
		if (!np->d_flush_overwritten) {
			list_add_tail(&np->d_flist, &tfn->d2_head);
		} else {
			list_add_tail(&np->d_flist, &priv_p->p_to_not_ready_head);
			np->d_where = DDN_WHERE_NOT_READY_FLUSH;
			priv_p->p_not_ready_counter++;
			tfn->d2_nr_counter++;
			tfn->d2_fl_counter++;
		}
	}

	nid_log_debug("%s: page flushing %d packets of pnp:%p, fn_seq_to_flush:%lu",
		log_header, tfn->d2_acounter, pnp, priv_p->p_seq_to_flush);

	pthread_mutex_unlock(&priv_p->p_flush_lck);
	list_for_each_entry(np, struct data_description_node, &tfn->d2_head, d_flist) {
		if (!np->d_flushed) {
			nid_log_debug("%s: calling __extend_write_request_async", log_header);
			__extend_write_request_async(priv_p, np, tfn, 1);
		}
	}

	pthread_mutex_lock(&priv_p->p_flush_lck);
	list_splice_tail_init(&tfn->d2_head, &pnp->pp_flush_head);
	nid_log_debug("%s: flushing done, fn_pnp:%p", log_header, pnp);
	tfn->d2_fl_counter--;
	if (!tfn->d2_partial_flush) {
		__sync_sub_and_fetch(&tfn->d2_acounter, 1);
		if (__bfe_pre_finish_flush(priv_p, pnp)) {
			nid_log_debug("%s: finish flush ...", log_header);
			__bfe_finish_flush(priv_p, pnp);
		}
	} else {
		if (tfn->d2_fl_counter == 0) {
			assert(tfn->d2_fl_seq != 0);
			priv_p->p_last_seq_flushed = tfn->d2_fl_seq;
			pnp->pp_where = PP_WHERE_FLUSH;
			pthread_cond_broadcast(&priv_p->p_flush_cond);
		}
	}
	pthread_mutex_unlock(&priv_p->p_flush_lck);
	goto next_try;

out:
	priv_p->p_flush_stop = 1;
	return NULL;
}

/*
 * Algorithm:
 * 	If some ddn are waiting for me to be flushed back, inform them I'm flushed back
 * 	fan->d1_arg.ag_overwritten_head contains all ddn which got one flushed back ddn which
 * 	they are waiting for.
 */
static void
__bfe_overwritten_cleanup(struct bfe_private *priv_p, struct d1a_node *fan)
{
	char *log_header = "__bfe_overwritten_cleanup";
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct ddn_interface *ddn_p = priv_p->p_ddn;
	struct d1an_interface *d1an_p = &priv_p->p_d1an;
	struct list_node *lnp;
	struct data_description_node *dnp;
	struct pp_page_node *pnp;
	struct d1a_node *fan2;
	struct d2c_node *tfn;
	struct list_head ddn_head;
	int do_signal = 0;

	INIT_LIST_HEAD(&ddn_head);
	list_splice_tail_init(&fan->d1_arg.ag_overwritten_head, &ddn_head);
	while (!list_empty(&ddn_head)) {
		lnp = list_first_entry(&ddn_head, struct list_node, ln_list);
		list_del(&lnp->ln_list);
		dnp = (struct data_description_node *)lnp->ln_data;
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);

		/*
		 * Process this dnp
		 */
		__sync_sub_and_fetch(&dnp->d_flush_overwritten, 1);
		nid_log_debug("%s: return_call_overwrtiiten new_np: %p", log_header, dnp);
		priv_p->p_overwritten_back_counter++;
		pnp = dnp->d_page_node;
		tfn = (struct d2c_node *)pnp->pp_tfn;
		tfn->d2_ow_counter--;
		if (!dnp->d_flush_overwritten && dnp->d_flushing) {
			do_signal = 1;
			if (__bfe_pre_finish_flush(priv_p, pnp))
			       __bfe_finish_flush(priv_p, pnp);
		}

		/*
		 * If this ddn is not waiting for any body else, and there is a list of ddn waiting for this dnp,
		 * add the list to the tail of ddn_head
		 */
		if (!dnp->d_flush_overwritten && (fan2 = (struct d1a_node *)dnp->d_callback_arg)) {
			list_splice_tail_init(&fan2->d1_arg.ag_overwritten_head, &ddn_head);
			if (dnp->d_where == DDN_WHERE_SHADOW) {
				d1an_p->d1_op->d1_put_node(d1an_p, fan2);
				ddn_p->d_op->d_put_node(ddn_p, dnp);
			}
		}
	}

	if (do_signal) {
		priv_p->p_check_not_ready = 1;
		pthread_cond_broadcast(&priv_p->p_not_ready_cond);
	}
}


static void *
_bfe_post_async_flush_thread(void *p)
{
	char *log_header = "_bfe_post_async_flush_thread";
	struct bfe_interface *bfe_p = (struct bfe_interface *)p;
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	struct d1an_interface *d1an_p = &priv_p->p_d1an;
	uint32_t page_sz = priv_p->p_pagesz;
	struct d1a_node *fan, *fan2;
	struct d2c_node *tfn, *my_tfn;
	struct rw_interface *rw_p = priv_p->p_rw;
	struct fpn_interface *fpn_p = rw_p->rw_op->rw_get_fpn_p(rw_p);
	struct rc_interface *rc_p = priv_p->p_rc;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	void *rc_handle = priv_p->p_rc_handle;
	struct list_node *lnp;
	struct data_description_node *my_dnp, *dnp;
	struct pp_page_node *pnp, *my_pnp;

	nid_log_warning("%s: start ...", log_header);
next_try:
	if (priv_p->p_stop)
		goto out;

	pthread_mutex_lock(&priv_p->p_flush_finish_lck);
	while (list_empty(&priv_p->p_flush_finished_head)) {
		pthread_cond_wait(&priv_p->p_flush_post_cond, &priv_p->p_flush_finish_lck);
		if (priv_p->p_stop) {
			pthread_mutex_unlock(&priv_p->p_flush_finish_lck);
			sleep(1);
			goto next_try;
		}
	}

	while (!list_empty(&priv_p->p_flush_finished_head)) {
		nid_log_notice("%s: got something flushed back", log_header);
		fan = list_first_entry(&priv_p->p_flush_finished_head, struct d1a_node, d1_list);
		list_del(&fan->d1_list);
		pthread_mutex_unlock(&priv_p->p_flush_finish_lck);

		my_dnp = (struct data_description_node *)fan->d1_arg.ag_request;
		assert(my_dnp->d_callback_arg == fan);
		priv_p->p_flush_back_counter++;

		// If read cache defined and not update and write return code is right, do rc updatev.
		if (rc_p && fan->d1_flag == 0 && fan->d1_ecode == 0) {
			rc_p->rc_op->rc_updatev(rc_p, rc_handle, fan->d1_iov,
				fan->d1_counter, fan->d1_offset, &fan->d1_arg.ag_fp_head);
		}

		pthread_mutex_lock(&priv_p->p_flush_lck);

		while (!list_empty(&fan->d1_arg.ag_coalesce_head)) {
			lnp = list_first_entry(&fan->d1_arg.ag_coalesce_head, struct list_node, ln_list);
			list_del(&lnp->ln_list);	// removed from fan->d1_arg.ag_coalesce_head
			dnp = (struct data_description_node *)lnp->ln_data;
			lstn_p->ln_op->ln_put_node(lstn_p, lnp);
			pnp = dnp->d_page_node;
			tfn = (struct d2c_node *)pnp->pp_tfn;
			assert(dnp->d_flushing && dnp->d_flushed && !dnp->d_flushed_back && !dnp->d_flush_done);

			/*
			 * Inform all nodes which are waiting for dnp flushed back
			 */
			if ((fan2 = (struct d1a_node *)dnp->d_callback_arg)) {
				assert(tfn == fan2->d1_data);
				__bfe_overwritten_cleanup(priv_p, fan2);
				d1an_p->d1_op->d1_put_node(d1an_p, fan2);
				dnp->d_callback_arg = NULL;
			}

			if (dnp->d_where == DDN_WHERE_FLUSH)
				dnp->d_where = DDN_WHERE_RELEASE;
			else
				assert(dnp->d_where == DDN_WHERE_NOT_READY_FLUSH);

			pnp->pp_flushed += dnp->d_len;	// Not d_flush_len, always use d_len to calculate pp_flushed
			assert(pnp->pp_flushed <= page_sz);
			dnp->d_flushed_back = 1;
			dnp->d_flush_done = 1;
			__sync_sub_and_fetch(&pnp->pp_coalesced, 1);

			// update bfe coalesce index
			__sync_sub_and_fetch(&priv_p->p_coalesce_index, 1);

			__sync_sub_and_fetch(&tfn->d2_acounter, 1);
			if (__bfe_pre_finish_flush(priv_p, pnp)) {
				nid_log_debug("%s: finish flush ...", log_header);
				__bfe_finish_flush(priv_p, pnp);
			}

			priv_p->p_coalesce_flush_back_counter++;
		}

		/*
		 * Inform all nodes which are waiting for my_dnp flushed back
		 */
		if (!list_empty(&fan->d1_arg.ag_overwritten_head)) {
			nid_log_debug("%s: return_call_overwrtiiten old _np: %p", log_header, dnp);
			__bfe_overwritten_cleanup(priv_p, fan);
		}

		assert(my_dnp->d_flushing && my_dnp->d_flushed && !my_dnp->d_flushed_back && !my_dnp->d_flush_done);
		my_dnp->d_flushed_back = 1;
		assert(my_dnp->d_len);
		my_pnp = my_dnp->d_page_node;
		my_pnp->pp_flushed += my_dnp->d_len;	// Not d_flush_len, always use d_len to calculate pp_flushed
		assert(my_pnp->pp_flushed <= page_sz);
		my_dnp->d_flush_done = 1;
		my_dnp->d_where = DDN_WHERE_RELEASE;
		my_dnp->d_callback_arg = NULL;

		my_tfn = fan->d1_data;
		assert(my_tfn == my_pnp->pp_tfn);
		__sync_sub_and_fetch(&my_tfn->d2_acounter, 1);
		my_tfn->d2_fl_counter--;

		if (!my_tfn->d2_partial_flush) {
			if (__bfe_pre_finish_flush(priv_p, my_pnp)) {
				nid_log_debug("%s: finish flush ...", log_header);
				__bfe_finish_flush(priv_p, my_pnp);
			}
		} else {
			if (my_tfn->d2_fl_counter == 0) {
				assert(my_tfn->d2_fl_seq);
				priv_p->p_last_seq_flushed = my_tfn->d2_fl_seq;
				my_pnp->pp_where = PP_WHERE_FLUSH;
				pthread_cond_broadcast(&priv_p->p_flush_cond);
			}
		}

		pthread_mutex_unlock(&priv_p->p_flush_lck);

		/*
		 *  release memory both with finger print and argument.
		 */
		while (!list_empty(&fan->d1_arg.ag_fp_head)) {
			struct fp_node *fpnd = list_first_entry(&fan->d1_arg.ag_fp_head, struct fp_node, fp_list);
			list_del(&fpnd->fp_list);
			fpn_p->fp_op->fp_put_node(fpn_p, fpnd);
		}

		d1an_p->d1_op->d1_put_node(d1an_p, fan);
		pthread_mutex_lock(&priv_p->p_flush_finish_lck);
	}

	pthread_mutex_unlock(&priv_p->p_flush_finish_lck);
	goto next_try;

out:
	priv_p->p_post_flush_stop = 1;
	return NULL;
}

int
bfea_initialization(struct bfe_interface *bfe_p, struct bfe_setup *setup)
{
	char *log_header = "bfe_initialization";
	struct bfe_private *priv_p;
	struct pp_interface *pp_p;
	struct d1an_interface *d1an_p;
	struct d1an_setup d1an_setup;
	struct d2cn_interface *d2cn_p;
	struct d2cn_setup d2cn_setup;
	pthread_attr_t attr_flush, attr_post_flush;
	pthread_t thread_id;
	mode_t mode;
	int flags;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	bfe_p->bf_op = &bfe_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bfe_p->bf_private = priv_p;
	pthread_mutex_init(&priv_p->p_flush_lck, NULL);
	pthread_mutex_init(&priv_p->p_flush_finish_lck, NULL);
	pthread_mutex_init(&priv_p->p_trim_lck, NULL);
	pthread_cond_init(&priv_p->p_flush_cond, NULL);
	pthread_cond_init(&priv_p->p_not_ready_cond, NULL);
	pthread_cond_init(&priv_p->p_overwritten_cond, NULL);
	pthread_cond_init(&priv_p->p_flush_post_cond, NULL);
	pthread_cond_init(&priv_p->p_flush_page_cond, NULL);
	INIT_LIST_HEAD(&priv_p->p_flush_page_head);
	INIT_LIST_HEAD(&priv_p->p_flush_finished_head);
	INIT_LIST_HEAD(&priv_p->p_flushing_head);
	INIT_LIST_HEAD(&priv_p->p_not_ready_head);
	INIT_LIST_HEAD(&priv_p->p_to_not_ready_head);
	priv_p->p_allocator = setup->allocator;
	priv_p->p_pp = setup->pp;
	priv_p->p_bse = setup->bse;
	priv_p->p_bre = setup->bre;
	priv_p->p_wc = setup->wc;
	priv_p->p_rc = setup->rc;
	priv_p->p_rw = setup->rw;
	priv_p->p_rw_handle = setup->rw_handle;
	priv_p->p_wc_chain_handle = setup->wc_chain_handle;
	priv_p->p_rc_handle = setup->rc_handle;
	priv_p->p_do_fp = setup->do_fp;
	priv_p->p_do_coalescing = 1;
	priv_p->p_lstn = setup->lstn;
	priv_p->p_ddn = setup->ddn;
	priv_p->p_ssd_mode = setup->ssd_mode;
	pp_p = priv_p->p_pp;
	priv_p->p_pagesz = pp_p->pp_op->pp_get_pagesz(pp_p);
	priv_p->p_vec_stat = 0;
	priv_p->p_coalesce_index = 0;
	priv_p->p_write_max_retry_time = 8;
	priv_p->p_flush_pp_page_size = BFE_MAX_IOV * 128 * 1024;

	struct pp_setup pp_setup;
	pp_setup.allocator = setup->allocator;
	strcpy(pp_setup.pp_name, "bfe_flush_pp") ;
	pp_setup.page_size = priv_p->p_flush_pp_page_size >> 20;
	pp_setup.pool_size = BFE_FLUSH_POOL_SIZE;
	pp_setup.set_id = 0; //Private use, currently not assign PP2_ID.
	pp_initialization(&priv_p->p_pp_flush, &pp_setup);


	priv_p->p_bufdevice = setup->bufdevice;
	mode = 0600;
	flags = O_RDONLY;
	priv_p->p_fhandle = open(priv_p->p_bufdevice, flags, mode);
	if (priv_p->p_fhandle < 0) {
		nid_log_error("%s: cannot open %s", log_header, priv_p->p_bufdevice);
		assert(0);
	}

	d2cn_setup.allocator = priv_p->p_allocator;
	d2cn_setup.set_id = ALLOCATOR_SET_BFE_D2CN;
	d2cn_setup.seg_size = 128;
	d2cn_p = &priv_p->p_d2cn;
	d2cn_initialization(d2cn_p, &d2cn_setup);

	d1an_setup.allocator = priv_p->p_allocator;
	d1an_setup.set_id = ALLOCATOR_SET_BFE_D1AN;
	d1an_setup.seg_size = 128;
	d1an_setup.node_size = sizeof(struct d1a_node) + sizeof(struct iovec) * BFE_MAX_IOV;;
	d1an_p = &priv_p->p_d1an;
	d1an_initialization(d1an_p, &d1an_setup);

	pthread_attr_init(&attr_flush);
	pthread_attr_setdetachstate(&attr_flush, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr_flush, _bfe_not_ready_thread, (void *)bfe_p);

	pthread_attr_init(&attr_flush);
	pthread_attr_setdetachstate(&attr_flush, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr_flush, _bfe_flush_thread_async, (void *)bfe_p);

	pthread_attr_init(&attr_post_flush);
	pthread_attr_setdetachstate(&attr_post_flush, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr_post_flush, _bfe_post_async_flush_thread, (void *)bfe_p);

	return 0;
}
