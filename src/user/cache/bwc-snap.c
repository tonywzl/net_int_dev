/*
 * bwc-snap.c
 *
 * snapshot related function for write back cache
 */


#include "bwc.h"

extern void __bwc_release_block(struct bwc_interface *, uint64_t);

static void
__bwc_destroy_chain_res(struct bwc_chain *chain_p)
{
	char *log_header = "__bwc_destroy_chain_res";
	struct bwc_interface *bwc_p = chain_p->c_bwc;
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct ddn_interface *ddn_p = &chain_p->c_ddn;
	struct bse_interface *bse_p = &chain_p->c_bse;
	struct bfe_interface *bfe_p = &chain_p->c_bfe;
	struct bre_interface *bre_p = &chain_p->c_bre;

	// release associated memory resource
	nid_log_debug("%s: close bfe %p...", log_header, bfe_p);
	bfe_p->bf_op->bf_close(bfe_p);
	nid_log_debug("%s: close bre %p...", log_header, bre_p);
	bre_p->br_op->br_dropcache_start(bre_p, priv_p->p_rw_sync);
	bre_p->br_op->br_close(bre_p);
	nid_log_debug("%s: close bse %p...", log_header, bse_p);
	bse_p->bs_op->bs_close(bse_p);
	nid_log_debug("%s: destroy ddn %p...", log_header, ddn_p);
	ddn_p->d_op->d_destroy(ddn_p);
}

void
__bwc_destroy_chain(struct bwc_chain *chain_p)
{
	char *log_header = "__bwc_destroy_chain";
	struct bwc_channel *chan_p, *chan1_p;

	nid_log_warning("%s: destroy chain %p (%s) start", log_header, chain_p, chain_p->c_resource);
	list_for_each_entry_safe(chan_p, chan1_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
		list_del(&chan_p->c_list);
		assert(list_empty(&chan_p->c_read_head));
		assert(list_empty(&chan_p->c_write_head));
		assert(list_empty(&chan_p->c_resp_head));
		free(chan_p);
	}

	pthread_mutex_destroy(&chain_p->c_lck);
	pthread_mutex_destroy(&chain_p->c_rlck);
	pthread_mutex_destroy(&chain_p->c_wlck);

	free((void *)chain_p);
	nid_log_warning("%s: destroy chain %p (%s) done", log_header, chain_p, chain_p->c_resource);
}


// clone a new chain, the function can only be called when hold lock
struct bwc_chain *
__bwc_clone_chain(struct bwc_interface *bwc_p, struct bwc_chain *cur_chain_p)
{
	char *log_header = "__bwc_clone_chain";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bwc_chain *chain_p = NULL;
	struct ddn_interface *ddn_p;
	struct ddn_setup ddn_setup;
	struct bse_interface *bse_p;
	struct bse_setup bse_setup;
	struct bfe_interface *bfe_p;
	struct bfe_setup bfe_setup;
	struct bre_interface *bre_p;
	struct bre_setup bre_setup;

	chain_p = x_calloc(1, sizeof(*chain_p));
	INIT_LIST_HEAD(&chain_p->c_chan_head);
	INIT_LIST_HEAD(&chain_p->c_search_head);
	pthread_mutex_init(&chain_p->c_rlck, NULL);
	pthread_mutex_init(&chain_p->c_wlck, NULL);
	pthread_mutex_init(&chain_p->c_lck, NULL);
	chain_p->c_bwc = cur_chain_p->c_bwc;
	chain_p->c_rw = cur_chain_p->c_rw;
	chain_p->c_rw_handle = cur_chain_p->c_rw_handle;
	chain_p->c_target_sync = cur_chain_p->c_target_sync;
	chain_p->c_rc = cur_chain_p->c_rc;
	chain_p->c_rc_handle = cur_chain_p->c_rc_handle;
	strncpy(chain_p->c_resource, cur_chain_p->c_resource, NID_MAX_PATH - 1);
	chain_p->c_id = cur_chain_p->c_id;

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
	bfe_setup.rc = chain_p->c_rc;
	bfe_setup.rc_handle = chain_p->c_rc_handle;
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
	bre_setup.sequence_mode = priv_p->p_rw_sync == 0 && priv_p->p_ssd_mode == 1 ? 1 : 0;
	bre_setup.wc = priv_p->p_wc;
	bre_setup.wc_chain_handle = (void*)chain_p;
	bre_initialization(bre_p, &bre_setup);

	chain_p->c_busy = 0;
	chain_p->c_active = 1;
	chain_p->c_freeze = 1;

	list_add_tail(&chain_p->c_list, &priv_p->p_chain_head);

	nid_log_warning("%s: chain:%p add to chain list", log_header, chain_p);

	return chain_p;
}

