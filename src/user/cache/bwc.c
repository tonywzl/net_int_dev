/*
 * bwc.c
 * 	Implementation of Writeback Write Cache Module
 */

#include "bwc.h"

/*
 * Notice:
 * 	- Do remember to add the new version item (for the new BWC_DEV_HEADER_VER) to the table for a new release.
 * 	- ver_num must be a positive integer.
 * 	- Different versions with the same ver_num have compatible layout of write cache and vice versa.
 */
struct bwc_ver_cmp {
	uint8_t	ver_num;
	char	*ver_name;
};

static struct bwc_ver_cmp bwc_ver_cmp_table[] = {
	{ 1,	"35100000" },
//	{ 1,	"35200000" },
	{ -1,	NULL },
};

/*
 * Algorithm:
 * 	if the resource doesn't exist, get a non-used id
 * Return:
 * 	>0: an valid id associated with the resouce
 * 	0: cannot find an non-used id
 */
static uint16_t
__get_resource_id(struct bwc_private *priv_p, char *res_p)
{
	char *log_header = "__get_resource_id";
	int i;
	uint16_t new_id = 0;

	for (i = 1; i < BWC_MAX_RESOURCE; i++) {
		if (priv_p->p_resources[i] && !strcmp(priv_p->p_resources[i], res_p)) {
			return i;
		}
		if (!new_id && !priv_p->p_resources[i]) {
			new_id = i;
		}
	}

	if (new_id) {
		/* got a new resource */
		priv_p->p_resources[new_id] = x_malloc(strlen(res_p) + 1);
		if (!priv_p->p_resources[new_id] ) {
			nid_log_error("%s: x_malloc failed.", log_header);
		}

		strcpy(priv_p->p_resources[new_id], res_p);
		__bufdevice_update_header(priv_p);
	}
	return new_id;
}

static struct bwc_chain *
bwc_get_freeze_chain_byid(struct bwc_interface *bwc_p, char *res_uuid)
{
	char *log_header = "bwc_get_freeze_chain_byid";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	int got_it = 0;

	nid_log_warning("%s: start uuid:%s...", log_header, res_uuid);

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_frozen_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			got_it = 1;
			break;
		}
	}

	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_freezing_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			got_it = 1;
			break;
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	if(!got_it) {
		chain_p = NULL;
	}

	return chain_p;
}


/*
 * create a channel with owner, it should be active
 * create a channel w/o owner, it should be inactive
 */
static void *
bwc_create_channel(struct bwc_interface *bwc_p, void *owner, struct wc_channel_info *wc_info, char *res_p, int *new)
{
	char *log_header = "bwc_create_channel";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bwc_channel *chan_p = NULL;
	struct bwc_chain *chain_p = NULL;
	struct ddn_interface *ddn_p;
	struct ddn_setup ddn_setup;
	struct bse_interface *bse_p;
	struct bse_setup bse_setup;
	struct bfe_interface *bfe_p;
	struct bfe_setup bfe_setup;
	struct bre_interface *bre_p;
	struct bre_setup bre_setup;
	struct rw_interface *rw_p = (struct rw_interface *)wc_info->wc_rw;
	struct rc_interface *rc_p = (struct rc_interface *)wc_info->wc_rc;
	void *rc_handle = wc_info->wc_rc_handle;
	int got_it = 0;
	*new = 1;

	nid_log_notice("%s: exportname:%s", log_header, res_p);

	if (priv_p->p_pause && owner)
		goto out;

tryagain:

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_p)) {
			if (chain_p->c_busy == 1) {
				priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
				// The chain is creating channel, so wait and try again for new channel create.
				sleep(1);
				goto tryagain;
			}
			chain_p->c_busy = 1;
			*new = 0;
			priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
			goto out;
		}
	}

	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_p)) {
			if (chain_p->c_busy == 1) {
				priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
				// The chain is creating channel, so wait and try again for new channel create.
				sleep(1);
				goto tryagain;
			}
			chain_p->c_busy = 1;
			got_it = 1;
			break;
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	if (got_it && owner) {
		nid_log_notice("%s: resource:%s exists in this inactive list, make it active",
			log_header, res_p);
		assert(!chain_p->c_active);
		priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		if (chain_p->c_active == 0) {
			list_del(&chain_p->c_list);	// removed from p_inactive_head
			list_add_tail(&chain_p->c_list, &priv_p->p_chain_head);
			chain_p->c_active = 1;
		}
		priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		*new = 0;
	} else if (got_it) {
		assert(!chain_p->c_active);
		nid_log_notice("%s: resource:%s exists in ths inactive list, cannot active it w/o owner",
			log_header, res_p);
		*new = 0;
	}

out:
	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_sac = (struct sac_interface *)owner;
	INIT_LIST_HEAD(&chan_p->c_read_head);
	INIT_LIST_HEAD(&chan_p->c_write_head);
	INIT_LIST_HEAD(&chan_p->c_trim_head);
	INIT_LIST_HEAD(&chan_p->c_resp_head);

	if (*new) {
		chain_p = x_calloc(1, sizeof(*chain_p));
		chan_p->c_chain = chain_p;
		INIT_LIST_HEAD(&chain_p->c_chan_head);
		INIT_LIST_HEAD(&chain_p->c_search_head);
		pthread_mutex_init(&chain_p->c_rlck, NULL);
		pthread_mutex_init(&chain_p->c_wlck, NULL);
		pthread_mutex_init(&chain_p->c_trlck, NULL);
		pthread_mutex_init(&chain_p->c_lck, NULL);
		chain_p->c_bwc = bwc_p;
		chain_p->c_rw = rw_p;
		chain_p->c_rw_handle = rw_p->rw_op->rw_create_worker(rw_p, wc_info->wc_rw_exportname, wc_info->wc_rw_sync, wc_info->wc_rw_direct_io,
				wc_info->wc_rw_alignment, wc_info->wc_rw_dev_size);
		chain_p->c_target_sync = wc_info->wc_rw_sync;
		chain_p->c_rc = rc_p;
		chain_p->c_rc_handle = rc_handle;
		strncpy(chain_p->c_resource, res_p, NID_MAX_PATH - 1);
		chain_p->c_id = __get_resource_id(priv_p, chain_p->c_resource);
		if (!chain_p->c_id) {
			nid_log_notice("%s: out of id for resource (%s)",
				log_header, chain_p->c_resource);
			free(chain_p);
			return NULL;
		}

		// there have matched resorce uuid in freeze chain list, new chain must mark with freeze
		if(bwc_get_freeze_chain_byid(bwc_p, chain_p->c_resource)) {
			chain_p->c_freeze = 1;
		}

		ddn_p = &chain_p->c_ddn;
		ddn_setup.allocator = priv_p->p_allocator;
		ddn_setup.set_id = ALLOCATOR_SET_BWC_DDN;
		ddn_setup.seg_size = 1024;
		ddn_initialization(ddn_p, &ddn_setup);

		bse_p = &chain_p->c_bse;
		bse_setup.devsz = 1024;
		bse_setup.cachesz = 1024 * 1024;
		bse_setup.bfe = &chain_p->c_bfe;
		bse_setup.ddn = &chain_p->c_ddn;
		bse_setup.srn = priv_p->p_srn;
		bse_setup.pp = priv_p->p_pp;
		bse_setup.allocator = priv_p->p_allocator;
		bse_setup.bufdevice = priv_p->p_bufdevice;
		bse_initialization(bse_p, &bse_setup);

		bfe_p = &chain_p->c_bfe;
		bfe_setup.allocator = priv_p->p_allocator;
		bfe_setup.bse = &chain_p->c_bse;
		bfe_setup.bre = &chain_p->c_bre;
		bfe_setup.wc = priv_p->p_wc;
		bfe_setup.wc_chain_handle = (void *)chain_p;
		bfe_setup.pp = priv_p->p_pp;
		bfe_setup.rw = chain_p->c_rw;
		bfe_setup.rw_handle = chain_p->c_rw_handle;
		bfe_setup.rc = rc_p;
		bfe_setup.rc_handle = rc_handle;
		bfe_setup.rw_sync = priv_p->p_rw_sync;
		bfe_setup.target_sync = chain_p->c_target_sync;
		bfe_setup.do_fp = priv_p->p_do_fp;
		bfe_setup.lstn = priv_p->p_lstn;
		bfe_setup.ddn = &chain_p->c_ddn;
		bfe_setup.max_flush_size = priv_p->p_max_flush_size;
		bfe_setup.bufdevice = priv_p->p_bufdevice;
		bfe_setup.ssd_mode = priv_p->p_ssd_mode;
		bfe_initialization(bfe_p, &bfe_setup);

		bre_p = &chain_p->c_bre;
		bre_setup.pp = priv_p->p_pp;
		bre_setup.bse = &chain_p->c_bse;
		bre_setup.ddn = &chain_p->c_ddn;
		bre_setup.sequence_mode = priv_p->p_rw_sync == 0 && priv_p->p_ssd_mode == 1 ? 1 : 0;;
		bre_setup.wc = priv_p->p_wc;
		bre_setup.wc_chain_handle = (void*)chain_p;
		bre_initialization(bre_p, &bre_setup);
		priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		if (owner) {
			list_add_tail(&chain_p->c_list, &priv_p->p_chain_head);
			chain_p->c_active = 1;
		} else {
			list_add_tail(&chain_p->c_list, &priv_p->p_inactive_head);
			chain_p->c_active = 0;
		}
		list_add(&chan_p->c_list, &chain_p->c_chan_head);
		chain_p->c_busy = 0;
		priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	} else {
		chan_p->c_chain = chain_p;
		priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		list_add(&chan_p->c_list, &chain_p->c_chan_head);
		chain_p->c_busy = 0;
		priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	}

	return chan_p;
}

static void *
bwc_prepare_channel(struct bwc_interface *bwc_p, struct wc_channel_info *wc_info, char *res_p)
{
	char *log_header = "bwc_prepare_channel";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_channel *chan_p = NULL;
	int new_chan;

	nid_log_notice("%s: resource:%s", log_header, res_p);
	assert(priv_p);
	chan_p = bwc_create_channel(bwc_p, NULL, wc_info, res_p, &new_chan);
	assert(!chan_p || new_chan);
	return chan_p;
}

static void
bwc_recover(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_recover";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	off_t start_off = 0;
	uint64_t start_seq = 0, flushed_seq = 0, final_seq = 0, last_mdz_seq = 0;
	struct bwc_dev_trailer *dtp = priv_p->p_dtp;
	struct list_head recover_buf_head;
	int rc, scan_res;
	int hdz_rewind = 0;

	nid_log_warning("%s: start ...", log_header);

	INIT_LIST_HEAD(&recover_buf_head);
	memset(dtp, 0, sizeof(*dtp));
	nid_log_warning("%s: p_mdz_recover:%u, md_first_seg:%d",
			log_header, priv_p->p_mdz_recover, (int)mhp->md_first_seg);

	if (priv_p->p_mdz_recover) {
		__bwc_mdz_scan(priv_p, &start_seq, &start_off, &last_mdz_seq, &recover_buf_head, &hdz_rewind);
		scan_res = __bwc_hdz_scan(priv_p, last_mdz_seq, &flushed_seq, &final_seq, &recover_buf_head, hdz_rewind);
	} else {
		scan_res = __bwc_scan_bufdevice(priv_p, &start_off, &start_seq,
			&flushed_seq, &final_seq, dtp, &recover_buf_head);
	}

	if (!scan_res) {
		assert(flushed_seq <= final_seq);
		nid_log_warning("%s: start_seq:%lu, flushed_seq:%lu, final_seq:%lu",
			log_header, start_seq, flushed_seq, final_seq);
	} else {
		nid_log_warning("%s: __bwc_scan_bufdevice failed(%d)", log_header, scan_res);
	}


	if (!dtp->dt_offset)
		dtp = NULL;

	if (priv_p->p_mdz_recover) {
		rc = __bwc_bufdevice_restore2(bwc_p, start_seq, flushed_seq, final_seq, dtp, &recover_buf_head, last_mdz_seq);
	} else if (!scan_res && flushed_seq < final_seq) {
		rc = __bwc_bufdevice_restore(bwc_p, start_off, start_seq, flushed_seq, final_seq, dtp, &recover_buf_head);
	} else {
		nid_log_warning("%s: nothing to recover!, scan_res:%d, flushed_seq:%lu, final_seq:%lu",
			log_header, scan_res, flushed_seq, final_seq);
		priv_p->p_cur_offset = lseek(priv_p->p_fhandle, priv_p->p_first_seg_off, SEEK_SET);
		rc = -1;
	}

	if (!rc) {
		priv_p->p_seq_assigned = final_seq;
		priv_p->p_seq_flushed = flushed_seq;
		priv_p->p_seq_flush_aviable = priv_p->p_seq_assigned;
		nid_log_warning("%s: __bwc_bufdevice_recover success!, seq_flushed: %lu, seq_assigned: %lu",
			log_header, priv_p->p_seq_flushed, priv_p->p_seq_assigned);
	} else {
		nid_log_error("%s: __bwc_bufdevice_recover failed", log_header);
	}

	rbn_p->nn_op->nn_destroy(rbn_p);
	priv_p->p_pause = 0;

	nid_log_warning("%s: end ...", log_header);
}

static uint32_t
bwc_get_poolsz(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	return pp_p->pp_op->pp_get_poolsz(pp_p);
}


static uint32_t
bwc_get_pagesz(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	return priv_p->p_pagesz;
}
/*
static uint32_t
bwc_get_blocksz(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	return (priv_p->p_bufdevicesz >> 20) / 1024;
}

static uint32_t
bwc_get_page_availabe(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	return priv_p->p_page_avail;
}
*/
static uint16_t
bwc_get_block_occupied(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	return priv_p->p_noccupied;
}


static ssize_t
bwc_pread(struct bwc_interface *bwc_p, void *io_handle, void *buf, size_t count, off_t offset)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_channel *chan_p = (struct bwc_channel *)io_handle;
	struct rw_interface *rw_p;
	assert(priv_p);

	rw_p = chan_p->c_chain->c_rw;
	return rw_p->rw_op->rw_pread(rw_p, chan_p->c_chain->c_rw_handle, buf, count, offset);
}

static void
bwc_read_list(struct bwc_interface *bwc_p, void *io_handle, struct list_head *read_head, int read_counter)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_channel *chan_p = (struct bwc_channel *)io_handle;

	pthread_mutex_lock(&chan_p->c_chain->c_rlck);
	list_splice_tail_init(read_head, &chan_p->c_read_head);
	chan_p->c_rd_recv_counter += read_counter;
	pthread_mutex_unlock(&chan_p->c_chain->c_rlck);
	pthread_mutex_lock(&priv_p->p_new_read_lck);
	if (!priv_p->p_new_read_task) {
		priv_p->p_new_read_task = 1;
		pthread_cond_signal(&priv_p->p_new_read_cond);
	}
	pthread_mutex_unlock(&priv_p->p_new_read_lck);
}

static void
bwc_write_list(struct bwc_interface *bwc_p, void *wc_handle, struct list_head *write_head, int write_counter)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_channel *chan_p = (struct bwc_channel *)wc_handle;

	pthread_mutex_lock(&chan_p->c_chain->c_wlck);
	list_splice_tail_init(write_head, &chan_p->c_write_head);
	chan_p->c_wr_recv_counter += write_counter;
	pthread_mutex_unlock(&chan_p->c_chain->c_wlck);

	pthread_mutex_lock(&priv_p->p_new_write_lck);
	if (!priv_p->p_new_write_task) {
		priv_p->p_new_write_task = 1;
		pthread_cond_signal(&priv_p->p_new_write_cond);
	}
	pthread_mutex_unlock(&priv_p->p_new_write_lck);
}

