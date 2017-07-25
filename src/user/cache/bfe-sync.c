/*
 * bfe.c
 * 	Implementation of BIO Flush Engine Version 0 Module
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
#include "pp_if.h"
#include "sac_if.h"

#define time_difference(t1, t2) \
	((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

/* Retain enough memory for avoid use get_node_forcebly, which may cause memory leaking.*/
#define BFE_FLUSH_POOL_SIZE	16

struct bfe_private {
	pthread_mutex_t			p_flush_lck;
	pthread_cond_t			p_flush_cond;
	pthread_mutex_t			p_trim_lck;
	struct list_head		p_flush_page_head;
	char		 		*p_flush_buffer;
	struct allocator_interface	*p_allocator;
	struct wc_interface		*p_wc;
	void				*p_wc_chain_handle;
	struct rc_interface		*p_rc;
	void				*p_rc_handle;
	struct rw_interface		*p_rw;
	void				*p_rw_handle;
	struct bse_interface		*p_bse;
	struct bre_interface		*p_bre;
	struct pp_interface		*p_pp;
	char				*p_bufdevice;
	uint64_t			p_seq_to_flush;
	uint64_t			p_last_seq_flushed;
	uint64_t			p_prepare_flush_seq;
	uint64_t			p_seq_updated;
	uint64_t			p_total_vecnum;
	uint64_t			p_total_flushsize;
	uint64_t			p_flush_counter;
	uint64_t			p_flush_back_counter;
	uint64_t			p_coalesce_flush_counter;
	uint64_t			p_coalesce_flush_back_counter;
	uint64_t			p_coalesce_index;
	int				p_vec_stat;
	int				p_wc_fd;
	uint32_t			p_pagesz;
	uint32_t			p_flush_num;
	uint32_t			p_flush_page_counter;	// number of pages in p_flush_page_head
	int				p_max_flush_size;
	int				p_target_sync;
	uint8_t				p_stop;
	uint8_t				p_do_fp;
	uint8_t				p_ssd_mode;
	uint8_t				p_flush_stop;
	uint8_t				p_triming;
};

inline static void
__update_seq_flushed(struct bfe_private *priv_p, uint64_t seq)
{
	struct wc_interface *wc_p = priv_p->p_wc;
	struct rw_interface *rw_p = priv_p->p_rw;

	if (!priv_p->p_target_sync) {
		rw_p->rw_op->rw_sync(rw_p, priv_p->p_rw_handle);
	}

	wc_p->wc_op->wc_flush_update(wc_p, priv_p->p_wc_chain_handle, seq);
	priv_p->p_seq_updated = seq;
}

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