static int
__bwc_freeze_chain_stage1(struct bwc_interface *bwc_p, char *res_uuid, struct bwc_chain **chain_pp)
{
	char *log_header = "__bwc_freeze_chain_stage1";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	int got_it = 0, rc = 0;

	nid_log_warning("%s: start uuid:%s...", log_header, res_uuid);
	pthread_mutex_lock(&priv_p->p_snapshot_lck);
	while (1) {
		if (priv_p->p_busy) {
			pthread_mutex_unlock(&priv_p->p_snapshot_lck);
			usleep(100*1000);
			pthread_mutex_lock(&priv_p->p_snapshot_lck);
		} else {
			priv_p->p_busy = 1;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_snapshot_lck);
	if (priv_p->p_stop)
		goto out;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_freezing_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			got_it = 1;
			break;
		}
	}

	if (!got_it) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_frozen_chain_head, c_list) {
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				got_it = 1;
				break;
			}
		}
	}

	if (got_it) {
		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		nid_log_warning("%s: the resource is in freeze state (uuid:%s)",
				log_header, res_uuid);
		rc = 2;
		goto out;
	}

	/*
	 * Now, I'm the only thread which is doing snapshot.
	 * And the write_release_thread got paused until informed
	 * search the channel of this res_uuid from both inactive and active channel lists
	 */
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			got_it = 1;
			break;
		}
	}

	if (!got_it) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				got_it = 1;
				break;
			}
		}
	}

	if (!got_it) {
		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		nid_log_warning("%s: the resource does not exist (uuid:%s)",
			log_header, res_uuid);
		rc = 1;
		goto out;
	}

	if (chain_p->c_busy) {
		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		nid_log_warning("%s: the resource busy (uuid:%s)",
			log_header, res_uuid);
		rc = 3;
		goto out;
	}

	priv_p->p_lck_if.l_op->l_uplock(&priv_p->p_lck_if, &priv_p->p_lnode);
	priv_p->p_freeze_chain = chain_p;
	chain_p->c_busy = 1;
	chain_p->c_freeze = 1;
	priv_p->p_lck_if.l_op->l_downlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	// let sac stop to work
	struct bwc_channel *chan_p;
	list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
		struct sac_interface *sac_p = chan_p->c_sac;
		if (sac_p && sac_p->sa_private) {
			sac_p->sa_op->sa_prepare_stop(sac_p);
		}
	}

	// make sure sac already stop to work
	while (1) {
		uint32_t sac_active_cnt = 0;
		list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
			struct sac_interface *sac_p = chan_p->c_sac;
			if (sac_p && sac_p->sa_private) {
				sac_active_cnt++;
				break;
			}
		}
		if (sac_active_cnt) {
			// there may have some request not write to buffer device when we try stop sac,
			// it need wake up bwc_write_buf_thread to process request
			pthread_cond_signal(&priv_p->p_new_write_cond);
			usleep(100*1000);
		} else {
			break;
		}
	}

	nid_log_warning("%s: make _bwc_write_buf_thread to pause ...", log_header);

	pthread_mutex_lock(&priv_p->p_snapshot_lck);
	priv_p->p_snapshot_to_pause = 1;

	pthread_mutex_lock(&priv_p->p_new_write_lck);
	if (!priv_p->p_new_write_task) {
		priv_p->p_new_write_task = 1;
		pthread_cond_signal(&priv_p->p_new_write_cond);
	}
	pthread_mutex_unlock(&priv_p->p_new_write_lck);

	while (!priv_p->p_stop && !priv_p->p_snapshot_pause)
		pthread_cond_wait(&priv_p->p_snapshot_cond, &priv_p->p_snapshot_lck);
	pthread_mutex_unlock(&priv_p->p_snapshot_lck);
	if(priv_p->p_stop) {
		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		goto out;
	}

	nid_log_warning("%s: _bwc_write_buf_thread paused", log_header);

	priv_p->p_lck_if.l_op->l_uplock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_del(&chain_p->c_list);
	list_add_tail(&chain_p->c_list, &priv_p->p_freezing_chain_head);

	// clone a new chain with new bre/bse/bfe
	if (!chain_pp) {
		__bwc_clone_chain(bwc_p, chain_p);
	} else {
		*chain_pp = chain_p;
	}

	priv_p->p_lck_if.l_op->l_downlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	// wake up _bwc_write_buf_thread
	pthread_mutex_lock(&priv_p->p_snapshot_lck);
	priv_p->p_snapshot_to_pause = 0;
	priv_p->p_snapshot_pause = 0;
	pthread_cond_broadcast(&priv_p->p_snapshot_cond);
	pthread_mutex_unlock(&priv_p->p_snapshot_lck);