static void
bwc_trim_list(struct bwc_interface *bwc_p, void *wc_handle, struct list_head *trim_head, int trim_counter)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_channel *chan_p = (struct bwc_channel *)wc_handle;

	pthread_mutex_lock(&chan_p->c_chain->c_trlck);
	list_splice_tail_init(trim_head, &chan_p->c_trim_head);
	chan_p->c_tr_recv_counter += trim_counter;
	pthread_mutex_unlock(&chan_p->c_chain->c_trlck);

	pthread_mutex_lock(&priv_p->p_new_trim_lck);
	if (!priv_p->p_new_trim_task) {
		priv_p->p_new_trim_task = 1;
		pthread_cond_signal(&priv_p->p_new_trim_cond);
	}
	pthread_mutex_unlock(&priv_p->p_new_trim_lck);
}

static int
bwc_chan_inactive(struct bwc_interface *bwc_p, void *wc_handle)
{
	char *log_header = "bwc_chan_inactive";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_channel *chan_p = (struct bwc_channel *)wc_handle;
	struct bwc_chain *chain_p = chan_p->c_chain;

	nid_log_notice("%s: start (%s)...", log_header, chain_p->c_resource);

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	if(chain_p->c_freeze || chain_p->c_busy) {
		chan_p->c_sac = NULL;
		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		nid_log_notice("%s: chain  (%s) in freeze state", log_header, chain_p->c_resource);
		return 0;
	}

	priv_p->p_lck_if.l_op->l_uplock(&priv_p->p_lck_if, &priv_p->p_lnode);
	assert(list_empty(&chan_p->c_read_head));
	list_del(&chan_p->c_list);
	if (list_empty(&chain_p->c_chan_head) && chain_p->c_busy == 0) {
		if (chain_p->c_active) {
			list_del(&chain_p->c_list);	// removed from p_chan_head
			chain_p->c_active = 0;
			list_add_tail(&chain_p->c_list, &priv_p->p_inactive_head);
		}
	}
	priv_p->p_lck_if.l_op->l_downlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	free(chan_p);
	return 0;
}

static int
bwc_stop(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_stop";
	struct bwc_private *priv_p = bwc_p->bw_private;

	nid_log_notice("%s: start (p_seq_flushed:%lu, p_seq_assigned:%lu) ...",
		log_header, priv_p->p_seq_flushed, priv_p->p_seq_assigned);

	priv_p->p_stop = 1;
	pthread_mutex_lock(&priv_p->p_new_read_lck);
	pthread_cond_signal(&priv_p->p_new_read_cond);
	pthread_mutex_unlock(&priv_p->p_new_read_lck);

	// bwc_write_buf_thread may wait p_new_write_cond/p_snapshot_bdev_cond, wake thread up
	pthread_mutex_lock(&priv_p->p_new_write_lck);
	pthread_cond_signal(&priv_p->p_new_write_cond);
	pthread_mutex_unlock(&priv_p->p_new_write_lck);

	// bwc_trim_thread may wait for trim command, wake it up
	pthread_mutex_lock(&priv_p->p_new_trim_lck);
	pthread_cond_signal(&priv_p->p_new_trim_cond);
	pthread_mutex_unlock(&priv_p->p_new_trim_lck);

	pthread_mutex_lock(&priv_p->p_snapshot_lck);
	pthread_cond_signal(&priv_p->p_snapshot_cond);
	pthread_mutex_unlock(&priv_p->p_snapshot_lck);

	// _bwc_release_thread may wait p_seq_cond/p_snapshot_cond, wake thread up
	pthread_mutex_lock(&priv_p->p_seq_lck);
	pthread_cond_signal(&priv_p->p_seq_cond);
	pthread_mutex_unlock(&priv_p->p_seq_lck);

	pthread_mutex_lock(&priv_p->p_occ_lck);
	pthread_cond_broadcast(&priv_p->p_occ_cond);
	pthread_mutex_unlock(&priv_p->p_occ_lck);

	pthread_mutex_lock(&priv_p->p_read_finish_lck);
	pthread_cond_broadcast(&priv_p->p_read_post_cond);
	pthread_mutex_unlock(&priv_p->p_read_finish_lck);

	while (!priv_p->p_read_stop || !priv_p->p_write_stop || !priv_p->p_trim_stop ||
			!priv_p->p_post_read_stop || !priv_p->p_release_stop) {
		nid_log_notice("%s: waiting to stop (_read_stop:%d, write_stop:%d, trim_stop:%d, release_stop:%d ...",
			log_header, priv_p->p_read_stop, priv_p->p_write_stop, priv_p->p_trim_stop, priv_p->p_release_stop);
		sleep(1);
	}

	return 0;
}

static void
bwc_start_page(struct bwc_interface *bwc_p, void *io_handle, void *page_p, int do_buffer)
{
	(void)bwc_p;
	struct bwc_channel *chan_p = (struct bwc_channel *)io_handle;
	__bwc_start_page(chan_p->c_chain, page_p, do_buffer);
}

static void
bwc_end_page(struct bwc_interface *bwc_p, void *io_handle, void *page_p, int do_buffer)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_channel *chan_p = (struct bwc_channel *)io_handle;
	struct bfe_interface *bfe_p = &chan_p->c_chain->c_bfe;
	struct bre_interface *bre_p = &chan_p->c_chain->c_bre;
	struct pp_page_node *pnp = (struct pp_page_node *)page_p;

	assert(priv_p);
	if (do_buffer) {
		bfe_p->bf_op->bf_end_page(bfe_p, pnp);
	} else {
		/*
		 * this page is used as a non buffer page.
		 * pretend the page already flushed, so bypass flushing proceedings
		 */
		pnp->pp_flushed = pnp->pp_occupied;
		bre_p->br_op->br_quick_release_page(bre_p, page_p);
	}
}

static void
bwc_flush_update(struct bwc_interface *bwc_p, void *io_handle, uint64_t seq)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p = (struct bwc_chain *)io_handle;
	uint64_t min_seq = 0;
	uint8_t force_wakeup_seq = 0;

	assert(chain_p->c_seq_flushed <= seq);
	chain_p->c_seq_flushed = seq;

	/*
	 * when most of write traffic lie in freezing chain, other chain will only
	 * update seq flushed in bwc at big interval , it may lead to little chance
	 * to wake up freeze thread which wait p_seq_cond signal, which will slow down
	 * flush to target action in freeze action because bfe trigger flush to target
	 * by bwc too slow, here trigger force wakeup to speed up flush to target speed
	 * in this situation
	 */
	if(chain_p->c_freeze) {
		force_wakeup_seq = 1;
	}

	/*
	 * at the initial stage, cloned chain with all seq 0, should not count it
	 * otherwise min_seq may always stay in 0
	 */
	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if(chain_p->c_freeze) continue;

		if (!min_seq)
			min_seq = chain_p->c_seq_flushed;
		else if (min_seq > chain_p->c_seq_flushed)
			min_seq = chain_p->c_seq_flushed;
	}

	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
		if(chain_p->c_freeze) continue;

		if (!min_seq)
			min_seq = chain_p->c_seq_flushed;
		else if (chain_p->c_seq_flushed && min_seq > chain_p->c_seq_flushed)
			min_seq = chain_p->c_seq_flushed;
	}

	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_freezing_chain_head, c_list) {
		if (!min_seq)
			min_seq = chain_p->c_seq_flushed;
		else if (chain_p->c_seq_flushed && min_seq > chain_p->c_seq_flushed)
			min_seq = chain_p->c_seq_flushed;
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	pthread_mutex_lock(&priv_p->p_seq_lck);
	if (min_seq > priv_p->p_seq_flushed) {
		priv_p->p_seq_flushed = min_seq;
		pthread_cond_broadcast(&priv_p->p_seq_cond);
	} else if(force_wakeup_seq) {
		pthread_cond_broadcast(&priv_p->p_seq_cond);
	}
	pthread_mutex_unlock(&priv_p->p_seq_lck);

	if (priv_p->p_seq_flushed > priv_p->p_rec_seq_flushed + 10) {
		pthread_mutex_lock(&priv_p->p_new_write_lck);
		if (!priv_p->p_new_write_task) {
			priv_p->p_new_write_task = 1;
			pthread_cond_signal(&priv_p->p_new_write_cond);
		}
		pthread_mutex_unlock(&priv_p->p_new_write_lck);
	}
}

static void
bwc_get_chan_stat(struct bwc_interface *bwc_p, void *io_handle, struct io_chan_stat *sp)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	if (list_empty(&priv_p->p_chain_head)) return;
	struct bwc_channel *chan_p = (struct bwc_channel *)io_handle;
	struct bse_interface *bse_p = &chan_p->c_chain->c_bse;
	assert(priv_p);
	bse_p->bs_op->bs_get_chan_stat(bse_p, sp);
}


static void
bwc_get_vec_stat(struct bwc_interface *bwc_p, void *io_handle, struct io_vec_stat *sp)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_channel *chan_p = (struct bwc_channel *)io_handle;
	struct bfe_interface *bfe_p;
	assert(priv_p);

	bfe_p = &chan_p->c_chain->c_bfe;
	bfe_p->bf_op->bf_get_stat(bfe_p, sp);
}

static void
bwc_get_vec_stat_by_uuid(struct bwc_interface *bwc_p, char* chan_uuid, struct io_vec_stat *sp)
{
	char *log_header = "bwc_get_vec_stat_by_uuid";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bfe_interface *bfe_p;
	struct bwc_chain *chain_p;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, chan_uuid)) {
			bfe_p = &chain_p->c_bfe;
			assert(bfe_p);
			nid_log_debug("%s: get bfe of %s", log_header, chan_uuid);
			bfe_p->bf_op->bf_get_stat(bfe_p, sp);
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
}

static void *
bwc_get_all_vec_stat(struct bwc_interface *bwc_p, int *num_bfe)
{
	char *log_header = "bwc_get_all_vec_stat";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bfe_interface *bfe_p;
	struct bwc_chain *chain_p;
	struct io_vec_stat *ret_sp, *new;
	int bfe_counter = 0;

	nid_log_debug("%s: start ...", log_header);
	ret_sp = x_calloc(1, sizeof(*ret_sp));
	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		bfe_p = &chain_p->c_bfe;
		assert(bfe_p);
		bfe_p->bf_op->bf_get_stat(bfe_p, &ret_sp[bfe_counter]);
		ret_sp[bfe_counter].s_chan_uuid = chain_p->c_resource;
		bfe_counter++;
		new = realloc(ret_sp, (bfe_counter + 1) * sizeof(*ret_sp));
		if (!new){
			nid_log_warning("%s: cannot realloc", log_header);
			break;
		}
		ret_sp = new;
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	*num_bfe = bfe_counter;

	return ret_sp;
}

static char *
bwc_get_uuid(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	return priv_p->p_uuid;
}

static void
bwc_get_stat(struct bwc_interface *bwc_p, struct io_stat *sp)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	struct smc_stat_record *sr_p;
	struct sac_stat *stat_p;
	struct pp_interface *pp_p = priv_p->p_pp;
	int32_t page_available,  pool_sz, pool_len, pool_nfree, rel_page_counter = 0;
	pool_sz = pp_p->pp_op->pp_get_poolsz(pp_p);
	rel_page_counter = bwc_p->bw_op->bw_get_release_page_counter(bwc_p);
	pool_nfree = pp_p->pp_op->pp_get_nfree(pp_p);
	pool_len = pp_p->pp_op->pp_get_poollen(pp_p);
	page_available = pool_nfree + pool_sz - pool_len + rel_page_counter;
	priv_p->p_page_avail = page_available;

	sp->s_block_occupied = priv_p->p_noccupied;
	sp->s_flush_nblocks = priv_p->p_flush_nblocks;
	sp->s_cur_write_block = priv_p->p_cur_block;
	sp->s_cur_flush_block = priv_p->p_cur_flush_block;
	sp->s_seq_assigned = priv_p->p_seq_assigned;
	sp->s_seq_flushed = priv_p->p_seq_flushed;
	sp->s_rec_seq_flushed = priv_p->p_rec_seq_flushed;
	sp->s_buf_avail	= priv_p->p_page_avail;
	INIT_LIST_HEAD(&sp->s_inactive_head);

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
		stat_p = x_calloc(1, sizeof(*stat_p));
		sr_p = x_calloc(1, sizeof(*sr_p));
		sr_p->r_type = NID_REQ_STAT_SAC;
		sr_p->r_data = (void *)stat_p;
		stat_p->sa_io_type = IO_TYPE_BUFFER;
		stat_p->sa_uuid = chain_p->c_resource;
		stat_p->sa_stat = NID_STAT_INACTIVE;
		list_add(&sr_p->r_list, &sp->s_inactive_head);
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	if (!priv_p->p_pause)
		sp->s_stat = NID_STAT_ACTIVE;
	else
		sp->s_stat = NID_STAT_INACTIVE;
}

static void
bwc_get_info_stat(struct bwc_interface *bwc_p, struct umessage_bwc_information_resp_stat *stat_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	stat_p->um_seq_assigned = priv_p->p_seq_assigned;
	stat_p->um_seq_flushed = priv_p->p_seq_flushed;
	stat_p->um_block_occupied = priv_p->p_noccupied;
	stat_p->um_flush_nblocks = priv_p->p_flush_nblocks;
	stat_p->um_cur_write_block = priv_p->p_cur_block;
	stat_p->um_cur_flush_block = priv_p->p_cur_flush_block;
	stat_p->um_resp_counter = priv_p->p_resp_counter;
	stat_p->um_rec_seq_flushed = priv_p->p_rec_seq_flushed;
	stat_p->um_buf_avail = priv_p->p_page_avail;
	if (!priv_p->p_pause)
		stat_p->um_state = NID_STAT_ACTIVE;
	else
		stat_p->um_state = NID_STAT_INACTIVE;
}

static int
bwc_vec_start(struct bwc_interface *bwc_p, char *chan_uuid)
{
	char *log_header = "bwc_vec_start";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bfe_interface *bfe_p;
	struct bwc_chain *chain_p;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if (!chan_uuid) {
			bfe_p = &chain_p->c_bfe;
			assert(bfe_p);
			nid_log_debug("%s: find channel (%s)", log_header, chain_p->c_resource);
			bfe_p->bf_op->bf_vec_start(bfe_p);
		} else if (!strcmp(chain_p->c_resource, chan_uuid)) {
			bfe_p = &chain_p->c_bfe;
			assert(bfe_p);
			nid_log_debug("%s: find channel (%s)", log_header, chain_p->c_resource);
			bfe_p->bf_op->bf_vec_start(bfe_p);
			break;
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	return 0;
}

static int
bwc_vec_stop(struct bwc_interface *bwc_p, char *chan_uuid)
{
	char *log_header = "bwc_vec_stop";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bfe_interface *bfe_p;
	struct bwc_chain *chain_p;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if (!chan_uuid) {
			bfe_p = &chain_p->c_bfe;
			assert(bfe_p);
			nid_log_debug("%s: find channel (%s)", log_header, chain_p->c_resource);
			bfe_p->bf_op->bf_vec_stop(bfe_p);
		} else if (!strcmp(chain_p->c_resource, chan_uuid)) {
			bfe_p = &chain_p->c_bfe;
			assert(bfe_p);
			nid_log_debug("%s: find channel (%s)", log_header, chain_p->c_resource);
			bfe_p->bf_op->bf_vec_stop(bfe_p);
			break;
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	return 0;
}

static uint32_t
bwc_get_release_page_counter(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	struct bre_interface *bre_p;
	uint32_t counter = 0;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		bre_p = &chain_p->c_bre;
		counter += bre_p->br_op->br_get_page_counter(bre_p);
	}

	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
		bre_p = &chain_p->c_bre;
		counter += bre_p->br_op->br_get_page_counter(bre_p);
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	return counter;
}

static uint32_t
bwc_get_flush_page_counter(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	struct bfe_interface *bfe_p;
	uint32_t counter = 0;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		bfe_p = &chain_p->c_bfe;
		counter += bfe_p->bf_op->bf_get_page_counter(bfe_p);
	}

	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
		bfe_p = &chain_p->c_bfe;
		counter += bfe_p->bf_op->bf_get_page_counter(bfe_p);
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	return counter;
}