static ssize_t
__extend_write_request(struct bfe_private *priv_p, struct data_description_node *dnp)
{
	char *log_header = "__extend_write_request";
	struct bse_interface *bse_p = priv_p->p_bse;
	struct rw_interface *rw_p = priv_p->p_rw;
	struct rc_interface *rc_p = priv_p->p_rc;
	void *rc_handle = priv_p->p_rc_handle;
	int do_fp = priv_p->p_do_fp;
	struct fpn_interface *fpn_p = rw_p->rw_op->rw_get_fpn_p(rw_p);
	struct data_description_node *low_dnp, *cur_dnp, *pre_dnp = NULL, *dnps[BFE_MAX_IOV];
	struct iovec iov[BFE_MAX_IOV];
	ssize_t total_write = 0;
	int vec_counter = 0, got_me = 0, i, max_flush_size = 0, has_ssd_location;
	struct list_head fp_head;
	void *align_buf;
	char *data_buf;
	size_t align_count;
	ssize_t ret;
	off_t align_offset;
	int32_t write_size;
	suseconds_t time_consume;
	struct timeval write_start_time;
	struct timeval write_end_time;

	assert(rw_p);
	nid_log_debug("%s: start (bse_p:%p, dnp:%p)...", log_header, bse_p, dnp);

	vec_counter = 0;
	write_size = 0;
	has_ssd_location = 0;
	data_buf = priv_p->p_flush_buffer;

	bse_p->bs_op->bs_traverse_start(bse_p, NULL);   // just hold lock
	pthread_mutex_lock(&priv_p->p_flush_lck);

	if (dnp->d_flushed)
	{
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		bse_p->bs_op->bs_traverse_end(bse_p);
		return dnp->d_len;
	}

	if (priv_p->p_max_flush_size)
		max_flush_size = priv_p->p_max_flush_size - dnp->d_flush_len;
	low_dnp = bse_p->bs_op->bs_contiguous_low_bound(bse_p, dnp, BFE_MAX_IOV - 1, max_flush_size);
	bse_p->bs_op->bs_traverse_set(bse_p, low_dnp);
	while (vec_counter < BFE_MAX_IOV && (cur_dnp = bse_p->bs_op->bs_traverse_next(bse_p))) {
		//nid_log_debug("%s: pre_dnp:%p, cur_dnp:%p, cur_flushing:%d, cur_offset:%ld, vec_counter:%d",
		//  log_header, pre_dnp, cur_dnp, cur_dnp->d_flushing, cur_dnp->d_offset, vec_counter);

		if (cur_dnp == dnp)
			got_me = 1;

		if (cur_dnp->d_flushed)
			break;

		assert(!cur_dnp->d_flush_done);
		if (cur_dnp->d_flushing) {
			if (pre_dnp && (pre_dnp->d_flush_offset + pre_dnp->d_flush_len != cur_dnp->d_flush_offset)){
				assert (cur_dnp != dnp);
				break;
			}
			if (priv_p->p_max_flush_size &&
			    ((int)(write_size + cur_dnp->d_flush_len) > priv_p->p_max_flush_size)) {
				// nid_log_warning("%s: rejecting cur_dnp:%p", log_header, cur_dnp);
				if (cur_dnp != dnp)
					break;
			}
		} else {
			if (pre_dnp && (pre_dnp->d_flush_offset + pre_dnp->d_flush_len != cur_dnp->d_offset)){
				assert (cur_dnp != dnp);
				break;
			}
			if (priv_p->p_max_flush_size &&
			    ((int)(write_size + cur_dnp->d_len) > priv_p->p_max_flush_size)) {
				if (cur_dnp != dnp)
					break;
			}

			cur_dnp->d_flushing = 1;
			cur_dnp->d_flush_offset = cur_dnp->d_offset;
			cur_dnp->d_flush_len = cur_dnp->d_len;
			cur_dnp->d_flush_pos = cur_dnp->d_pos;
		}

		if (cur_dnp->d_location_disk) {
			assert(priv_p->p_max_flush_size >= (int)cur_dnp->d_flush_len);
			/* Do not read data in flush lock*/
//			pread(priv_p->p_fhandle, data_buf, cur_dnp->d_flush_len, cur_dnp->d_pos);
			iov[vec_counter].iov_base = data_buf;
			data_buf += cur_dnp->d_flush_len;
			has_ssd_location = 1;
		} else {
			iov[vec_counter].iov_base = (void*)cur_dnp->d_flush_pos;
		}
		dnps[vec_counter] = cur_dnp;
		iov[vec_counter++].iov_len = cur_dnp->d_flush_len;

		cur_dnp->d_flushed = 1;
		pre_dnp = cur_dnp;
		if (cur_dnp != dnp)
			priv_p->p_coalesce_flush_counter++;
		write_size += cur_dnp->d_flush_len;
		assert(write_size <= priv_p->p_max_flush_size);
	}

	pthread_mutex_unlock(&priv_p->p_flush_lck);
	bse_p->bs_op->bs_traverse_end(bse_p);
	assert(got_me);

	/*
	 * As both none-ssd_mode and ssd_mode may have dnp on SSD, so use a flag check for avoid do loop every time.
	 */
	if (has_ssd_location) {
		for (i=0; i<vec_counter; i++) {
			cur_dnp = dnps[i];
			if (cur_dnp->d_location_disk) {
				pread(priv_p->p_wc_fd, iov[i].iov_base, cur_dnp->d_flush_len, cur_dnp->d_flush_pos);
			}
		}
	}

	/*
	* Now, we are doing buffer write without holding lock (p_flush_lck)
	*/
	if (priv_p->p_vec_stat == 1){
		priv_p->p_total_flushsize += write_size;
		priv_p->p_total_vecnum += vec_counter;
		priv_p->p_flush_num += 1;
	}

	if (rc_p && do_fp) {
		priv_p->p_flush_counter++;
		gettimeofday(&write_start_time, NULL);
		ret = rw_p->rw_op->rw_pwritev_fp(rw_p, priv_p->p_rw_handle, iov, vec_counter, low_dnp->d_flush_offset, &fp_head, &align_buf, &align_count, &align_offset);
		gettimeofday(&write_end_time, NULL);
		time_consume = time_difference(write_start_time, write_end_time);
		if (time_consume > 1000000) {
			nid_log_warning("%s, one write consumed: %ld", log_header, time_consume);
		}
		priv_p->p_flush_back_counter++;
		if (ret < 0) {
			nid_log_error("Target device write failed, error number: %d", errno);
			assert(0);
		}
		if (align_buf) {
			rc_p->rc_op->rc_update(rc_p, rc_handle, align_buf, align_count, align_offset, &fp_head);
			free(align_buf);
		} else {
			rc_p->rc_op->rc_updatev(rc_p, rc_handle, iov, vec_counter, low_dnp->d_flush_offset, &fp_head);
		}
		/* Free finger print nodes */
		while (!list_empty(&fp_head)) {
			struct fp_node *fpnd = list_first_entry(&fp_head, struct fp_node, fp_list);
			list_del(&fpnd->fp_list);
			fpn_p->fp_op->fp_put_node(fpn_p, fpnd);
		}
	} else if (rc_p) {
		/*Not support yet, as currently read cache need fp.*/
		assert(0);
	} else {
		assert(!do_fp);
		priv_p->p_flush_counter++;
		gettimeofday(&write_start_time, NULL);
		ret = rw_p->rw_op->rw_pwritev(rw_p, priv_p->p_rw_handle, iov, vec_counter, low_dnp->d_flush_offset);
		gettimeofday(&write_end_time, NULL);
		time_consume = time_difference(write_start_time, write_end_time);
		if (time_consume > 1000000) {
			nid_log_warning("%s, one write consumed: %ld", log_header, time_consume);
		}
		if (ret < 0) {
			nid_log_error("Target device write failed, error number: %d", errno);
			assert(0);
		}

		priv_p->p_flush_back_counter++;
	}

	return total_write;
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

	nid_log_debug("%s: adding page %p", log_header, pnp);
	pthread_mutex_lock(&priv_p->p_flush_lck);
 	list_add_tail(&pnp->pp_rlist, &priv_p->p_flush_page_head);
	priv_p->p_flush_page_counter++;
	assert(pnp->pp_where != PP_WHERE_FLUSH);
	pnp->pp_where = PP_WHERE_FLUSH;
	pnp->pp_last_flush_seq = 0;
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
	if (!np->d_flushing && ((int)(np->d_len + new_np->d_len) < priv_p->p_max_flush_size)) {
		np->d_len += new_np->d_len;
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
	struct data_description_node *pre_np;

	nid_log_debug("%s: start (new_np:%p) ...", log_header, new_np);
	assert(new_np->d_where == DDN_WHERE_NO);
	pthread_mutex_lock(&priv_p->p_flush_lck);
	list_add_tail(&new_np->d_flist, &pnp->pp_flush_head);
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

	if (np->d_flushing && !np->d_flushed) {
		/* don't flush the cut part */
		np->d_flush_pos = np->d_pos;
		np->d_flush_offset = np->d_offset;
		np->d_flush_len = np->d_len;
	}

	if (!np->d_flush_done) {
		pnp->pp_flushed += cut_len;
		assert(pnp->pp_flushed < page_sz);
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
	if (np->d_flushing && !np->d_flushed) {
		/* don't flush the cut part */
		np->d_flush_len = np->d_len;
	}

	if (!np->d_flush_done) {
		pnp->pp_flushed += cut_len;
		assert(pnp->pp_flushed < page_sz);
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
 *
 * Data Corruption Fix:
 * 	A special case is right cut adjust, the request will divide into two pieces by the new request, we only flush the first piece in this round, and add the left one
 * 	into the flush list,flush thread will flush that one in the next round. The prepare_flush_seq need to reset to the seq_num of the request A.
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
	if (!np->d_flushed) {
		assert(!np->d_flush_done);
		if (np->d_flushing) {
			np->d_flush_len = np->d_len;
			if (priv_p->p_prepare_flush_seq > np->d_seq) {
				priv_p->p_prepare_flush_seq = np->d_seq;
				nid_log_debug("Set priv_p->p_prepare_flush_seq:%lu",
						priv_p->p_prepare_flush_seq);
			}
		}

		a_np->d_flushing = 0;
		a_np->d_flushed = 0;
		a_np->d_flush_done = 0;

		/*
		 * insert the a_np to the flush list. It will be flushed later
		 */
		list_add_tail(&a_np->d_flist, &pnp->pp_flush_head);
		a_np->d_where = DDN_WHERE_FLUSH;

		pnp->pp_flushed += cut_len - adjust_len;
		assert(pnp->pp_flushed <= page_sz);

	} else {
		assert(np->d_flushing);
		a_np->d_flushing = 1;
		a_np->d_flushed = 1;	// don't flush it
		a_np->d_flush_done = 1;	// don't count it in pp_flushed

		/*
		 * Insert the a_np in the release list instead of flush list, since it shouldn't
		 * be flushed later.
		 */
		list_add_tail(&a_np->d_flist, &pnp->pp_release_head);
		a_np->d_where = DDN_WHERE_RELEASE;

		if (!np->d_flush_done) {
			/*
			 * the adjust np (a_np) won't be flushed. the np will got be flushed w/o cut.
			 * But it's len already got reduced, its reduced part won't be added to pp_flushed.
			 * So add it here.
			 */
			pnp->pp_flushed += cut_len;
			assert(pnp->pp_flushed <= page_sz);
		}
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
	} else {
		/* don't flush it */
		np->d_flushed = 1;
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

	nid_log_notice("%s: start ...", log_header);
	priv_p->p_stop = 1;
	while (!priv_p->p_flush_stop) {
		usleep(100*1000);

		// _bfe_flush_thread my miss signal, we have to send it again and again
		pthread_cond_broadcast(&priv_p->p_flush_cond);
	}

	pthread_mutex_destroy(&priv_p->p_flush_lck);
	pthread_cond_destroy(&priv_p->p_flush_cond);
	pthread_mutex_destroy(&priv_p->p_trim_lck);
	if (priv_p->p_wc_fd >= 0) {
		close(priv_p->p_wc_fd);
		priv_p->p_wc_fd = -1;
	}
	free(priv_p);
	bfe_p->bf_private = NULL;

	nid_log_notice("%s: end ...", log_header);
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
//	char *log_header = "bfe_get_rw_stat";
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;

	sp->flush_page_num = priv_p->p_flush_page_counter;
	sp->overwritten_num = 0; //priv_p->p_overwritten_counter;
	sp->overwritten_back_num = 0; //priv_p->p_overwritten_back_counter;
	sp->coalesce_flush_num = priv_p->p_coalesce_flush_counter;
	sp->coalesce_flush_back_num = priv_p->p_coalesce_flush_back_counter;
	sp->flush_num = priv_p->p_flush_counter;
	sp->flush_back_num = priv_p->p_flush_back_counter;
	sp->not_ready_num = 0; //priv_p->p_not_ready_counter;
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

/*
 *
 * Data Corruption Bug:
 * We have unlocked the lock after set all requests' flushing flag of one page. The new requests may join the bse durning that time.
 * The corrupte problem will happened if the new request overwrite parts of old request A , and be coalesced by request B.
 * If the request B flushed earlier than request A, the older data will be write to the disk.
 * The solution is do not flush the overwrite part of request A even it was in flushing status.
 *
 *
 */
static void *
_bfe_flush_thread(void *p)
{
	char *log_header = "_bfe_flush_thread";
	struct bfe_interface *bfe_p = (struct bfe_interface *)p;
	struct bfe_private *priv_p = (struct bfe_private *)bfe_p->bf_private;
	uint32_t page_sz = priv_p->p_pagesz;
	struct bre_interface *bre_p = priv_p->p_bre;
	struct data_description_node *np, *np2;
	struct pp_page_node *pnp;
	struct list_head to_flush_head, to_release_head;
	uint32_t flush_counter;
	uint64_t seq_to_flush = 0;
	uint8_t fast_flush = 0;

	nid_log_warning("%s: start ...", log_header);
	INIT_LIST_HEAD(&to_flush_head);
	INIT_LIST_HEAD(&to_release_head);

next_try:
	pthread_mutex_lock(&priv_p->p_flush_lck);
	if (priv_p->p_stop) {
		if(list_empty(&priv_p->p_flush_page_head)) {
			pthread_mutex_unlock(&priv_p->p_flush_lck);
			goto out;
		}
		fast_flush = 1;
		goto do_flush;
	}

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
	 * Now, we got flushing requirement from bio, postive p_seq_to_flush.
	 * First of all, check if the flushing reqirement has already been done
	 */
	if (priv_p->p_seq_to_flush <= priv_p->p_last_seq_flushed) {
		/*
		 * the flushing requirement has already been done
		 */
		priv_p->p_seq_to_flush = 0;
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		__update_seq_flushed(priv_p, priv_p->p_last_seq_flushed);
		goto next_try;
	}

	/*
	 * if we have nothing to flush, just inform/update bio
	 */
	if (list_empty(&priv_p->p_flush_page_head)) {
		seq_to_flush = priv_p->p_seq_to_flush;
		priv_p->p_seq_to_flush = 0;
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		__update_seq_flushed(priv_p, seq_to_flush);
		goto next_try;
	}

	/*
	 * if there is nothing ready to flush, waiting for the prepare thread to inform me
	 */
	while (!priv_p->p_stop && list_empty(&priv_p->p_flush_page_head)) {
		pthread_cond_wait(&priv_p->p_flush_cond, &priv_p->p_flush_lck);
	}
	if (priv_p->p_stop) {
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		goto next_try;
	}

do_flush:
	if(list_empty(&priv_p->p_flush_page_head)) {
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		goto out;
	}

	pnp = list_first_entry(&priv_p->p_flush_page_head, struct pp_page_node, pp_rlist);
	assert(pnp->pp_where == PP_WHERE_FLUSH);
	assert(pnp->pp_rlist.prev == &priv_p->p_flush_page_head);

	/*
	 * Got a page from p_flush_page_head, but not necessary there is anything to flush
	 * in this page. if its pp_flush_head is empty, we still have nothing to flush, update
	 * the bio module.
	 * Notice: if got an empty pp_flush_head of this page, its pp_flushed must be zero(?)
	 */
	// if page has nothing to flush, release page directly
	if (list_empty(&pnp->pp_flush_head)) {
		if(pnp->pp_flushed != page_sz) {
			seq_to_flush = priv_p->p_seq_to_flush;
			if(fast_flush) {
				list_del(&pnp->pp_rlist);
				bfe_p->bf_op->bf_end_page(bfe_p, pnp);
				if (priv_p->p_ssd_mode) {
					bre_p->br_op->br_release_page_immediately(bre_p, pnp);
				} else {
					bre_p->br_op->br_release_page(bre_p, pnp);
				}
				if(seq_to_flush > priv_p->p_seq_updated) {
					__update_seq_flushed(priv_p, seq_to_flush);
				}
				goto do_flush;
			} else {
				priv_p->p_seq_to_flush = 0;
				pthread_mutex_unlock(&priv_p->p_flush_lck);
				__update_seq_flushed(priv_p, seq_to_flush);
				goto next_try;
			}
		}
	}

	/*
	 * we are here, since we know we have something to flush
	 */
	flush_counter = 0;
	seq_to_flush = 0;
	assert(list_empty(&to_flush_head));
	list_for_each_entry_safe(np, np2, struct data_description_node, &pnp->pp_flush_head, d_flist) {
		nid_log_debug("%s: seq_to_flush:%ld, d_seq:%ld", log_header, seq_to_flush, np->d_seq);
		assert(np->d_where == DDN_WHERE_FLUSH);
		list_del(&np->d_flist);         // removed from pp_flush_head
		if (np->d_flushed) {
			assert(np->d_flushing && !np->d_flush_done);
			list_add_tail(&np->d_flist, &pnp->pp_release_head);
			np->d_where = DDN_WHERE_RELEASE;
			pnp->pp_flushed += np->d_len;   // Not d_flush_len, always use d_len to calculate pp_flushed
			assert(pnp->pp_flushed <= page_sz);
			np->d_flush_done = 1;
			seq_to_flush = np->d_seq;
			if (np->d_seq > pnp->pp_last_flush_seq) {
				pnp->pp_last_flush_seq = np->d_seq;
			}
			continue;
		}

		assert(!np->d_flushing && !np->d_flushed && !np->d_flush_done);
		np->d_flushing = 1;
		np->d_flush_offset = np->d_offset;
		np->d_flush_len = np->d_len;
		np->d_flush_pos = np->d_pos;
		flush_counter++;
		list_add_tail(&np->d_flist, &to_flush_head);
		seq_to_flush = np->d_seq;
		if (np->d_seq > pnp->pp_last_flush_seq) {
			pnp->pp_last_flush_seq = np->d_seq;
		}
	}
	pthread_mutex_unlock(&priv_p->p_flush_lck);

	nid_log_debug("%s: page flushing %d packets of pnp:%p, seq_to_flush:%lu, flush_counter:%d",
		log_header, flush_counter, pnp, seq_to_flush, flush_counter);

	list_for_each_entry(np, struct data_description_node, &to_flush_head, d_flist) {
		flush_counter--;
		if (!np->d_flushed)
			__extend_write_request(priv_p, np);
	}
	nid_log_debug("%s: flushing done, pnp:%p", log_header, pnp);

	pthread_mutex_lock(&priv_p->p_flush_lck);

	nid_log_debug("%s: seq_to_flush:%lu priv_p->p_waiting_flush_seq:%lu pnp->pp_flushed:%u page_sz:%u",
			log_header, seq_to_flush, priv_p->p_prepare_flush_seq, pnp->pp_flushed, page_sz);
	if (list_empty(&pnp->pp_flush_head) && priv_p->p_prepare_flush_seq != (uint64_t)-1L) {
		/*
		 * Once prepare_flush_seq is set and flush head is empty,
		 * the right_cut ddn had been flushed already.
		 * So set last_seq_flushed to current pnp biggest flush sequence for update.
		 */
		priv_p->p_last_seq_flushed = pnp->pp_last_flush_seq;
		priv_p->p_prepare_flush_seq = -1L;
	} else if (seq_to_flush && pnp->pp_last_flush_seq < priv_p->p_prepare_flush_seq) {
		priv_p->p_last_seq_flushed = seq_to_flush;
	}

	list_for_each_entry(np, struct data_description_node, &to_flush_head, d_flist) {
		assert(np->d_flushing && np->d_flushed && !np->d_flush_done);
		pnp->pp_flushed += np->d_len;   // Not d_flush_len, always use d_len to calculate pp_flushed
		assert(pnp->pp_flushed <= page_sz);
		np->d_flush_done = 1;
		np->d_where = DDN_WHERE_RELEASE;
	}
	list_splice_tail_init(&to_flush_head, &pnp->pp_release_head);

	/*
	 * Release the page pnp to bre if the whole page got flushed
	 * Notice: the last page in the page list (p_flush_page_head) may not be fully occupied,
	 * i.e pp_occupied < page_sz, don't release this page since it is still being used by ds
	 */
	if (pnp->pp_flushed == page_sz && pnp->pp_occupied == page_sz) {
		assert(list_empty(&pnp->pp_flush_head));
		assert(pnp->pp_where == PP_WHERE_FLUSH);
		list_del(&pnp->pp_rlist);   // removed from p_flush_page_head
		assert(&pnp->pp_rlist != priv_p->p_flush_page_head.next);
		priv_p->p_flush_page_counter--;
		pthread_mutex_unlock(&priv_p->p_flush_lck);
		if (priv_p->p_ssd_mode) {
			bre_p->br_op->br_release_page_immediately(bre_p, pnp);
		} else {
			bre_p->br_op->br_release_page(bre_p, pnp);
		}
	} else {
		pthread_mutex_unlock(&priv_p->p_flush_lck);
	}

	if(fast_flush) {
		pthread_mutex_lock(&priv_p->p_flush_lck);
		goto do_flush;
	} else {
		goto next_try;
	}

out:
	priv_p->p_flush_stop = 1;
	return NULL;
}

int
bfes_initialization(struct bfe_interface *bfe_p, struct bfe_setup *setup)
{
	char *log_header = "bfe_initialization";
	struct bfe_private *priv_p;
	struct pp_interface *pp_p;
	pthread_attr_t attr_flush;
	pthread_t thread_id;
	mode_t mode;
	int flags;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	bfe_p->bf_op = &bfe_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bfe_p->bf_private = priv_p;
	pthread_mutex_init(&priv_p->p_flush_lck, NULL);
	pthread_cond_init(&priv_p->p_flush_cond, NULL);
	pthread_mutex_init(&priv_p->p_trim_lck, NULL);
	INIT_LIST_HEAD(&priv_p->p_flush_page_head);
	priv_p->p_allocator = setup->allocator;
	priv_p->p_pp = setup->pp;
	priv_p->p_bse = setup->bse;
	priv_p->p_bre = setup->bre;
	priv_p->p_wc = setup->wc;
	priv_p->p_rc = setup->rc;
	priv_p->p_rw = setup->rw;
	priv_p->p_rw_handle = setup->rw_handle;
	priv_p->p_ssd_mode = setup->ssd_mode;
	priv_p->p_prepare_flush_seq = -1L;
	priv_p->p_max_flush_size = ((uint64_t)setup->max_flush_size << 20);
	priv_p->p_wc_chain_handle = setup->wc_chain_handle;
	priv_p->p_rc_handle = setup->rc_handle;
	priv_p->p_do_fp = setup->do_fp;
	priv_p->p_target_sync = setup->target_sync;
	pp_p = priv_p->p_pp;
	priv_p->p_pagesz = pp_p->pp_op->pp_get_pagesz(pp_p);
	priv_p->p_vec_stat = 0;
	priv_p->p_coalesce_index = 0;

	if (setup->bufdevice) {
		posix_memalign((void **)&priv_p->p_flush_buffer, getpagesize(), priv_p->p_max_flush_size);
		priv_p->p_bufdevice = setup->bufdevice;
		mode = 0600;
		flags = O_RDONLY;
		priv_p->p_wc_fd = open(priv_p->p_bufdevice, flags, mode);
		if (priv_p->p_wc_fd < 0) {
			nid_log_error("%s: cannot open %s", log_header, priv_p->p_bufdevice);
			assert(0);
		}
	} else {
		priv_p->p_wc_fd = -1;
	}
	pthread_attr_init(&attr_flush);
	pthread_attr_setdetachstate(&attr_flush, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr_flush, _bfe_flush_thread, (void *)bfe_p);

	return 0;
}