out:

	pthread_mutex_lock(&priv_p->p_snapshot_lck);
	priv_p->p_busy = 0;
	pthread_mutex_unlock(&priv_p->p_snapshot_lck);

	return rc;
}

static int
__bwc_freeze_chain_stage2(struct bwc_interface *bwc_p, char *res_uuid)
{
	char *log_header = "__bwc_freeze_chain_stage2";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	int got_it = 0, rc = 0;

	nid_log_warning("%s: start uuid:%s...", log_header, res_uuid);
	pthread_mutex_lock(&priv_p->p_snapshot_lck);
	while (1) {
		if (priv_p->p_busy) {
			pthread_mutex_unlock(&priv_p->p_snapshot_lck);
			usleep(100*1000);
			pthread_mutex_lock(&priv_p->p_snapshot_lck);
		} else {
			priv_p->p_busy = 1;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_snapshot_lck);
	if (priv_p->p_stop)
		goto out;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_freezing_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			got_it = 1;
			break;
		}
	}

	if (!got_it) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_frozen_chain_head, c_list) {
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
				nid_log_warning("%s: the resource (uuid:%s) already in freeze stage 2",
					log_header, res_uuid);
				rc = 2;
				goto out;
			}
		}
	}

	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	if (!got_it) {
		nid_log_warning("%s: the resource (uuid:%s) not in freeze stage 1, cannot do freeze stage 2",
			log_header, res_uuid);
		rc = 1;
		goto out;
	}

	nid_log_warning("%s: snapshot target freeze seq:%lu", log_header, chain_p->c_seq_assigned);

	// _bfe_flush_thread may in wait cond state, wake it up
	struct bfe_interface *bfe_p = &chain_p->c_bfe;
	uint64_t seq_to_flush = 0;
	while(seq_to_flush < chain_p->c_seq_assigned) {
		seq_to_flush = chain_p->c_seq_flushed + 1;
		bfe_p->bf_op->bf_flush_seq(bfe_p, seq_to_flush);

		pthread_mutex_lock(&priv_p->p_seq_lck);
		while (!priv_p->p_stop && (seq_to_flush > chain_p->c_seq_flushed)) {
			pthread_cond_wait(&priv_p->p_seq_cond, &priv_p->p_seq_lck);
		}
		pthread_mutex_unlock(&priv_p->p_seq_lck);

		// we can only release block, when block already in write cache and flushed to targer device
		if(seq_to_flush <= priv_p->p_seq_flushed && seq_to_flush <= priv_p->p_rec_seq_flushed) {
			__bwc_release_block(bwc_p, seq_to_flush);
		}
	}

	// destroy bfe/bre/bse/ddn associated with chain
	__bwc_destroy_chain_res(chain_p);

	// chain has been flushed, move it to frozen chain list
	priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_del(&chain_p->c_list);
	list_add_tail(&chain_p->c_list, &priv_p->p_frozen_chain_head);
	priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);