static int
bwc_dropcache_start(struct bwc_interface *bwc_p, char *res_uuid, int do_sync)
{
	char *log_header = "bwc_dropcache_start";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	int rc = 1;

	nid_log_warning("%s: start (uuid:%s) ...", log_header, res_uuid);
	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		nid_log_warning("%s: comapring %s with %s", chain_p->c_resource, res_uuid, log_header);
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			chain_p->c_do_dropcache = 1;
			rc = 0;
			break;
		}
	}

	if (rc) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
			nid_log_warning("%s: comapring %s with %s", chain_p->c_resource, res_uuid, log_header);
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				chain_p->c_do_dropcache = 1;
				rc = 0;
				break;
			}
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	if (!rc) {
		struct bre_interface *bre_p;
		bre_p = &chain_p->c_bre;
		bre_p->br_op->br_dropcache_start(bre_p, do_sync);
	}

	return rc;
}

static int
bwc_dropcache_stop(struct bwc_interface *bwc_p, char *res_uuid)
{
	char *log_header = "bwc_dropcache_stop";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	int rc = 1;

	nid_log_warning("%s: start (uuid:%s) ...", log_header, res_uuid);
	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		nid_log_warning("%s: comapring %s with %s", log_header, chain_p->c_resource, res_uuid);
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			chain_p->c_do_dropcache = 0;
			rc = 0;
			break;
		}
	}

	if (rc) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
			nid_log_warning("%s: comapring %s with %s", log_header, chain_p->c_resource, res_uuid);
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				chain_p->c_do_dropcache = 0;
				rc = 0;
				break;
			}
		}
	}
	if (!rc) {
		struct bre_interface *bre_p;
		bre_p = &chain_p->c_bre;
		bre_p->br_op->br_dropcache_stop(bre_p);
	} else {
		nid_log_warning("%s: not matched %s", log_header, res_uuid);
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	return rc;
}

static void
bwc_get_throughput(struct bwc_interface *bwc_p, struct bwc_throughput_stat *stat_p)
{

	struct bwc_private *priv_p = bwc_p->bw_private;
	stat_p->data_flushed = priv_p->p_data_flushed;
	stat_p->data_pkg_flushed = priv_p->p_data_pkg_flushed;
	stat_p->nondata_pkg_flushed = priv_p->p_nondata_pkg_flushed;
}

static void
bwc_get_rw(struct bwc_interface *bwc_p, char *res_uuid, struct bwc_rw_stat *stat_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bfe_rw_stat info;

	char *log_header = "bwc_get_rw";
	struct bwc_chain *chain_p;
	int rc = 1;

	nid_log_warning("%s: start (uuid:%s) ...", log_header, res_uuid);
	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		nid_log_warning("%s: comapring %s with %s", chain_p->c_resource, res_uuid, log_header);
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			rc = 0;
			break;
		}
	}

	if (rc) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
			nid_log_warning("%s: comapring %s with %s", chain_p->c_resource, res_uuid, log_header);
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				rc = 0;
				break;
			}
		}
	}


	if (!rc) {
		struct bfe_interface *bfe_p;
		bfe_p= &chain_p->c_bfe;
		bfe_p->bf_op->bf_get_rw_stat(bfe_p,&info);
		stat_p->flush_page= info.flush_page_num;
		stat_p->overwritten_num = info.overwritten_num;
		stat_p->overwritten_back_num = info.overwritten_back_num;
		stat_p->coalesce_num = info.coalesce_flush_num;
		stat_p->coalesce_back_num = info.coalesce_flush_back_num;
		stat_p->flush_num = info.flush_num;
		stat_p->flush_back_num = info.flush_back_num;
		stat_p->not_ready_num = info.not_ready_num;
		stat_p->res = 1;
		nid_log_warning("%s: overwritten:%lu, overwritten_back:%lu, coalesce:%lu, coalesce_back:%lu, flush:%lu, flush_back:%lu, flush_page:%lu, not_ready:%lu",
						log_header, info.overwritten_num, info.overwritten_back_num, info.coalesce_flush_num, info.coalesce_flush_back_num,
						info.flush_num, info.flush_back_num, info.flush_page_num, info.not_ready_num);

	} else {
		stat_p->res = 0;
	}

	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
}

static void
bwc_get_delay(struct bwc_interface *bwc_p, struct bwc_delay_stat *stat_p)
{
	char *log_header = "bwc_update_delay_level";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	uint16_t block_left = priv_p->p_flush_nblocks - priv_p->p_noccupied;

	nid_log_info("%s: start ...", log_header);

	stat_p->write_delay_first_level = priv_p->p_write_delay_first_level;
	stat_p->write_delay_first_level_increase_interval = priv_p->p_write_delay_first_level_increase_interval;
	stat_p->write_delay_second_level = priv_p->p_write_delay_second_level;
	stat_p->write_delay_second_level_increase_interval = priv_p->p_write_delay_second_level_increase_interval;
	stat_p->write_delay_second_level_start_ms = priv_p->p_write_delay_second_level_start_ms;
	stat_p->write_delay_time = _bwc_calculate_sleep_time(priv_p, block_left);
}

static void
bwc_reset_throughput(struct bwc_interface *bwc_p)
{
	struct bwc_private *priv_p = bwc_p->bw_private;
	priv_p->p_data_flushed = 0;
	priv_p->p_data_pkg_flushed = 0;
	priv_p->p_nondata_pkg_flushed = 0;
}

static	void
bwc_reset_read_counter(struct bwc_interface *bwc_p) {
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	__sync_lock_test_and_set(&priv_p->p_read_counter, 0);
}

static int64_t
bwc_get_read_counter(struct bwc_interface *bwc_p) {
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	return priv_p->p_read_counter;
}

static void
__bwc_read_callback(int errorcode, struct rw_callback_arg *arg)
{
	struct d2b_node *ran;
	struct bwc_private *priv_p;
	struct rw_alignment_read_arg *ran_align;
	struct bwc_channel *chan_p;
	struct rw_interface *rw_p;
	struct request_node *rn_p;
	struct d2a_node *rn;
	struct sub_request_node *sreq_p;
	int do_fp;

	nid_log_notice("__bwc_read_callback: callback ...");

	if (arg->ag_type == RW_CB_TYPE_ALIGNMENT) {
		ran_align = (struct rw_alignment_read_arg *) arg;
		ran = (struct d2b_node *)ran_align->arg_packaged;
		ran->d2_flag = 1;
	} else {
		ran = (struct d2b_node *)arg;
		ran->d2_flag = 0;
	}
	rn = (struct d2a_node *)ran->d2_data[BWC_RDNODE_INDEX];
	priv_p = (struct bwc_private *)rn->d2_data[BWC_PRIV_INDEX];
	ran->d2_ret = errorcode;
	if (errorcode != 0) {
		rn = (struct d2a_node *)ran->d2_data[BWC_RDNODE_INDEX];
		priv_p = (struct bwc_private *)rn->d2_data[BWC_PRIV_INDEX];
		if (ran->d2_retry_time++ < priv_p->p_read_max_retry_time) {
			nid_log_warning("Warning: Read failed. Error code: %d, retrying: %d", errorcode, ran->d2_retry_time);
			/* Slow down retry read */
			usleep(100000);
			rn_p = (struct request_node *)rn->d2_data[BWC_REQNODE_INDEX];
			sreq_p = ran->d2_data[BWC_SREQNODE_INDEX];
			chan_p = (struct bwc_channel *)rn_p->r_io_handle;
			rw_p = chan_p->c_chain->c_rw;
			do_fp = priv_p->p_do_fp;
			if (do_fp) {
				rw_p->rw_op->rw_pread_async_fp(rw_p, chan_p->c_chain->c_rw_handle,
					sreq_p->sr_buf, (size_t)sreq_p->sr_len, sreq_p->sr_offset,
					__bwc_read_callback, &ran->d2_arg);
			} else {
				rw_p->rw_op->rw_pread_async(rw_p, chan_p->c_chain->c_rw_handle,
						sreq_p->sr_buf, (size_t)sreq_p->sr_len, sreq_p->sr_offset,
						__bwc_read_callback, &ran->d2_arg);
			}
			return;
		} else {
			nid_log_error("Error: Read failed. Error code: %d. ", errorcode);
		}
	}

	pthread_mutex_lock(&priv_p->p_read_finish_lck);
	list_add_tail(&ran->d2_list, &priv_p->p_read_finished_head);
	pthread_cond_broadcast(&priv_p->p_read_post_cond);
	pthread_mutex_unlock(&priv_p->p_read_finish_lck);
}

static void
__bwc_do_read(struct tp_jobheader *job)
{
	char *log_header = "__bwc_do_read";
	struct request_node *rn_p = (struct request_node *)job;
	struct bwc_interface *bwc_p = (struct bwc_interface *)rn_p->r_io_obj;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct bwc_channel *chan_p = (struct bwc_channel *)rn_p->r_io_handle;
	struct rc_interface *rc_p = chan_p->c_chain->c_rc;
	void *rc_handle_p = chan_p->c_chain->c_rc_handle;
	struct rw_interface *rw_p = chan_p->c_chain->c_rw;
	struct sac_interface *sac_p = chan_p->c_sac;
	struct bse_interface *bse_p = &chan_p->c_chain->c_bse;
	struct fpn_interface *fpn_p = rw_p->rw_op->rw_get_fpn_p(rw_p);
	struct nid_request *ir_p = &rn_p->r_ir;
	struct sub_request_node *sreq_p, *sreq_p2;
	struct list_head fp_head;
	void *align_buf;
	int do_fp = priv_p->p_do_fp;
	size_t align_count;
	off_t align_offset;
	ssize_t ret;

	__sync_add_and_fetch(&priv_p->p_read_busy, 1);
	assert(ir_p->cmd == NID_REQ_READ);
	bse_p->bs_op->bs_search(bse_p, rn_p);

	/*
	 * r_head contains all blocks which are not available from bse.
	 */
	if (list_empty(&rn_p->r_head)) {
		/*
		 * we found everything from the wc. Just do response
		 */
		goto response;
	}

	/*
	 * Now, we have someting not found from wc. Try rc
	 */
	if (rc_p) {
		struct list_head res_head;
		INIT_LIST_HEAD(&res_head);
		list_for_each_entry_safe(sreq_p, sreq_p2, struct sub_request_node, &rn_p->r_head, sr_list) {
			rc_p->rc_op->rc_search(rc_p, rc_handle_p, sreq_p, &res_head);
			list_del(&sreq_p->sr_list);
			srn_p->sr_op->sr_put_node(srn_p, sreq_p);
		}
		assert(list_empty(&rn_p->r_head));
		if (!list_empty(&res_head))
			list_splice_tail_init(&res_head, &rn_p->r_head);
	}

	/*
	 * Now, r_head contains all segments which are not found from wc nor from rc
	 * We have to read them through rw
	 */
	INIT_LIST_HEAD(&fp_head);
	list_for_each_entry_safe(sreq_p, sreq_p2, struct sub_request_node, &rn_p->r_head, sr_list) {

		/*
		 * Update read cache
		 */
		if (rc_p && do_fp) {
			ret = rw_p->rw_op->rw_pread_fp(rw_p, chan_p->c_chain->c_rw_handle, sreq_p->sr_buf, sreq_p->sr_len,
				sreq_p->sr_offset, &fp_head, &align_buf, &align_count, &align_offset);

			if (ret < 0) {
				nid_log_error("%s: read failed: %ld.", log_header, ret);
				rn_p->r_ir.code = (char)ret;
				if (align_buf) free(align_buf);
			} else {
				if (align_buf) {
					/*Will not do read cache update when not alignment.*/
					free(align_buf);
				} else {
					rc_p->rc_op->rc_update(rc_p, rc_handle_p, sreq_p->sr_buf,
						sreq_p->sr_len, sreq_p->sr_offset, &fp_head);
				}
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

			ret = rw_p->rw_op->rw_pread(rw_p, chan_p->c_chain->c_rw_handle, sreq_p->sr_buf,
				sreq_p->sr_len, sreq_p->sr_offset);
			if (ret < 0) {
				nid_log_error("%s: read failed: %ld.", log_header, ret);
				rn_p->r_ir.code = (char)ret;
			}
		}

		// update read data size counter
		__sync_add_and_fetch(&priv_p->p_read_counter, 1);
		__sync_add_and_fetch(&chan_p->c_rd_done_counter, 1);

		list_del(&sreq_p->sr_list);
		srn_p->sr_op->sr_put_node(srn_p, sreq_p);
	}

	__sync_sub_and_fetch(&priv_p->p_read_busy, 1);
response:
	sac_p->sa_op->sa_add_response_node(sac_p, rn_p);
}

static void
__bwc_read_callback_twosteps(int errcode, struct rc_wc_cb_node *arg)
{
	struct request_node *rn_p = arg->rm_rn_p;
	struct bwc_channel *chan_p = (struct bwc_channel *)rn_p->r_io_handle;
	struct sub_request_node *sreq_p = arg->rm_sreq_p;
	struct bwc_interface *bwc_p = (struct bwc_interface *)rn_p->r_io_obj;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct sac_interface *sac_p = chan_p->c_sac;
	struct rc_wc_cbn_interface *p_rc_wc_cbn = &priv_p->p_rc_wc_cbn;
	struct rc_wc_node *rc_wc_node = arg->rm_wc_node;
	struct rc_wcn_interface *p_rc_wcn = &priv_p->p_rc_wcn;
	int cnt;

	/* Read error process. */
	if (errcode) {
		rn_p->r_ir.code = errcode;
	}
	/* Free sub request */
	nid_log_debug("__bwc_do_read_async_twosteps: TWO_STEP_READ30 off:%ld length:%u, recv:%lu resp:%lu"
			, sreq_p->sr_offset, sreq_p->sr_len, chan_p->c_rd_recv_counter, chan_p->c_rd_done_counter);
	srn_p->sr_op->sr_put_node(srn_p, sreq_p);
	cnt = __sync_sub_and_fetch(&rc_wc_node->rm_sub_req_counter, 1);
	nid_log_debug("__bwc_do_read_async_twosteps: TWO_STEP_READ31 cnt:%d", cnt);
	if (cnt == 0) {
		/* If all sub request comes back.
		 * Send response and free rc_wc_node.*/

		__sync_add_and_fetch(&priv_p->p_read_counter, 1);
		__sync_add_and_fetch(&chan_p->c_rd_done_counter, 1);

		nid_log_debug("__bwc_do_read_async_twosteps: TWO_STEP_READ32, recv:%lu resp:%lu", chan_p->c_rd_recv_counter, chan_p->c_rd_done_counter);
		sac_p->sa_op->sa_add_response_node(sac_p, rn_p);
		p_rc_wcn->rm_op->rm_put_node(p_rc_wcn, rc_wc_node);
	}
	/* Free rc_wc callback node*/
	p_rc_wc_cbn->rm_op->rm_put_node(p_rc_wc_cbn, arg);

}

static void
__bwc_do_read_async_twosteps(struct tp_jobheader *job)
{
	char* log_header = "__bwc_do_read_async_twosteps";
	struct request_node *rn_p = (struct request_node *)job;
	struct bwc_interface *bwc_p = (struct bwc_interface *)rn_p->r_io_obj;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bwc_channel *chan_p = (struct bwc_channel *)rn_p->r_io_handle;
	struct sac_interface *sac_p = chan_p->c_sac;
	struct rc_interface *rc_p = chan_p->c_chain->c_rc;
	void *rc_handle_p = chan_p->c_chain->c_rc_handle;
	struct rw_interface *rw_p = chan_p->c_chain->c_rw;
	void *rw_handle = chan_p->c_chain->c_rw_handle;
	struct bse_interface *bse_p = &chan_p->c_chain->c_bse;
	struct nid_request *ir_p = &rn_p->r_ir;
	struct sub_request_node *sreq_p, *sreq_p2;
	struct rc_wc_cbn_interface *p_rc_wc_cbn = &priv_p->p_rc_wc_cbn;
	struct rc_wcn_interface *p_rc_wcn = &priv_p->p_rc_wcn;
	struct rc_wc_cb_node *arg;
	struct rc_wc_node *rc_wc_node;
	int cnt;

	nid_log_debug("%s: start ...", log_header);
	assert(rw_p->rw_op->rw_get_type(rw_p) == RW_TYPE_MSERVER);
	assert(ir_p->cmd == NID_REQ_READ);

	bse_p->bs_op->bs_search(bse_p, rn_p);

	/*
	 * r_head contains all blocks which are not available from bse.
	 */
	if (!list_empty(&rn_p->r_head)) {
		assert(rc_p);
		rc_wc_node = p_rc_wcn->rm_op->rm_get_node(p_rc_wcn);
		rc_wc_node->rm_sub_req_counter = 1;
		list_for_each_entry_safe(sreq_p, sreq_p2, struct sub_request_node, &rn_p->r_head, sr_list) {
			arg = p_rc_wc_cbn->rm_op->rm_get_node(p_rc_wc_cbn);
			/* Add sub request counter */
			__sync_add_and_fetch(&rc_wc_node->rm_sub_req_counter, 1);
			arg->rm_wc_node = rc_wc_node;
			arg->rm_rn_p = rn_p;
			arg->rm_sreq_p = sreq_p;
			arg->rm_rw_handle = rw_handle;
			/* Do search and fetch for not found offsets from read cache and MDS*/
			nid_log_debug("__bwc_do_read_async_twosteps: TWO_STEP_READ2 off:%ld length:%u, recv:%lu resp:%lu",
					sreq_p->sr_offset, sreq_p->sr_len, chan_p->c_rd_recv_counter, chan_p->c_rd_done_counter);
			list_del(&sreq_p->sr_list);
			rc_p->rc_op->rc_search_fetch(rc_p, rc_handle_p, sreq_p, __bwc_read_callback_twosteps, arg);
		}
		cnt = __sync_sub_and_fetch(&rc_wc_node->rm_sub_req_counter, 1);
		if (cnt == 0) {
			/* If all sub request have been processed.
			 * Send response and free rc_wc_node.*/
			__sync_add_and_fetch(&priv_p->p_read_counter, 1);
			__sync_add_and_fetch(&chan_p->c_rd_done_counter, 1);

			nid_log_debug("__bwc_do_read_async_twosteps: TWO_STEP_READ33 off:%ld length:%u, recv:%lu resp:%lu",
								sreq_p->sr_offset, sreq_p->sr_len, chan_p->c_rd_recv_counter, chan_p->c_rd_done_counter);
			sac_p->sa_op->sa_add_response_node(sac_p, rn_p);
			p_rc_wcn->rm_op->rm_put_node(p_rc_wcn, rc_wc_node);
		}
	} else {
		__sync_add_and_fetch(&priv_p->p_read_counter, 1);
		__sync_add_and_fetch(&chan_p->c_rd_done_counter, 1);
		nid_log_debug("__bwc_do_read_async_twosteps: TWO_STEP_READ34");
		sac_p->sa_op->sa_add_response_node(sac_p, rn_p);
	}
}

static void
__bwc_do_read_async(struct tp_jobheader *job)
{
	char* log_header = "__bwc_do_read_async";
	struct request_node *rn_p = (struct request_node *)job;
	struct bwc_interface *bwc_p = (struct bwc_interface *)rn_p->r_io_obj;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct d2an_interface *d2an_p = &priv_p->p_d2an;
	struct d2bn_interface *d2bn_p = &priv_p->p_d2bn;
	struct bwc_channel *chan_p = (struct bwc_channel *)rn_p->r_io_handle;
	struct rc_interface *rc_p = chan_p->c_chain->c_rc;
	void *rc_handle_p = chan_p->c_chain->c_rc_handle;
	struct rw_interface *rw_p = chan_p->c_chain->c_rw;
	struct sac_interface *sac_p = chan_p->c_sac;
	struct bse_interface *bse_p = &chan_p->c_chain->c_bse;
	struct nid_request *ir_p = &rn_p->r_ir;
	struct sub_request_node *sreq_p, *sreq_p2;
	struct d2a_node *rn = NULL;
	struct d2b_node *ran = NULL;
	int do_fp = priv_p->p_do_fp;
	uint32_t rcnt;

	nid_log_debug("%s: start ...", log_header);
	assert(ir_p->cmd == NID_REQ_READ);
	bse_p->bs_op->bs_search(bse_p, rn_p);

	/*
	 * r_head contains all blocks which are not available from bse.
	 */
	if (!list_empty(&rn_p->r_head)) {
		if (rc_p) {
			struct list_head res_head;
			INIT_LIST_HEAD(&res_head);
			list_for_each_entry_safe(sreq_p, sreq_p2, struct sub_request_node, &rn_p->r_head, sr_list) {
				rc_p->rc_op->rc_search(rc_p, rc_handle_p, sreq_p, &res_head);
				list_del(&sreq_p->sr_list);
				srn_p->sr_op->sr_put_node(srn_p, sreq_p);
			}
			assert(list_empty(&rn_p->r_head));
			if (!list_empty(&res_head))
				list_splice_tail_init(&res_head, &rn_p->r_head);
		}

		if (!list_empty(&rn_p->r_head)) {
			rn = d2an_p->d2_op->d2_get_node(d2an_p);
			rn->d2_acounter = 1;
			rn->d2_data[BWC_PRIV_INDEX] = (void *)priv_p;
			rn->d2_data[BWC_REQNODE_INDEX] = (void *)rn_p;

			/*
			 * Now, r_head contains all segments which are not available from cache.
			 * We have to read them through rw
			 */
			list_for_each_entry_safe(sreq_p, sreq_p2, struct sub_request_node, &rn_p->r_head, sr_list) {
				ran = d2bn_p->d2_op->d2_get_node(d2bn_p);
				ran->d2_data[BWC_RDNODE_INDEX] = (void *)rn;
				ran->d2_data[BWC_SREQNODE_INDEX] = (void *)sreq_p;
				ran->d2_arg.ag_type = RW_CB_TYPE_NORMAL;
				ran->d2_retry_time = 0;
				list_del(&sreq_p->sr_list);
				rcnt = __sync_add_and_fetch(&rn->d2_acounter, 1);
				if (do_fp) {
					rw_p->rw_op->rw_pread_async_fp(rw_p, chan_p->c_chain->c_rw_handle,
						sreq_p->sr_buf, (size_t)sreq_p->sr_len, sreq_p->sr_offset,
						__bwc_read_callback, &ran->d2_arg);
				} else {
					INIT_LIST_HEAD(&ran->d2_arg.ag_fp_head);
					rw_p->rw_op->rw_pread_async(rw_p, chan_p->c_chain->c_rw_handle,
							sreq_p->sr_buf, (size_t)sreq_p->sr_len, sreq_p->sr_offset,
							__bwc_read_callback, &ran->d2_arg);
				}
			}
		}
	}

	if (rn) {
		rcnt = __sync_sub_and_fetch(&rn->d2_acounter, 1);
		if (rcnt == 0) {
			sac_p->sa_op->sa_add_response_node(sac_p, rn_p);
			d2an_p->d2_op->d2_put_node(d2an_p, rn);
			__sync_add_and_fetch(&priv_p->p_read_counter, 1);
			__sync_add_and_fetch(&chan_p->c_rd_done_counter, 1);
		}
	} else {
		sac_p->sa_op->sa_add_response_node(sac_p, rn_p);
	}
}

/*
 * Algorithm:
 * 	Read from write cache first, then read from read cache
 * 	if not available from the write cache.
 * 	If neither available from wc nor from rc, read it by rw
 */
static void *
_bwc_read_buf_thread(void *p)
{
	char *log_header = "_bwc_read_buf_thread";
	struct bwc_interface *bwc_p = (struct bwc_interface *)p;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct bwc_chain *chain_p;
	struct bwc_channel *chan_p;
	struct list_head read_head;
	struct request_node *rn_p, *rn_p2;
	void (*__bwc_do_read_func)(struct tp_jobheader *job);
	int did_it = 0;
	uint16_t resp_delay;

	nid_log_notice("%s: start ...", log_header);
	if (!priv_p->p_rw_sync) {
		if (!priv_p->p_two_step_read) {
			__bwc_do_read_func = __bwc_do_read_async;
		} else {
			__bwc_do_read_func = __bwc_do_read_async_twosteps;
		}
	} else {
		__bwc_do_read_func = __bwc_do_read;
	}


next_read:
	if (priv_p->p_stop)
		goto out;

	if (!did_it) {
		pthread_mutex_lock(&priv_p->p_new_read_lck);
		while (!priv_p->p_stop && !priv_p->p_new_read_task) {
			pthread_cond_wait(&priv_p->p_new_read_cond, &priv_p->p_new_read_lck);
		}
		priv_p->p_new_read_task = 0;
		pthread_mutex_unlock(&priv_p->p_new_read_lck);
	}

	if (priv_p->p_stop)
		goto next_read;

	did_it = 0;
	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
			pthread_mutex_lock(&chan_p->c_chain->c_rlck);
			if (list_empty(&chan_p->c_read_head)) {
				pthread_mutex_unlock(&chan_p->c_chain->c_rlck);
				continue;
			}
			INIT_LIST_HEAD(&read_head);
			list_splice_tail_init(&chan_p->c_read_head, &read_head);
			/*
			 * also slow down the read while doing preparation for bwc merging
			 */
			resp_delay = chan_p->c_chain->c_fflush_state == BWC_C_FFLUSH_DOING ? chan_p->c_chain->c_rd_slowdown_step : 0;
			pthread_mutex_unlock(&chan_p->c_chain->c_rlck);
			if (list_empty(&read_head))
				continue;
			/*
			 * Got something to read for this channel
			 */
			list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &read_head, r_read_list) {
				list_del(&rn_p->r_read_list);
				rn_p->r_header.jh_do = __bwc_do_read_func;
				rn_p->r_header.jh_free = NULL;
				rn_p->r_io_obj = (void *)bwc_p;
				rn_p->r_resp_delay = resp_delay;
				tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)rn_p);
			}
			did_it = 1;
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	goto next_read;

out:
	priv_p->p_read_stop = 1;
	return NULL;
}

static void
__wait_for_free_block(struct bwc_interface *bwc_p, uint16_t wait_block)
{
	char *log_header = "__wait_for_free_block";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	nid_log_notice("%s: flushing too slow, waiting for block %u", log_header, wait_block);
	pthread_mutex_lock(&priv_p->p_occ_lck);
	while (!priv_p->p_stop && priv_p->p_flushing_st[wait_block].f_occupied) {
		pthread_cond_wait(&priv_p->p_occ_cond, &priv_p->p_occ_lck);
	}
	pthread_mutex_unlock(&priv_p->p_occ_lck);
	nid_log_notice("%s: waiting for block %u done!", log_header, wait_block);
}