out:

	pthread_mutex_lock(&priv_p->p_snapshot_lck);
	priv_p->p_busy = 0;
	pthread_mutex_unlock(&priv_p->p_snapshot_lck);

	return rc;
}

/*
 * Input:
 * 	res_uuid:
 */
static int
__bwc_freeze_chain(struct bwc_interface *bwc_p, char *res_uuid, struct bwc_chain **chain_pp)
{
	char *log_header = "__bwc_freeze_chain";
	int rc;

	rc = __bwc_freeze_chain_stage1(bwc_p, res_uuid, chain_pp);
	if(rc) {
		nid_log_error("%s: snapshot target freeze stage1 failed with rc: %d", log_header, rc);
		return rc;
	}

	rc = __bwc_freeze_chain_stage2(bwc_p, res_uuid);
	if(rc) {
		nid_log_error("%s: snapshot target freeze stage2 failed with rc: %d", log_header, rc);
	}

	return rc;
}


int
bwc_destroy_chain(struct bwc_interface *bwc_p, char *res_uuid)
{
	// release associated memory resource
	struct bwc_chain *chain_p = NULL;
	int rc;

	rc = __bwc_freeze_chain(bwc_p, res_uuid, &chain_p);
	if(rc)
		return -1;

	__bwc_destroy_chain(chain_p);
	return 0;
}


/*
 * Input:
 *      res_uuid:
 *      sid: snapshot id
 */
int
bwc_unfreeze_snapshot(struct bwc_interface *bwc_p, char *res_uuid)
{
	char *log_header = "bwc_unfreeze_snapshot";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	int got_it = 0, rc = 0;
	int quit_freeze = 0;

	nid_log_warning("%s: start uuid:%s...", log_header, res_uuid);

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_frozen_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			got_it = 1;
			break;
		}
	}

	if (got_it) {
		priv_p->p_lck_if.l_op->l_uplock(&priv_p->p_lck_if, &priv_p->p_lnode);
		list_del(&chain_p->c_list);
		priv_p->p_lck_if.l_op->l_downlock(&priv_p->p_lck_if, &priv_p->p_lnode);

		__bwc_destroy_chain(chain_p);
		quit_freeze = 2;

		// set it, we need make matche chain in chain list to active
		got_it = 0;
	} else {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_freezing_chain_head, c_list) {
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
				quit_freeze = 1;
				goto out;
			}
		}
	}

	// make chain quit freeze state
	priv_p->p_lck_if.l_op->l_uplock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			chain_p->c_freeze = 0;
			got_it = 1;
			break;
		}
	}

	if (!got_it) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				chain_p->c_freeze = 0;
				got_it = 1;
				break;
			}
		}
	}
	priv_p->p_lck_if.l_op->l_downlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

out:

	if (quit_freeze == 2) {
		// trigger unfreezed chain flush target device
		struct bfe_interface *bfe_p = &chain_p->c_bfe;
		bfe_p->bf_op->bf_flush_seq(bfe_p, priv_p->p_seq_flushable);

		nid_log_warning("%s: resource %s quit freeze state", log_header, res_uuid);
		rc = 0;
	} else if (quit_freeze == 1) {
		nid_log_warning("%s: resource %s is in freezing progress state", log_header, res_uuid);
		rc = 2;
	} else {
		if(got_it) {
			nid_log_warning("%s: resource %s  not in freeze state, no need to unfreeze", log_header, res_uuid);
			rc = 3;
		} else {
			rc = 1;
			nid_log_warning("%s: resource %s not found", log_header, res_uuid);
		}
	}

	return rc;
}

int
bwc_freeze_chain_stage1(struct bwc_interface *bwc_p, char *res_uuid)
{
	int rc;

	rc = __bwc_freeze_chain_stage1(bwc_p, res_uuid, NULL);

	return rc;
}

int
bwc_freeze_chain_stage2(struct bwc_interface *bwc_p, char *res_uuid)
{
	int rc;

	rc = __bwc_freeze_chain_stage2(bwc_p, res_uuid);

	return rc;
}