static void *
_bwc_write_buf_thread(void *data)
{
	char *log_header = "_bwc_write_buf_thread";
	struct bwc_interface *bwc_p = (struct bwc_interface *)data;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bwc_dev_header *dhp = priv_p->p_dhp;
	struct bwc_dev_trailer *dtp = priv_p->p_dtp;
	struct bwc_seg_header *shp = priv_p->p_shp;
	struct bwc_seg_trailer *stp = priv_p->p_stp;
	struct bwc_req_header *rhp;
	struct ds_interface *ds_p;
	struct bse_interface *bse_p;
	struct ddn_interface *ddn_p;
	struct sac_interface *sac_p;
	struct bwc_chain *chain_p, *freeze_chain_p;
	struct request_node *rn_p, *rn_p2;
	struct bwc_channel *chan_p;
	char *pos;
	off_t wc_dev_offset, seg_off;
	uint32_t seg_len, sub_len, len, rh_len, rh_sub_len, req_nr;
	struct data_description_node *np, *nps[BWC_MAX_IOV];
	struct iovec iov[BWC_MAX_IOV];
	struct bwc_req_header mrh;
	int i, did_it = 0, vec_counter, rc = 0, need_rewind, retry0, retry1;
	int aligned_len;
	ssize_t wret;
	uint32_t start_req;
	suseconds_t time_consume;
	struct timeval write_start_time;
	struct timeval write_end_time;
	ssize_t io_size, nwritten;

	nid_log_notice("%s: start ...", log_header);

next_write:
	if (priv_p->p_stop)
		goto out;

	if (!did_it && !priv_p->p_snapshot_to_pause) {
		pthread_mutex_lock(&priv_p->p_new_write_lck);
		while (!priv_p->p_stop && !priv_p->p_new_write_task) {
			pthread_cond_wait(&priv_p->p_new_write_cond, &priv_p->p_new_write_lck);
		}
		priv_p->p_new_write_task = 0;
		pthread_mutex_unlock(&priv_p->p_new_write_lck);
	}

	priv_p->p_write_vec_counter = 0;
	/*
	 * If there is a snapshot requirement, we should wait.
	 */
	if (priv_p->p_snapshot_to_pause) {
		// there may have un-processed req in freeze chain, must process here, otherwise it
		// will lead sac cannot stop
		freeze_chain_p = priv_p->p_freeze_chain;
		priv_p->p_freeze_chain = NULL;
		if(freeze_chain_p) {
			priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
			list_for_each_entry(chan_p, struct bwc_channel, &freeze_chain_p->c_chan_head, c_list) {
				if (!list_empty(&chan_p->c_write_head)) {
					list_splice_tail_init(&chan_p->c_write_head, &priv_p->p_write_head);
				}
			}
			priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
			if(!list_empty(&priv_p->p_write_head)) {
				goto process_req;
			}
		}

		pthread_mutex_lock(&priv_p->p_snapshot_lck);
		assert(!priv_p->p_snapshot_pause);
		priv_p->p_snapshot_pause = 1;
		pthread_cond_broadcast(&priv_p->p_snapshot_cond);
		nid_log_warning("%s: pause flush to buffer device action ...", log_header);
		while (!priv_p->p_stop && priv_p->p_snapshot_pause)
			pthread_cond_wait(&priv_p->p_snapshot_cond, &priv_p->p_snapshot_lck);
		pthread_mutex_unlock(&priv_p->p_snapshot_lck);
		nid_log_warning("%s: restart flush to buffer device action ...", log_header);
		goto next_write;
	}

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
			pthread_mutex_lock(&chan_p->c_chain->c_wlck);
			if (list_empty(&chan_p->c_write_head)) {
				pthread_mutex_unlock(&chan_p->c_chain->c_wlck);
				continue;
			}
			list_splice_tail_init(&chan_p->c_write_head, &priv_p->p_write_head);
			pthread_mutex_unlock(&chan_p->c_chain->c_wlck);
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

process_req:
	io_size = 0;
	seg_off = lseek(priv_p->p_fhandle, 0, SEEK_CUR);

	if (list_empty(&priv_p->p_write_head)) {
		if (priv_p->p_seq_flushed > priv_p->p_rec_seq_flushed + 1024 || priv_p->p_sync_rec_seq) {
			/*
			 * The record of flushed sequence number on bufdevice is far behind
			 * the real flushed one. So we need to update the record on bufdevice
			 * with an empty segment
			 */

			sub_len = BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ;
			if (priv_p->p_cur_offset + sub_len + BWC_DEV_TRAILERSZ >= priv_p->p_bufdevicesz) {
				need_rewind = 1;
				goto do_rewind;
			}

			/* segment header */
			memcpy(shp->header_magic, dhp->dh_seg_header_magic, BWC_MAGICSZ);
			shp->req_nr = 0;
			shp->header_len = 0;
			shp->data_len = 0;
			shp->my_seq = ++priv_p->p_seq_assigned;
			shp->flushed_seq = priv_p->p_seq_flushed;
		//	nid_log_warning("temp_test: flush_seq: %lu", shp->flushed_seq);
			shp->wc_offset = seg_off;
			iov[0].iov_base = shp;
			iov[0].iov_len = BWC_SEG_HEADERSZ;
			io_size += iov[0].iov_len;
			priv_p->p_nondata_pkg_flushed += iov[0].iov_len;

			/* segment trailer */
			memcpy(stp->trailer_magic, dhp->dh_seg_trailer_magic, BWC_MAGICSZ);
			stp->req_nr =  shp->req_nr;
			stp->header_len = shp->header_len;
			stp->data_len = shp->data_len;
			stp->my_seq = shp->my_seq;
			stp->flushed_seq = shp->flushed_seq;
			stp->wc_offset = shp->wc_offset;
			iov[1].iov_base = stp;
			iov[1].iov_len = BWC_SEG_TRAILERSZ;
			io_size += iov[1].iov_len;
			priv_p->p_nondata_pkg_flushed += iov[1].iov_len;
			retry0 = 3;
retry0:
			lseek(priv_p->p_fhandle, seg_off, SEEK_SET);
			wret = writev(priv_p->p_fhandle, iov, 2);
			if (wret == -1) {
				nid_log_error("%s, write cache device error:%d, retry0:%d", log_header, errno, retry0);
				if (retry0--) {
					usleep(100000);
					goto retry0;
				}
			} else if (wret != io_size) {
				nid_log_notice("%s, going to retry writev, io_size:%ld, wret:%ld", log_header, io_size, wret);
				goto retry0;
			}
			pthread_mutex_lock(&priv_p->p_occ_lck);
			priv_p->p_seq_flush_aviable = priv_p->p_seq_assigned;
			pthread_mutex_unlock(&priv_p->p_occ_lck);

			priv_p->p_cur_offset = lseek(priv_p->p_fhandle, 0, SEEK_CUR);	// current offset
			priv_p->p_rec_seq_flushed = shp->flushed_seq;

			memset (&mrh, 0, sizeof(mrh));
			mrh.data_len = 0;
			mrh.wc_offset = priv_p->p_cur_offset;
			mrh.my_seq = priv_p->p_seq_assigned;
			mrh.is_mdz = 1;
			__bwc_mdz_add_request(priv_p, &mrh);
			__bwc_mdz_try_invalid(priv_p, shp);
			__bwc_mdz_try_flush(priv_p, shp);
		}
		did_it = 0;
		goto next_write;
	}

	/*
	 * we have something to write to the buffer device
	 */
	req_nr = 0;
	vec_counter = 2;
	need_rewind = 0;
	seg_len = BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ;
	rh_len = 0;
	rh_sub_len = 0;
	rhp = (struct bwc_req_header *)priv_p->p_rh_vec;
	start_req = priv_p->p_mdz_next_req;
	list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &priv_p->p_write_head, r_write_list) {
		struct nid_request *ir_p = &rn_p->r_ir;
		int need_new_block = 0;

		__sync_add_and_fetch(&priv_p->p_write_busy, 1);
		chan_p = (struct bwc_channel *)rn_p->r_io_handle;
		assert(chan_p);
		ddn_p = &chan_p->c_chain->c_ddn;
		/*
		 * slow down the per channel response when the channel is doing preparation for bwc merging
		 */
		rn_p->r_resp_delay = \
				chan_p->c_chain->c_fflush_state == BWC_C_FFLUSH_DOING ? \
						chan_p->c_chain->c_wr_slowdown_step : 0;

		sub_len = 0;
		if (rh_sub_len && rh_sub_len < priv_p->p_dio_alignment) {
			/*
			 * priv_p->p_dio_alignment is multiple of BWC_REQ_HEADERSZ
			 */
			rh_sub_len += BWC_REQ_HEADERSZ;
		} else {
			rh_sub_len = BWC_REQ_HEADERSZ;		// the first request header in a new logical block
			need_new_block = 1;			// need one new logical block
			sub_len = priv_p->p_dio_alignment;	// len increased for request header of this req
		}
		sub_len += rn_p->r_len;				// len increased for data of this req

		if (priv_p->p_cur_offset + seg_len + sub_len + BWC_DEV_TRAILERSZ >= priv_p->p_bufdevicesz) {
			need_rewind = 1;
			__sync_sub_and_fetch(&priv_p->p_write_busy, 1);
			break;
		}

		if (seg_len + sub_len > priv_p->p_segsz) {
			/* this segment is too larger to add next request */
			__sync_sub_and_fetch(&priv_p->p_write_busy, 1);
			break;
		}

		seg_len += sub_len;				// it's ok to add this req
		if (need_new_block) {
			rh_len += priv_p->p_dio_alignment;	// need one new logical block
			need_new_block = 0;
		}

		len = rn_p->r_len;
		ds_p = rn_p->r_ds;
		pos = ds_p->d_op->d_position_length(ds_p, rn_p->r_ds_index, rn_p->r_seq, &len);

		/*
		 * len could be less than rn_p->r_len since one request could be splited in
		 * two pages. But we'll combine splited pages into one on bufdevice.
		 *
		 */
		np = ddn_p->d_op->d_get_node(ddn_p);
		np->d_page_node = ds_p->d_op->d_page_node(ds_p, rn_p->r_ds_index, rn_p->r_seq);
		if (!priv_p->p_ssd_mode) {
			np->d_pos = (int64_t)pos;
			np->d_location_disk = 0;
		}
		np->d_seq = ++priv_p->p_seq_assigned;
//		nid_log_warning("add node to memory:d_seq:%lu d_pos:%lu", np->d_seq, np->d_pos);
		np->d_len = len;
		np->d_offset = rn_p->r_offset;
		np->d_flushing = 0;
		np->d_flushed = 0;
		np->d_flushed_back = 0;
		np->d_flush_done = 0;
		np->d_released = 0;
		np->d_where = DDN_WHERE_NO;
		np->d_flush_overwritten = 0;
		np->d_callback_arg = NULL;
		list_add_tail(&np->d_slist, &chan_p->c_chain->c_search_head);
		nid_log_debug("%s: add data node %p to chain %p search list %p", log_header, np,
				chan_p->c_chain, &chan_p->c_chain->c_search_head);
		/* request header */
		rhp->data_len = rn_p->r_len; //the whole length instead of np->d_len;
		rhp->offset = np->d_offset;
		rhp->resource_id = chan_p->c_chain->c_id;
		assert(rhp->resource_id);
		rhp->my_seq = np->d_seq;
		rhp->is_mdz = 0;

		/* update mdz */
		memset (&mrh, 0, sizeof(mrh));
		mrh.resource_id = rhp->resource_id;
		mrh.data_len = rn_p->r_len;
		mrh.offset = np->d_offset;
		mrh.wc_offset = 0;	// don't know yet
		mrh.my_seq = np->d_seq;
		mrh.is_mdz = 1;
		__bwc_mdz_add_request(priv_p, &mrh);

		// don't use rhp++ in case BWC_REQ_HEADERSZ > sizeof(*rhp)
		rhp = (struct bwc_req_header *)((char *)rhp + BWC_REQ_HEADERSZ);
		req_nr++;

		/* data */
		iov[vec_counter].iov_base = pos;
		iov[vec_counter].iov_len = np->d_len;
		io_size += iov[vec_counter].iov_len;
		nps[vec_counter] = np;
		priv_p->p_data_flushed += iov[vec_counter].iov_len;
		priv_p->p_data_pkg_flushed += iov[vec_counter].iov_len;
		if (priv_p->p_start_count) {
			priv_p->p_req_num++;
			priv_p->p_req_len += iov[vec_counter].iov_len;
		}
		vec_counter++;
		priv_p->p_write_vec_counter++;

		// update potential snapshot point
		pthread_mutex_lock(&chain_p->c_lck);
		chain_p->c_seq_assigned = priv_p->p_seq_assigned;
		pthread_mutex_unlock(&chain_p->c_lck);

		/*
		 * taking care the second part of the request if there is
		 */
		if (!(rc < 0) && len < rn_p->r_len) {
			pos = ds_p->d_op->d_position(ds_p, rn_p->r_ds_index, rn_p->r_seq + len);
			np = ddn_p->d_op->d_get_node(ddn_p);
			np->d_page_node = ds_p->d_op->d_page_node(ds_p, rn_p->r_ds_index, rn_p->r_seq + len);
			if (!priv_p->p_ssd_mode) {
				np->d_pos = (int64_t)pos;
				np->d_location_disk = 0;
			}
			np->d_seq = priv_p->p_seq_assigned;	// share the same seq in memory
//			nid_log_warning("add node to memory:d_seq:%lu d_pos:%lu", np->d_seq, np->d_pos);
			np->d_len = rn_p->r_len - len;		// reduce the first part
			np->d_offset = rn_p->r_offset + len;
			np->d_flushing = 0;
			np->d_flushed = 0;
			np->d_flushed_back = 0;
			np->d_flush_done = 0;
			np->d_released = 0;
			np->d_where = DDN_WHERE_NO;
			np->d_flush_overwritten = 0;
			np->d_callback_arg = NULL;
			list_add_tail(&np->d_slist, &chan_p->c_chain->c_search_head);
			nid_log_debug("%s: add data node %p to chain %p search list %p", log_header, np,
					chan_p->c_chain, &chan_p->c_chain->c_search_head);
			/* data */
			iov[vec_counter].iov_base = pos;
			iov[vec_counter].iov_len = np->d_len;
			io_size += iov[vec_counter].iov_len;
			nps[vec_counter] = np;
			priv_p->p_data_flushed += iov[vec_counter].iov_len;
			priv_p->p_data_pkg_flushed += iov[vec_counter].iov_len;
			if (priv_p->p_start_count) {
				priv_p->p_req_len += iov[vec_counter].iov_len;
			}
			vec_counter++;
		}
		ir_p->code = (rc < 0) ? errno : 0;
		ir_p->len = ntohl(0);
		list_del(&rn_p->r_write_list);
		list_add_tail(&rn_p->r_list, &chan_p->c_resp_head);
		priv_p->p_resp_counter++;
		if (vec_counter >= BWC_MAX_IOV - 1) {
			__sync_sub_and_fetch(&priv_p->p_write_busy, 1);
			break;
		}
		__sync_sub_and_fetch(&priv_p->p_write_busy, 1);
	}

	if (vec_counter > 2) {
		assert(rh_len % BWC_REQ_HEADERSZ == 0);
		assert(req_nr * BWC_REQ_HEADERSZ <= rh_len);
		assert(req_nr * BWC_REQ_HEADERSZ > rh_len - priv_p->p_dio_alignment);

		/*
		 * Before write to the bufdevice, make sure we have enough free space to write
		 */
		i = 0;
		if (!priv_p->p_flushing_st[priv_p->p_cur_block].f_occupied) {
			/*
			 * don't count p_cur_block even if its not occupied since we don't want
			 * to worry about how much free space left
			 * This condition should always be true unless all blocks got occupied
			 */
			i = 1;
		}

		/*
		 * don't count p_cur_block even if its not occupied since we don't want
		 * to worry about how much free space left
		 * This condition should should always be true (?)
		 */
		sub_len = 0;
		while (sub_len < seg_len && (priv_p->p_cur_block + i < priv_p->p_flush_nblocks)) {
			if (priv_p->p_flushing_st[priv_p->p_cur_block + i].f_occupied) {
				__wait_for_free_block(bwc_p, priv_p->p_cur_block + i);
			}
			if (priv_p->p_stop)
				goto next_write;
			sub_len += priv_p->p_flush_blocksz;
			i++;
		}

		/*
		 * segment header
		 */
		memcpy(shp->header_magic, dhp->dh_seg_header_magic, BWC_MAGICSZ);
		shp->req_nr = req_nr;
		shp->header_len = rh_len;		// aligned
		shp->data_len = seg_len - (BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ + shp->header_len);
		shp->my_seq = priv_p->p_seq_assigned;	// seq of the last req in this segment
		shp->flushed_seq = priv_p->p_seq_flushed;
//		nid_log_warning("temp_test: flush_seq: %lu", shp->flushed_seq);
		shp->wc_offset = seg_off;
		iov[0].iov_base = shp;
		iov[0].iov_len = BWC_SEG_HEADERSZ;
		io_size += iov[0].iov_len;
		priv_p->p_data_pkg_flushed += iov[0].iov_len;

		/*
		 * segment trailer
		 */
		memcpy(stp->trailer_magic, dhp->dh_seg_trailer_magic, BWC_MAGICSZ);
		stp->req_nr = shp->req_nr;
		stp->header_len = shp->header_len;
		stp->data_len = shp->data_len;
		stp->my_seq = shp->my_seq;
		stp->flushed_seq = shp->flushed_seq;
		stp->wc_offset = shp->wc_offset;
		iov[vec_counter].iov_base = stp;
		iov[vec_counter].iov_len = BWC_SEG_TRAILERSZ;
		io_size += iov[vec_counter].iov_len;
		priv_p->p_data_pkg_flushed += iov[vec_counter].iov_len;
		vec_counter++;

		/*
		 * request header
		 */
		__bwc_hdz_update_offset(priv_p, req_nr, rh_len, seg_off);
		iov[1].iov_base = priv_p->p_rh_vec;
		iov[1].iov_len = rh_len;
		io_size += iov[1].iov_len;
		priv_p->p_data_pkg_flushed += iov[1].iov_len;

		if (priv_p->p_ssd_mode) {
			/**
			 * Calculate np offsets.
			 */
			priv_p->p_cur_offset = lseek(priv_p->p_fhandle, 0, SEEK_CUR);	// current offset
			wc_dev_offset = priv_p->p_cur_offset + iov[0].iov_len + iov[1].iov_len;
			for (i = 2; i < vec_counter-1; i++) {
				nps[i]->d_pos = wc_dev_offset;
			//	nid_log_warning("add node to memory:d_seq:%lu d_pos:%lu d_len:%u", nps[i]->d_seq, nps[i]->d_pos, nps[i]->d_len);
				nps[i]->d_location_disk = 1;
				wc_dev_offset += nps[i]->d_len;
			}
		}

		/*
		 * Now, we know we have enough free space on bufdevice to write the segment
		 */
		priv_p->p_cur_offset = lseek(priv_p->p_fhandle, 0, SEEK_CUR);
		assert(priv_p->p_cur_offset + BWC_DEV_TRAILERSZ <= priv_p->p_bufdevicesz);
		nid_log_notice("%s: write something to bufdevice", log_header);
		retry1 = 3;
retry1:
		lseek(priv_p->p_fhandle, seg_off, SEEK_SET);
		gettimeofday(&write_start_time, NULL);
		wret = writev(priv_p->p_fhandle, iov, vec_counter);
		gettimeofday(&write_end_time, NULL);
		time_consume = time_difference(write_start_time, write_end_time);
		if (time_consume > 1000000) {
			nid_log_warning("%s, one write consumed: %ld", log_header, time_consume);
		}
		if (wret == -1) {
			nid_log_error("%s, write cache device error:%d, retry1:%d", log_header, errno, retry1);
			if (retry1--) {
				usleep(100000);
				goto retry1;
			}
			list_for_each_entry(rn_p, struct request_node, &chan_p->c_resp_head,  r_list) {
				rn_p->r_ir.code = errno;
			}
			// Response error now for this error chan_p.
			sac_p = chan_p->c_sac;
			sac_p->sa_op->sa_add_response_list(sac_p, &chan_p->c_resp_head);
		} else if (wret != io_size) {
			nid_log_notice("%s, going to retry writev, io_size:%ld, wret:%ld", log_header, io_size, wret);
			goto retry1;
		}
		priv_p->p_rec_seq_flushed = shp->flushed_seq;
		__bwc_mdz_update_offset(priv_p, start_req, priv_p->p_cur_offset + BWC_SEG_HEADERSZ + shp->header_len);
		__bwc_mdz_try_invalid(priv_p, shp);
		__bwc_mdz_try_flush(priv_p, shp);

		priv_p->p_cur_offset = lseek(priv_p->p_fhandle, 0, SEEK_CUR);	// current offset
		assert(priv_p->p_cur_offset + BWC_DEV_TRAILERSZ <= priv_p->p_bufdevicesz);
		priv_p->p_seq_flushable = priv_p->p_seq_assigned;

		__bwc_write_delay(priv_p);

		priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
			if (!list_empty(&chain_p->c_search_head)) {
				bse_p = &chain_p->c_bse;
				bse_p->bs_op->bs_add_list(bse_p, &chain_p->c_search_head);
			}
			list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
				if (!list_empty(&chan_p->c_resp_head)) {
					sac_p = chan_p->c_sac;
					nid_log_notice("%s: write response", log_header);
					sac_p->sa_op->sa_add_response_list(sac_p, &chan_p->c_resp_head);
				}
			}
		}
		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	}

	pthread_mutex_lock(&priv_p->p_occ_lck);
	while (priv_p->p_cur_offset > priv_p->p_flushing_st[priv_p->p_cur_block].f_end_offset) {
		priv_p->p_flushing_st[priv_p->p_cur_block].f_seq = priv_p->p_seq_assigned;
		priv_p->p_flushing_st[priv_p->p_cur_block].f_occupied = 1;
		priv_p->p_noccupied++;
		assert(priv_p->p_noccupied <= priv_p->p_flush_nblocks);
		priv_p->p_cur_block++;
	}
	priv_p->p_seq_flush_aviable = priv_p->p_seq_assigned;
	pthread_mutex_unlock(&priv_p->p_occ_lck);
do_rewind:
	if (need_rewind) {
		nid_log_notice("%s: need rewind, p_cur_offset:%lu",
			log_header, priv_p->p_cur_offset);

		pthread_mutex_lock(&priv_p->p_occ_lck);
		while (priv_p->p_cur_block < priv_p->p_flush_nblocks) {
			priv_p->p_flushing_st[priv_p->p_cur_block].f_seq = priv_p->p_seq_assigned;
			priv_p->p_flushing_st[priv_p->p_cur_block].f_occupied = 1;
			priv_p->p_noccupied++;
			priv_p->p_cur_block++;
		}
		assert(priv_p->p_noccupied <= priv_p->p_flush_nblocks);
		pthread_mutex_unlock(&priv_p->p_occ_lck);
		memcpy(dtp->trailer_magic, BWC_DEV_TRAILER_MAGIC, BWC_MAGICSZ);
		memcpy(dtp->trailer_ver, BWC_DEV_TRAILER_VER, BWC_VERSZ);
		dtp->dt_offset = priv_p->p_cur_offset;
		dtp->dt_my_seq = priv_p->p_seq_assigned;
		dtp->dt_flushed_seq = priv_p->p_seq_flushed;

		lseek(priv_p->p_fhandle, priv_p->p_bufdevicesz - BWC_DEV_TRAILERSZ, SEEK_SET);	// the trailer
		len = sizeof(*dtp);
		aligned_len = __round_up(len, priv_p->p_dio_alignment);
		nwritten = __write_n(priv_p->p_fhandle, (char*)dtp, aligned_len);
		assert(nwritten == aligned_len);

		lseek(priv_p->p_fhandle, priv_p->p_first_seg_off, SEEK_SET);	// rewind
		priv_p->p_cur_offset = priv_p->p_first_seg_off;						// current offset
		priv_p->p_cur_block = (priv_p->p_first_seg_off - priv_p->p_mdz_size) / priv_p->p_flush_blocksz;
	}

	did_it = 1;
	goto next_write;

out:
	priv_p->p_write_stop = 1;
	return NULL;
}

static void *
_bwc_trim_thread(void *data)
{
	char *log_header = "_bwc_trim_thread";
	struct bwc_interface *bwc_p = (struct bwc_interface *)data;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct sac_interface *sac_p;
	struct bwc_chain *chain_p;
	struct bwc_channel *chan_p;
	struct bfe_interface *bfe_p;
	struct request_node *rn_p, *rn_p2;

	nid_log_notice("%s: start ...", log_header);

next_trim:
	if (priv_p->p_stop)
		goto out;

	// it no trim task, wait for get new trim task
	// bwc_trim_list insert new trim request to trim list, then wake up the thread
	pthread_mutex_lock(&priv_p->p_new_trim_lck);
	while (!priv_p->p_stop && !priv_p->p_new_trim_task) {
		pthread_cond_wait(&priv_p->p_new_trim_cond, &priv_p->p_new_trim_lck);
	}
	priv_p->p_new_trim_task = 0;
	pthread_mutex_unlock(&priv_p->p_new_trim_lck);

	if (priv_p->p_stop)
		goto next_trim;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if (chain_p->c_busy || list_empty(&chain_p->c_chan_head)) {
			continue;
		}

		chain_p->c_busy = 1;

		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

		pthread_mutex_lock(&chain_p->c_trlck);
		list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
			if (list_empty(&chan_p->c_trim_head)) {
				continue;
			}

			list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &chan_p->c_trim_head, r_trim_list) {
				struct nid_request *ir_p = &rn_p->r_ir;
				chan_p = (struct bwc_channel *)rn_p->r_io_handle;
				assert(chan_p);

				ir_p->code = ntohl(0);
				ir_p->len = ntohl(0);
				list_add_tail(&rn_p->r_list, &chan_p->c_resp_head);
				priv_p->p_resp_counter++;
			}
		}

		pthread_mutex_unlock(&chain_p->c_trlck);
		bfe_p = &chain_p->c_bfe;
		list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
			if (list_empty(&chan_p->c_trim_head)) {
				continue;
			}
			bfe_p->bf_op->bf_trim_list(bfe_p, &chan_p->c_trim_head);
			if (!list_empty(&chan_p->c_resp_head)) {
				sac_p = chan_p->c_sac;
				nid_log_notice("%s: trim response", log_header);
				sac_p->sa_op->sa_add_response_list(sac_p, &chan_p->c_resp_head);
			}

		}
		priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		chain_p->c_busy = 0;
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	goto next_trim;

out:
	priv_p->p_trim_stop = 1;
	return NULL;
}

void
__bwc_release_block(struct bwc_interface *bwc_p, uint64_t seq_expecting)
{
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	pthread_mutex_lock(&priv_p->p_occ_lck);
	while (priv_p->p_flushing_st[priv_p->p_cur_flush_block].f_occupied &&
	    seq_expecting >= priv_p->p_flushing_st[priv_p->p_cur_flush_block].f_seq) {
		priv_p->p_flushing_st[priv_p->p_cur_flush_block].f_occupied = 0;
		priv_p->p_noccupied--;
		priv_p->p_cur_flush_block++;
//		nid_log_warning("temp_test: flush_block: %d", priv_p->p_cur_flush_block);
		if (priv_p->p_cur_flush_block == priv_p->p_flush_nblocks) {
			priv_p->p_cur_flush_block = (priv_p->p_first_seg_off - priv_p->p_mdz_size) / priv_p->p_flush_blocksz;
//		nid_log_warning("temp_test: flush_block: %d", priv_p->p_cur_flush_block);
		}
	}
	pthread_cond_signal(&priv_p->p_occ_cond);
	pthread_mutex_unlock(&priv_p->p_occ_lck);
}

static void *
_bwc_release_thread(void *p)
{
	char *log_header = "_bwc_release_thread";
	struct bwc_interface *bwc_p = (struct bwc_interface *)p;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bwc_chain *chain_p;
	struct bfe_interface *bfe_p;
	struct bfp_interface *bfp_p = priv_p->p_bfp;
	struct rw_interface *rw_p;
	uint64_t seq_expecting;
	int cnt_flush_page = 0; // for common flush and fast flush all
	int cnt_flush_page2 = 0; // for channel specific fast flush

	nid_log_notice("%s: start ...", log_header);

next_try:
	if (priv_p->p_stop)
		goto out;

	// before finish recovery, bwc will stay in pause state
	if (priv_p->p_pause) {
		sleep(1);
		goto next_try;
	}

	if (cnt_flush_page != 0 || cnt_flush_page2 != 0)
		goto do_flushing;

	if (priv_p->p_fast_flush &&
	    priv_p->p_seq_flushed < priv_p->p_seq_flush_aviable) {
		if (list_empty(&priv_p->p_cfflush_head))
			cnt_flush_page = 1; // fast flush for all
		else
			cnt_flush_page2 = 1; // fast flush for listed channels

		goto do_flushing;
	}

	cnt_flush_page = bfp_p->fp_op->bf_get_flush_num(bfp_p);
	assert(cnt_flush_page >= 0);
	if (!cnt_flush_page || priv_p->p_seq_flush_aviable <= priv_p->p_seq_flushed) {
		sleep(1);
		goto next_try;
	}

do_flushing:
	/*
	 * we are here since cnt_flush_page > 0 or cnt_flush_page2 > 0
	 * Flushing one block: p_cur_flush_block
	 */

	if (priv_p->p_flushing_st[priv_p->p_cur_flush_block].f_occupied)
		seq_expecting = priv_p->p_flushing_st[priv_p->p_cur_flush_block].f_seq;
	else
		seq_expecting = priv_p->p_seq_flush_aviable;
	/**
	 * After set seq_expecting, we may still got bigger seq_flushed,
	 * due to do fast flush with last page coalesce flushed.
	 * And we may got the seq_expecting from priv_p->p_flushing_st[priv_p->p_cur_flush_block].f_seq,
	 * which have very possibility smaller than seq_flushed in this case.
	 */
	if (seq_expecting < priv_p->p_seq_flushed)
		seq_expecting = priv_p->p_seq_flushed;

	nid_log_debug("%s: seq flushed %lu, target %lu", log_header,
			priv_p->p_seq_flushed, seq_expecting);
	if (cnt_flush_page) {
		if (seq_expecting > priv_p->p_seq_flushed) {
			priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
			list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
				if(chain_p->c_freeze)
					continue;
				bfe_p = &chain_p->c_bfe;
				rw_p = chain_p->c_rw;
				assert(bfe_p);
				nid_log_debug("%s: flushing seq:%lu", log_header, seq_expecting);

				if (!rw_p->rw_op->rw_get_device_ready(rw_p)) {
					nid_log_notice("%s: rw of chain:%s not ready", log_header, chain_p->c_resource);
					continue;
				}
				bfe_p->bf_op->bf_flush_seq(bfe_p, seq_expecting);
			}

			list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
				if(chain_p->c_freeze)
					continue;
				bfe_p = &chain_p->c_bfe;
				rw_p = chain_p->c_rw;
				assert(bfe_p);
				nid_log_debug("%s: flushing seq:%lu", log_header, seq_expecting);

				if (!rw_p->rw_op->rw_get_device_ready(rw_p)) {
					nid_log_notice("%s: rw of chain:%s not ready", log_header, chain_p->c_resource);
					continue;
				}
				bfe_p->bf_op->bf_flush_seq(bfe_p, seq_expecting);
			}
			priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		}
	}

	if (cnt_flush_page2) {
		if (seq_expecting > priv_p->p_seq_flushed) {
			priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
			list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_cfflush_head, c_fflush_list) {
				bfe_p = &chain_p->c_bfe;
				rw_p = chain_p->c_rw;
				assert(bfe_p);
				nid_log_debug("%s: flushing seq:%lu", log_header, seq_expecting);

				if (!rw_p->rw_op->rw_get_device_ready(rw_p)) {
					nid_log_notice("%s: rw of chain:%s not ready", log_header, chain_p->c_resource);
					continue;
				}
				bfe_p->bf_op->bf_flush_seq(bfe_p, seq_expecting);
			}
			priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		}
	}

	pthread_mutex_lock(&priv_p->p_seq_lck);
	while (!priv_p->p_stop && (priv_p->p_seq_flushed < seq_expecting)) {
		pthread_cond_wait(&priv_p->p_seq_cond, &priv_p->p_seq_lck);
	}
	if (priv_p->p_stop) {
		pthread_mutex_unlock(&priv_p->p_seq_lck);
		goto next_try;
	}
	pthread_mutex_unlock(&priv_p->p_seq_lck);

	// release flushed block in cache device
	__bwc_release_block(bwc_p, seq_expecting);

	if (cnt_flush_page) {
		nid_log_debug("%s: cnt_flush_page:%d", log_header, cnt_flush_page);
		cnt_flush_page--;
	}

	if (cnt_flush_page2) {
		nid_log_debug("%s: cnt_flush_page2:%d", log_header, cnt_flush_page2);
		cnt_flush_page2--;
	}
	goto next_try;

out:
	priv_p->p_release_stop = 1;
	return NULL;
}


static void *
_bwc_post_read_thread(void *p)
{
	char *log_header = "_bwc_post_read_thread";
	struct request_node *rn_p;
	struct bwc_interface *bwc_p = (struct bwc_interface *)p;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct d2an_interface *d2an_p = &priv_p->p_d2an;
	struct d2bn_interface *d2bn_p = &priv_p->p_d2bn;
	struct bwc_channel *chan_p;
	struct rc_interface *rc_p;
	struct rw_interface *rw_p;
	struct sac_interface *sac_p;
	struct d2b_node *ran;
	struct sub_request_node *sreq_p;
	struct d2a_node *rn;
	struct fpn_interface *fpn_p;
	uint32_t rcnt;

	nid_log_info("%s: start ...", log_header);

next_try:
	if (priv_p->p_stop)
		goto out;

	pthread_mutex_lock(&priv_p->p_read_finish_lck);
	while (list_empty(&priv_p->p_read_finished_head)) {
		pthread_cond_wait(&priv_p->p_read_post_cond, &priv_p->p_read_finish_lck);
		if (priv_p->p_stop) {
			pthread_mutex_unlock(&priv_p->p_read_finish_lck);
			sleep(1);
			goto next_try;
		}
	}

	while (!list_empty(&priv_p->p_read_finished_head)) {
		nid_log_notice("%s: got something back", log_header);
		ran = list_first_entry(&priv_p->p_read_finished_head, struct d2b_node, d2_list);
		list_del(&ran->d2_list);
		pthread_mutex_unlock(&priv_p->p_read_finish_lck);

		rn = (struct d2a_node *)ran->d2_data[BWC_RDNODE_INDEX];
		rn_p = (struct request_node *)rn->d2_data[BWC_REQNODE_INDEX];
		chan_p = (struct bwc_channel *)rn_p->r_io_handle;
		rc_p = chan_p->c_chain->c_rc;
		sac_p = chan_p->c_sac;
		rw_p = chan_p->c_chain->c_rw;
		fpn_p = rw_p->rw_op->rw_get_fpn_p(rw_p);


		sreq_p = (struct sub_request_node *)ran->d2_data[BWC_SREQNODE_INDEX];

		/*Check if error happened, set error number to response code.*/
		if (ran->d2_ret) {
			nid_log_error("%s: read failed: %d.", log_header, ran->d2_ret);
			rn_p->r_ir.code = ran->d2_ret;
		} else {
			/* Update read cache */
			if (rc_p && ran->d2_flag == 0 && !ran->d2_ret) {
				rc_p->rc_op->rc_update(rc_p, chan_p->c_chain->c_rc_handle,
					sreq_p->sr_buf, sreq_p->sr_len,
					sreq_p->sr_offset, &ran->d2_arg.ag_fp_head);
			}
		}
		srn_p->sr_op->sr_put_node(srn_p, sreq_p);
		rcnt = __sync_sub_and_fetch(&rn->d2_acounter, 1);
		if (rcnt == 0) {
			sac_p->sa_op->sa_add_response_node(sac_p, rn_p);
			d2an_p->d2_op->d2_put_node(d2an_p, rn);

			__sync_add_and_fetch(&priv_p->p_read_counter, 1);
			__sync_add_and_fetch(&chan_p->c_rd_done_counter, 1);

		}

		/* Free finger print nodes */
		while (!list_empty(&ran->d2_arg.ag_fp_head)) {
			struct fp_node *fpnd = list_first_entry(&ran->d2_arg.ag_fp_head, struct fp_node, fp_list);
			list_del(&fpnd->fp_list);
			fpn_p->fp_op->fp_put_node(fpn_p, fpnd);
		}

		d2bn_p->d2_op->d2_put_node(d2bn_p, ran);
		pthread_mutex_lock(&priv_p->p_read_finish_lck);
	}
	pthread_mutex_unlock(&priv_p->p_read_finish_lck);

	goto next_try;

out:
	priv_p->p_post_read_stop = 1;
	return NULL;
}

static uint64_t
bwc_get_coalesce_index(struct bwc_interface *bwc_p) {
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bwc_chain *chain_p;
	struct bfe_interface *bfe_p;
	uint64_t coalesce_idx = 0;
	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		bfe_p = &chain_p->c_bfe;
		assert(bfe_p);
		coalesce_idx += bfe_p->bf_op->bf_get_coalesce_index(bfe_p);
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	return coalesce_idx;
}

static void
bwc_reset_coalesce_index(struct bwc_interface *bwc_p) {
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bwc_chain *chain_p;
	struct bfe_interface *bfe_p;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		bfe_p = &chain_p->c_bfe;
		assert(bfe_p);
		bfe_p->bf_op->bf_reset_coalesce_index(bfe_p);
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
}

static uint8_t
bwc_get_cutoff(struct bwc_interface *bwc_p, void *wc_handle)
{
	(void)bwc_p;
	struct bwc_channel *chan_p = (struct bwc_channel *)wc_handle;
	uint8_t cutoff;
	cutoff = chan_p->c_chain->c_cutoff;
	return cutoff;
}

static int
bwc_update_water_mark(struct bwc_interface *bwc_p, int low_water_mark, int high_water_mark)
{
	char *log_header = "bwc_update_water_mark";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bfp_interface *bfp_p = priv_p->p_bfp;
	int rc;

	nid_log_info("%s: start ...", log_header);
	priv_p->p_low_water_mark = low_water_mark;
	priv_p->p_high_water_mark = high_water_mark;
	rc =  bfp_p->fp_op->bf_update_water_mark(bfp_p, low_water_mark, high_water_mark);

	return rc;

}

static int
bwc_update_delay_level(struct bwc_interface *bwc_p, struct bwc_delay_stat *delay_stat)
{
	char *log_header = "bwc_update_delay_level";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	nid_log_info("%s: start ...", log_header);

	if (delay_stat->write_delay_first_level <= delay_stat->write_delay_second_level && delay_stat->write_delay_first_level != 0)
		return -1;
	priv_p->p_write_delay_first_level = delay_stat->write_delay_first_level * priv_p->p_flush_nblocks / 100;
	if (priv_p->p_write_delay_first_level) {
		priv_p->p_write_delay_second_level = delay_stat->write_delay_second_level * priv_p->p_flush_nblocks / 100;
		priv_p->p_write_delay_first_level_increase_interval = delay_stat->write_delay_first_level_max_us / (priv_p->p_write_delay_first_level - priv_p->p_write_delay_second_level);
		priv_p->p_write_delay_second_level_increase_interval = delay_stat->write_delay_second_level_max_us / priv_p->p_write_delay_second_level;
		priv_p->p_write_delay_second_level_start_ms = delay_stat->write_delay_first_level_max_us;
	}
	return 0;

}

static int
bwc_destroy(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_destroy";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct d2an_interface *d2an_p = &priv_p->p_d2an;
	struct d2bn_interface *d2bn_p =  &priv_p->p_d2bn;
	struct rc_wc_cbn_interface *rc_wc_cbn_p = &priv_p->p_rc_wc_cbn;
	struct rc_wcn_interface *rc_wcn_p = &priv_p->p_rc_wcn;

	int rc = 1;

	if (!list_empty(&priv_p->p_chain_head)) {
		nid_log_error("%s: active chain list not empty, cannot destroy!", log_header);
		return rc;
	}

	if (!list_empty(&priv_p->p_frozen_chain_head)) {
		nid_log_error("%s: frozen chain list not empty, cannot destroy!", log_header);
		return rc;
	}

	if (!list_empty(&priv_p->p_freezing_chain_head)) {
		nid_log_error("%s: freezing chain list not empty, cannot destroy!", log_header);
		return rc;
	}

	if (!list_empty(&priv_p->p_inactive_head)) {
		nid_log_error("%s: inactive chain list not empty, cannot destroy!", log_header);
		return rc;
	}
	if (!list_empty(&priv_p->p_cfflush_head)) {
		nid_log_error("%s: fast flush chain list not empty, cannot destroy!", log_header);
		return rc;
	}
	rc = 0;

	bwc_stop(bwc_p);

	/*
	 * cleanup work
	 */
	d2an_p->d2_op->d2_destroy(d2an_p);
	d2bn_p->d2_op->d2_destroy(d2bn_p);
	rc_wc_cbn_p->rm_op->rm_destroy(rc_wc_cbn_p);
	rc_wcn_p->rm_op->rm_destroy(rc_wcn_p);

	free((void *)priv_p->p_rh_vec);
	free((void *)priv_p->p_dhp);
	free((void *)priv_p->p_dtp);
	free((void *)priv_p->p_shp);
	free((void *)priv_p->p_stp);
	free((void *)priv_p->p_seg_content);
	free((void *)priv_p->p_mdz_hp);
	free((void *)priv_p->p_mdz_rhp);

	close(priv_p->p_fhandle);

	pthread_mutex_destroy(&priv_p->p_read_finish_lck);
	pthread_cond_destroy(&priv_p->p_read_post_cond);
	pthread_mutex_destroy(&priv_p->p_lck);
	priv_p->p_lck_if.l_op->l_destroy(&priv_p->p_lck_if);
	pthread_mutex_destroy(&priv_p->p_snapshot_lck);
	pthread_cond_destroy(&priv_p->p_snapshot_cond);
	pthread_mutex_destroy(&priv_p->p_new_trim_lck);
	pthread_cond_destroy(&priv_p->p_new_trim_cond);
	pthread_mutex_destroy(&priv_p->p_new_write_lck);
	pthread_cond_destroy(&priv_p->p_new_write_cond);
	pthread_mutex_destroy(&priv_p->p_new_read_lck);
	pthread_cond_destroy(&priv_p->p_new_read_cond);
	pthread_mutex_destroy(&priv_p->p_seq_lck);
	pthread_cond_destroy(&priv_p->p_seq_cond);
	pthread_mutex_destroy(&priv_p->p_occ_lck);
	pthread_cond_destroy(&priv_p->p_occ_cond);

	free((void *)priv_p->p_flushing_st);
	free((void *)priv_p);
	return rc;
}



static int
bwc_write_response(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_write_response";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bse_interface *bse_p;
	struct sac_interface *sac_p;
	struct bwc_chain *chain_p;
	struct bwc_channel *chan_p;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
			if (!list_empty(&chan_p->c_resp_head)) {
				sac_p = chan_p->c_sac;
				bse_p = &chain_p->c_bse;
				bse_p->bs_op->bs_add_list(bse_p, &chain_p->c_search_head);
				priv_p->p_seq_flushable = priv_p->p_seq_assigned;
				nid_log_notice("%s: write response", log_header);
				sac_p->sa_op->sa_add_response_list(sac_p, &chan_p->c_resp_head);
			}
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	return  0;
}

static void
bwc_post_initialization(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_post_initialization";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bfp_interface *bfp_p;
	struct bfp_setup bfp_setup;
	pthread_attr_t attr;
	pthread_t thread_id;
	mode_t mode;
	int i, flags;
	int max_vec;
	struct d2an_setup d2an_setup;
	struct d2bn_interface *d2bn_p;
	struct d2bn_setup d2bn_setup;
	struct d2an_interface *d2an_p;
	struct rc_wc_cbn_setup rmc_setup;
	struct rc_wcn_setup rm_setup;
	struct bwc_rbn_setup nn_setup;
	size_t page_size;

	nid_log_warning("%s: start ...", log_header);

	page_size = getpagesize();
	x_posix_memalign((void **)&priv_p->p_rh_vec, page_size, BWC_REQ_HEADERSZ * BWC_MAX_IOV);
	memset(priv_p->p_rh_vec, 0, page_size);
	x_posix_memalign((void **)&priv_p->p_dhp, page_size, BWC_DEV_HEADERSZ);
	x_posix_memalign((void **)&priv_p->p_dtp, page_size, BWC_DEV_TRAILERSZ);
	x_posix_memalign((void **)&priv_p->p_shp, page_size, BWC_SEG_HEADERSZ);
	x_posix_memalign((void **)&priv_p->p_stp, page_size, BWC_SEG_TRAILERSZ);
	x_posix_memalign((void **)&priv_p->p_seg_content, page_size, priv_p->p_segsz);
	x_posix_memalign((void **)&priv_p->p_mdz_hp, page_size, priv_p->p_mdz_headersz);
	x_posix_memalign((void **)&priv_p->p_mdz_rhp, page_size, 2 * priv_p->p_mdz_segsz * priv_p->p_mdz_reqheadersz);

	mode = 0600;
	flags = priv_p->p_ssd_mode ? (O_RDWR | O_SYNC) : (O_RDWR | O_SYNC | O_DIRECT);

try_open:
	priv_p->p_fhandle = open(priv_p->p_bufdevice, flags, mode);
	if (priv_p->p_fhandle < 0) {
		nid_log_error("%s: cannot open %s, errno:%d", log_header, priv_p->p_bufdevice, errno);
		sleep(1);
		goto try_open;
	}

	if(util_bdev_getinfo(priv_p->p_fhandle, &priv_p->dev_size, &priv_p->sec_size, &priv_p->is_dev)) {
		nid_log_error("%s: failed to apply ioctl on device: %s", log_header, priv_p->p_bufdevice);
		close(priv_p->p_fhandle);
		free(priv_p);
		exit(1);
	}

	if (!priv_p->p_bfp) {
		bfp_p = x_calloc(1, sizeof(*bfp_p));
		bfp_setup.wc = priv_p->p_wc;
		bfp_setup.pp = priv_p->p_pp;
		bfp_setup.pagesz = priv_p->p_pagesz;
		bfp_setup.buffersz = priv_p->p_bufdevicesz - priv_p->p_mdz_size;
		bfp_setup.uuid = priv_p->p_uuid;
		bfp_setup.type = priv_p->p_bfp_type;
		bfp_setup.coalesce_ratio = priv_p->p_coalesce_ratio;
		bfp_setup.load_ratio_max = priv_p->p_load_ratio_max;
		bfp_setup.load_ratio_min = priv_p->p_load_ratio_min;
		bfp_setup.load_ctrl_level = priv_p->p_load_ctrl_level;
		bfp_setup.flush_delay_ctl = priv_p->p_flush_delay_ctl;
		bfp_setup.throttle_ratio = priv_p->p_throttle_ratio;
		bfp_setup.low_water_mark = priv_p->p_low_water_mark;
		bfp_setup.high_water_mark = priv_p->p_high_water_mark;
		bfp_initialization(bfp_p, &bfp_setup);
		priv_p->p_bfp = bfp_p;
	}

	/*
	 * read the header of mdz
	 */
	if (__bufdevice_rebuild_mdz_header(priv_p) == -2) {
		nid_log_error("%s: mdz version not compatible, stop nidserver", log_header);
		exit(1);
	}

	/*
	 * read the dev_header
	 */
	if (__bufdevice_rebuild_dev_header(priv_p) == -2) {
		nid_log_error("%s: dev version not compatible, stop nidserver", log_header);
		exit(1);
	}

	priv_p->p_flushing_st = x_calloc(priv_p->p_flush_nblocks, sizeof(struct bwc_flushing_st));
	for (i = 0; i < priv_p->p_flush_nblocks; i++)
		priv_p->p_flushing_st[i].f_end_offset = priv_p->p_mdz_size + priv_p->p_flush_blocksz * (i + 1) - 1;
	nid_log_debug("%s: flush_blocksz:%lu", log_header, priv_p->p_flush_blocksz);

	d2an_setup.allocator = priv_p->p_allocator;
	d2an_setup.set_id = ALLOCATOR_SET_BWC_D2AN;
	d2an_setup.seg_size = 128;
	d2an_p = &priv_p->p_d2an;
	d2an_initialization(d2an_p, &d2an_setup);

	d2bn_setup.allocator = priv_p->p_allocator;
	d2bn_setup.set_id = ALLOCATOR_SET_BWC_D2BN;
	d2bn_setup.seg_size = 128;
	d2bn_p = &priv_p->p_d2bn;
	d2bn_initialization(d2bn_p, &d2bn_setup);

	rmc_setup.allocator = priv_p->p_allocator;
	rmc_setup.seg_size = 128;
	rmc_setup.set_id = ALLOCATOR_SET_BWC_RCBWC_CB;
	rc_wc_cbn_initialization(&priv_p->p_rc_wc_cbn, &rmc_setup);

	rm_setup.allocator = priv_p->p_allocator;
	rm_setup.seg_size = 1024;
	rm_setup.set_id = ALLOCATOR_SET_BWC_RCBWC;
	rc_wcn_initialization(&priv_p->p_rc_wcn, &rm_setup);

	nn_setup.allocator = priv_p->p_allocator;
	nn_setup.seg_size = priv_p->p_mdz_segsz;
	nn_setup.set_id = ALLOCATOR_SET_BWC_RBN;
	nn_setup.alignment = priv_p->p_ssd_mode ? 0 : page_size;
	bwc_rbn_initialization(&priv_p->p_rbn, &nn_setup);

	max_vec = sysconf(_SC_IOV_MAX);
	nid_log_info("%s: max_vec:%d", log_header, max_vec);
	INIT_LIST_HEAD(&priv_p->p_chain_head);
	INIT_LIST_HEAD(&priv_p->p_frozen_chain_head);
	INIT_LIST_HEAD(&priv_p->p_freezing_chain_head);
	INIT_LIST_HEAD(&priv_p->p_inactive_head);
	INIT_LIST_HEAD(&priv_p->p_cfflush_head);
	INIT_LIST_HEAD(&priv_p->p_write_head);
	INIT_LIST_HEAD(&priv_p->p_trim_head);
	INIT_LIST_HEAD(&priv_p->p_read_finished_head);

	priv_p->p_data_flushed = 0;				// bytes
	priv_p->p_data_pkg_flushed = 0;			// bytes
	priv_p->p_nondata_pkg_flushed = 0;
	priv_p->p_read_counter = 0;

	struct lck_setup lck_setup;

	pthread_mutex_init(&priv_p->p_read_finish_lck, NULL);
	pthread_cond_init(&priv_p->p_read_post_cond, NULL);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	lck_initialization(&priv_p->p_lck_if, &lck_setup);
	lck_node_init(&priv_p->p_lnode);
	pthread_mutex_init(&priv_p->p_snapshot_lck, NULL);
	pthread_cond_init(&priv_p->p_snapshot_cond, NULL);
	pthread_mutex_init(&priv_p->p_new_trim_lck, NULL);
	pthread_cond_init(&priv_p->p_new_trim_cond, NULL);
	pthread_mutex_init(&priv_p->p_new_write_lck, NULL);
	pthread_cond_init(&priv_p->p_new_write_cond, NULL);
	pthread_mutex_init(&priv_p->p_new_read_lck, NULL);
	pthread_cond_init(&priv_p->p_new_read_cond, NULL);
	pthread_mutex_init(&priv_p->p_seq_lck, NULL);
	pthread_cond_init(&priv_p->p_seq_cond, NULL);
	pthread_mutex_init(&priv_p->p_occ_lck, NULL);
	pthread_cond_init(&priv_p->p_occ_cond, NULL);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _bwc_read_buf_thread, bwc_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _bwc_write_buf_thread, bwc_p);

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&thread_id, &attr, _bwc_trim_thread, bwc_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _bwc_release_thread, bwc_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _bwc_post_read_thread, bwc_p);
}

static void
bwc_start_ioinfo(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_start_ioinfo";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	nid_log_debug("%s: start ...", log_header);
	priv_p->p_start_count = 1;
	priv_p->p_req_num = 0;
	priv_p->p_req_len = 0;
}

static void
bwc_stop_ioinfo(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_stop_ioinfo";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	nid_log_debug("%s: start ...", log_header);
	priv_p->p_start_count = 0;
}

static void
bwc_check_ioinfo(struct bwc_interface *bwc_p, struct bwc_req_stat *cstat_p)
{
	char *log_header = "bwc_check_ioinfo";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	nid_log_debug("%s: start ...", log_header);
	cstat_p->is_running = priv_p->p_start_count;
	cstat_p->req_num = priv_p->p_req_num;
	cstat_p->req_len = priv_p->p_req_len;
}

static void
bwc_sync_rec_seq(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_sync_rec_seq";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	nid_log_debug("%s: start ...", log_header);
	__sync_lock_test_and_set(&priv_p->p_sync_rec_seq, 1);

	pthread_mutex_lock(&priv_p->p_new_write_lck);
	if (!priv_p->p_new_write_task) {
		priv_p->p_new_write_task = 1;
		pthread_cond_signal(&priv_p->p_new_write_cond);
	}
	pthread_mutex_unlock(&priv_p->p_new_write_lck);
}

static void
bwc_stop_sync_rec_seq(struct bwc_interface *bwc_p)
{
	char *log_header = "bwc_stop_sync_rec_seq";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;

	nid_log_debug("%s: start ...", log_header);
	__sync_lock_test_and_set(&priv_p->p_sync_rec_seq, 0);

}

static uint8_t
__bwc_get_vernum(char *ver_name)
{
	struct bwc_ver_cmp *vp = &bwc_ver_cmp_table[0];
	uint8_t ver_num = 0;

	while (vp->ver_name != NULL) {
		if (!memcmp(ver_name, vp->ver_name, BWC_VERSZ)) {
			ver_num = vp->ver_num;
			break;
		}
		vp++;
	}
	return ver_num;
}

int
__bwc_ver_cmp_check(char *ver_name1, char *ver_name2)
{
	uint8_t ver_num1, ver_num2;
	int ret = 0;

	ver_num1 = __bwc_get_vernum(ver_name1);
	ver_num2 = __bwc_get_vernum(ver_name2);
	if (ver_num1 != ver_num2)
		ret = 1;
	return ret;
}

static int
bwc_get_list_info(struct bwc_interface *bwc_p, char *res_uuid, struct umessage_bwc_information_list_resp_stat *stat_p)
{
	char *log_header = "bwc_get_list_info";
	struct	bwc_private *priv_p = bwc_p->bw_private;
	struct	bwc_chain *chain_p;
	struct	bwc_channel *chan_p;
	int	to_write_counter, to_read_counter, channel_counter;
//	struct	list_head to_read_head, to_write_head, write_head, resp_head;
	struct	request_node *rn_p, *rn_p2;
	int	rc = 0;

	to_write_counter = 0;
	to_read_counter = 0;
	channel_counter = 0;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		nid_log_warning("%s: comapring %s with %s",log_header, chain_p->c_resource, res_uuid);
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			nid_log_warning("%s: found chain %s", log_header, chain_p->c_resource);
			stat_p->um_found_it = 1;
			list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
				nid_log_warning("%s: No.%d channel in this chain, sac: %p", log_header, channel_counter, chan_p->c_sac);
				list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &chan_p->c_read_head, r_read_list) {
					to_read_counter++;
				}
				nid_log_warning("%s: there are %u request in to_read_list", log_header, to_read_counter);
				list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &chan_p->c_write_head, r_write_list) {
					to_write_counter++;
				}
				nid_log_warning("%s: there are %u request in to_write_list", log_header, to_read_counter);

				channel_counter++;
				stat_p->um_to_write_list_counter += to_write_counter;
				stat_p->um_to_read_list_counter += to_read_counter;
			}
			rc = 1;
			break;
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &priv_p->p_write_head, r_write_list) {
		stat_p->um_write_list_counter++;
	}
	nid_log_warning("%s: there are %u request in write__list", log_header, stat_p->um_write_list_counter);
	stat_p->um_write_vec_list_counter = priv_p->p_write_vec_counter;
	nid_log_warning("%s: there are %u request in write_vec_list", log_header, stat_p->um_write_vec_list_counter);
	stat_p->um_write_busy = priv_p->p_write_busy;
	nid_log_warning("%s: write busy %u", log_header, stat_p->um_write_busy);
	stat_p->um_read_busy = priv_p->p_read_busy;
	nid_log_warning("%s: read busy %u", log_header, stat_p->um_read_busy);

	return rc;
}
struct bwc_operations bwc_op = {
	.bw_create_channel = bwc_create_channel,
	.bw_prepare_channel = bwc_prepare_channel,
	.bw_recover = bwc_recover,
	.bw_get_poolsz = bwc_get_poolsz,
	.bw_get_pagesz = bwc_get_pagesz,
	.bw_pread = bwc_pread,
	.bw_read_list = bwc_read_list,
	.bw_write_list = bwc_write_list,
	.bw_trim_list = bwc_trim_list,
	.bw_flush_update = bwc_flush_update,
	.bw_chan_inactive = bwc_chan_inactive,
	.bw_stop = bwc_stop,
	.bw_start_page = bwc_start_page,
	.bw_end_page = bwc_end_page,
	.bw_get_chan_stat = bwc_get_chan_stat,
	.bw_get_stat = bwc_get_stat,
	.bw_get_info_stat = bwc_get_info_stat,
	.bw_fast_flush = bwc_fast_flush,
	.bw_get_fast_flush = bwc_get_fast_flush,
	.bw_stop_fast_flush = bwc_stop_fast_flush,
	.bw_sync_rec_seq = bwc_sync_rec_seq,
	.bw_stop_sync_rec_seq = bwc_stop_sync_rec_seq,
	.bw_get_block_occupied = bwc_get_block_occupied,
	.bw_get_release_page_counter = bwc_get_release_page_counter,
	.bw_get_flush_page_counter = bwc_get_flush_page_counter,
	.bw_vec_start = bwc_vec_start,
	.bw_vec_stop = bwc_vec_stop,
	.bw_get_vec_stat = bwc_get_vec_stat,
	.bw_get_vec_stat_by_uuid = bwc_get_vec_stat_by_uuid,
	.bw_get_all_vec_stat = bwc_get_all_vec_stat,
	.bw_get_uuid = bwc_get_uuid,
	.bw_freeze_chain_stage1 = bwc_freeze_chain_stage1,
	.bw_freeze_chain_stage2 = bwc_freeze_chain_stage2,
	.bw_destroy_chain = bwc_destroy_chain,
	.bw_unfreeze_snapshot = bwc_unfreeze_snapshot,
	.bw_dropcache_start = bwc_dropcache_start,
	.bw_dropcache_stop = bwc_dropcache_stop,
	.bw_get_rw = bwc_get_rw,
	.bw_get_delay = bwc_get_delay,
	.bw_get_throughput = bwc_get_throughput,
	.bw_reset_throughput = bwc_reset_throughput,
	.bw_reset_read_counter = bwc_reset_read_counter,
	.bw_get_read_counter = bwc_get_read_counter,
	.bw_get_coalesce_index = bwc_get_coalesce_index,
	.bw_reset_coalesce_index = bwc_reset_coalesce_index,
	.bw_get_cutoff = bwc_get_cutoff,
	.bw_destroy = bwc_destroy,
	.bw_write_response = bwc_write_response,
	.bw_post_initialization = bwc_post_initialization,
	.bw_update_water_mark = bwc_update_water_mark,
	.bw_update_delay_level = bwc_update_delay_level,
	.bw_start_ioinfo = bwc_start_ioinfo,
	.bw_stop_ioinfo = bwc_stop_ioinfo,
	.bw_check_ioinfo = bwc_check_ioinfo,
	.bw_get_list_info = bwc_get_list_info,
};

int
bwc_initialization(struct bwc_interface *bwc_p, struct bwc_setup *setup)
{
	char *log_header = "bwc_initialization";
	struct bwc_private *priv_p;
	struct pp_interface *pp_p;
	uint64_t hdz_size;

	nid_log_warning("%s: start ...", log_header);
	bwc_p->bw_op = &bwc_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bwc_p->bw_private = priv_p;
	priv_p->p_mdz_recover = 1;

	priv_p->p_dio_alignment = BWC_LOGICAL_BLOCK_SIZE;
	priv_p->p_bufdevicesz = (uint64_t)setup->bufdevicesz << 20;

	if (priv_p->p_mdz_recover) {
		priv_p->p_mdz_headersz = priv_p->p_dio_alignment;	// header of mdz
		priv_p->p_mdz_reqheadersz = BWC_REQ_HEADERSZ;		// header of a request
		priv_p->p_mdz_segsz = BWC_MDZ_SEGSZ;			// number of req headers
		priv_p->p_mdz_segsz_mask = ~((uint64_t)priv_p->p_mdz_segsz - 1);
		priv_p->p_mdz_seg_num = ((uint64_t)(priv_p->p_bufdevicesz) >> 12) / priv_p->p_mdz_segsz;// larger enough
		priv_p->p_mdz_size = 2 * priv_p->p_mdz_headersz +
			(priv_p->p_mdz_seg_num * priv_p->p_mdz_segsz) * priv_p->p_mdz_reqheadersz;	// size in bytes
	}
	priv_p->p_first_seg_off = priv_p->p_mdz_size + BWC_DEV_HEADERSZ;

	hdz_size = priv_p->p_bufdevicesz - priv_p->p_mdz_size;
	priv_p->p_flush_blocksz = BWC_BLOCK_SIZE;
	priv_p->p_flush_nblocks = hdz_size / priv_p->p_flush_blocksz;
	priv_p->p_bufdevicesz = priv_p->p_mdz_size + priv_p->p_flush_nblocks * priv_p->p_flush_blocksz;

	priv_p->p_pause = 1;	// not ready to start yet
	strcpy(priv_p->p_uuid, setup->uuid);
	priv_p->p_allocator = setup->allocator;
	priv_p->p_wc = setup->wc;
	priv_p->p_pp = setup->pp;
	priv_p->p_tp = setup->tp;
	priv_p->p_srn = setup->srn;
	priv_p->p_rw_sync = setup->rw_sync;
	priv_p->p_two_step_read = setup->two_step_read;
	priv_p->p_do_fp = setup->do_fp;
	priv_p->p_max_flush_size = setup->max_flush_size;
	priv_p->p_lstn = setup->lstn;
	priv_p->p_read_max_retry_time = 8;
	priv_p->p_ssd_mode = setup->ssd_mode;
	priv_p->p_busy = 0;
	priv_p->p_mqtt = setup->mqtt;
	priv_p->p_record_delay = 0;
	priv_p->p_read_busy = 0;
	priv_p->p_write_busy = 0;
	priv_p->p_write_vec_counter = 0;

	pp_p = priv_p->p_pp;
	priv_p->p_pagesz = pp_p->pp_op->pp_get_pagesz(pp_p);
	priv_p->p_segsz = pp_p->pp_op->pp_get_pagesz(pp_p);
	if (priv_p->p_segsz > BWC_MAX_SEQSZ)
		priv_p->p_segsz = BWC_MAX_SEQSZ;

	strcpy(priv_p->p_bufdevice, setup->bufdevice);

	priv_p->p_bfp = setup->bfp;
	priv_p->p_bfp_type = setup->bfp_type;
	priv_p->p_coalesce_ratio = setup->coalesce_ratio;
	priv_p->p_load_ratio_max = setup->load_ratio_max;
	priv_p->p_load_ratio_min = setup->load_ratio_min;
	priv_p->p_load_ctrl_level = setup->load_ctrl_level;
	priv_p->p_flush_delay_ctl = setup->flush_delay_ctl;
	priv_p->p_throttle_ratio = setup->throttle_ratio;
	priv_p->p_low_water_mark = setup->low_water_mark;
	priv_p->p_high_water_mark = setup->high_water_mark;


	priv_p->p_write_delay_first_level = setup->write_delay_first_level * priv_p->p_flush_nblocks / 100;
	if (priv_p->p_write_delay_first_level) {
		priv_p->p_write_delay_second_level = setup->write_delay_second_level * priv_p->p_flush_nblocks / 100;
		priv_p->p_write_delay_first_level_increase_interval = setup->write_delay_first_level_max_us / (priv_p->p_write_delay_first_level - priv_p->p_write_delay_second_level);
		priv_p->p_write_delay_second_level_increase_interval = setup->write_delay_second_level_max_us / priv_p->p_write_delay_second_level;
		priv_p->p_write_delay_second_level_start_ms = setup->write_delay_first_level_max_us;
	}

	return 0;
}

