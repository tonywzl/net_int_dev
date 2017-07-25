#include "bwc.h"

/*
 * Algorithm:
 * 	- This ought to be per resource for the universality of merging bwc from live nodes (only specific resources need to be migrated).
 * 	- It should to be a two phase process:
 * 		1. Slow down IO and start fast flush.
 * 		2. Cut off IO when all flushed(or less than a threshold). Then confirm all flushed again.
 * 		3. Remove the bwc channel from the list, clean it up(release pages, stop the engines and memory recycle, etc.) and destroy it.
 * 		4. Create new bwc channel associated with the sac in the new bwc.
 * 		5. The outer layer will determine when to destroy the entire bwc.
 */
static void *
_bwc_chan_fflush_thread(void *p)
{
	char *log_header = "_bwc_chan_fflush_thread";
	struct bwc_chain *chain_p = (struct bwc_chain *)p, *chain2_p;
	struct bwc_interface *bwc_p = chain_p->c_bwc;
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	struct bwc_channel *chan_p, *chan1_p;
	uint64_t wait_wr_cnt = BWC_C_WR_CNT_THRESHD, wait_rd_cnt = BWC_C_RD_CNT_THRESHD;
	uint64_t last_total_wr_recv_cnt = 0, last_total_rd_recv_cnt = 0;
	uint64_t total_wr_recv_cnt, total_rd_recv_cnt, total_rd_done_cnt;
	int wr_waited = 0, rd_waited = 0, ff_waited = 0;
	char *chan_uuid =  chain_p->c_resource;
	char *bwc_uuid = priv_p->p_uuid;
	int got_it = 0;

wait:
	for (;;) {
		/*
		 * When priv_p->p_pause == 1, the bwc is doing recovery here.
		 * In theory we shouldn't run into this case as the outer layer is supposed to
		 * call bwc recover command and confirm it recovered before calling the fast flush
		 * command, just a double confirmation here.
		 */
		if (priv_p->p_pause) {
			nid_log_warning("%s: bwc:%s yet to be recovered, cannot start fast flush for channel:%s!",
					log_header, bwc_uuid, chan_uuid);
			usleep(5000000);
			continue;
		}

		total_wr_recv_cnt = 0;
		total_rd_recv_cnt = 0;
		priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
			total_wr_recv_cnt += chan_p->c_wr_recv_counter;
			total_rd_recv_cnt += chan_p->c_rd_recv_counter;
		}
		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

		pthread_mutex_lock(&chain_p->c_wlck);
		if (!wr_waited) {
			if ((total_wr_recv_cnt - last_total_wr_recv_cnt) > wait_wr_cnt)
				chain_p->c_wr_slowdown_step += BWC_C_WR_SLOWDOWN_STEP; // lengthen the step of slowing down write IO
			else
				wr_waited = 1; // the write requests received during last period are supposed to be few enough
			last_total_wr_recv_cnt = total_wr_recv_cnt;
		}
		pthread_mutex_unlock(&chain_p->c_wlck);

		pthread_mutex_lock(&chain_p->c_rlck);
		if (!rd_waited) {
			if ((total_rd_recv_cnt - last_total_rd_recv_cnt) > wait_rd_cnt)
				chain_p->c_rd_slowdown_step += BWC_C_RD_SLOWDOWN_STEP; // lengthen the step of slowing down read IO
			else
				rd_waited = 1;
			last_total_rd_recv_cnt = total_rd_recv_cnt;
		}
		pthread_mutex_unlock(&chain_p->c_rlck);

		if (wr_waited && rd_waited)
			break;

		usleep(5000000);
	}

	/*
	 * wait_wr_cnt == 0 and wait_rd_cnt == 0 and waited means IO cut off, no more write/read coming
	 */
	if (wait_wr_cnt || wait_rd_cnt) {
		/*
		 * now the write IO is supposed to be slow enough and there should not be much unflushed data.
		 * cut off the write IO thoroughly and then wait for all data be be flushed
		 */
		__sync_lock_test_and_set(&chain_p->c_cutoff, 1);

		/*
		 * wait again for IO cut off and all data flushed
		 */
		wait_wr_cnt = wait_rd_cnt = 0;
		wr_waited = rd_waited = 0;
		nid_log_warning("%s: channel:%s IO cut off, wait again...", log_header, chan_uuid);
		goto wait;
	}

	/*
	 * If priv_p->p_seq_assigned == 0, there is no write IO yet.
	 * If not, wait for all data flushed.
	 */
//	if (priv_p->p_seq_assigned)
//		(void)bfe_p->bf_op->bf_wait_flush_page_counter(bfe_p, 0, NULL);
	for (;;) {
		pthread_mutex_lock(&chain_p->c_lck);
		if (chain_p->c_seq_flushed == chain_p->c_seq_assigned)
			ff_waited = 1;
		pthread_mutex_unlock(&chain_p->c_lck);
		if (ff_waited)
			break;

		usleep(1000000);
	}
	nid_log_warning("%s: channel:%s all data flushed.", log_header, chan_uuid);

	/*
	 * wait for all read done
	 */
	for (;;) {
		total_rd_done_cnt = 0;
		total_rd_recv_cnt = 0;
		priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		list_for_each_entry(chan_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
			total_rd_done_cnt += chan_p->c_rd_done_counter;
			total_rd_recv_cnt += chan_p->c_rd_recv_counter;
		}
		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

		if (total_rd_done_cnt == total_rd_recv_cnt) {
			/*
			 * safe enough here to set fast flush state to done here
			 * for outer layer to determine to switch the resource to the new bwc.
			 * there will be a nidmanager command that queries this state.
			 * if this bwc channel is already gone when the query comes, we still consider it as flush done.
			 */
			__sync_lock_test_and_set(&chain_p->c_fflush_state, BWC_C_FFLUSH_DONE);
			break;
		}

		usleep(5000000);
	}
	nid_log_warning("%s: channel:%s all read done.", log_header, chan_uuid);

	/*
	 * remove the bwc channel from bwc, close the engines and destroy it.
	 * for safety, look for the channel in the list again.
	 * notice that even if this is the last channel of the bwc, don't destroy the bwc here.
	 * the outer layer will determine when to destroy the bwc.
	 */
	bwc_stop_fast_flush(bwc_p, chan_uuid);

	priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
//	list_del(&chain_p->c_fflush_list);
	list_for_each_entry(chain2_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
		if (!strcmp(chain2_p->c_resource, chain_p->c_resource)) {
			got_it = 1;
			list_del(&chain2_p->c_list);
			break;
		}
	}

	if (!got_it) {
		list_for_each_entry(chain2_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
			if (!strcmp(chain2_p->c_resource, chain_p->c_resource)) {
				got_it = 1;
				list_del(&chain2_p->c_list);
				break;
			}
		}
	}

	if (!got_it) {
		nid_log_warning("%s: channel:%s not found in the list!",
				log_header, chain_p->c_resource);
	}

	while (chain_p->c_busy) {
		priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		usleep(10000);
		priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	}
	chain_p->c_busy = 1;
	list_for_each_entry_safe(chan_p, chan1_p, struct bwc_channel, &chain_p->c_chan_head, c_list) {
		priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		if (chan_p->c_sac) {
			/*
			 * This channel could be inactive because agent hasn't connected yet (sac hasn't accepted any new channel).
			 * If owner sac not NULL, it should not use this bwc any more.
			 */
			chan_p->c_sac->sa_op->sa_remove_wc(chan_p->c_sac);
			nid_log_warning("%s: channel:%s removed from bwc:%s.", log_header, chan_uuid, bwc_uuid);
		}
		priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	}
	priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	__bwc_destroy_chain(chain_p);

	return NULL;
}

static void
__bwc_chan_fflush(struct bwc_chain *chain_p)
{
	pthread_attr_t attr;
	pthread_t thread_id;

	pthread_mutex_lock(&chain_p->c_wlck);
//	chan_p->c_fflush_state = BWC_C_FFLUSH_DOING;
	chain_p->c_wr_slowdown_step = BWC_C_WR_SLOWDOWN_STEP;
	pthread_mutex_unlock(&chain_p->c_wlck);

	pthread_mutex_lock(&chain_p->c_rlck);
	chain_p->c_rd_slowdown_step = BWC_C_RD_SLOWDOWN_STEP;
	pthread_mutex_unlock(&chain_p->c_rlck);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _bwc_chan_fflush_thread, chain_p);
}

int
bwc_fast_flush(struct bwc_interface *bwc_p, char *resource)
{
	char *log_header = "bwc_fast_flush";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p, *chain_p2;
	int got_it = 0, got_it2 = 0;

	if (resource) {
		nid_log_debug("%s: start(channel:%s)...", log_header, resource);
		priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
			if (!strcmp(chain_p->c_resource, resource)) {
				nid_log_debug("%s: %s found in active chain list.",
						log_header, resource);
				got_it = 1;
				break;
			}
		}

		if (!got_it) {
			list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
				if (!strcmp(chain_p->c_resource, resource)) {
					nid_log_debug("%s: %s found in inactive chain list.",
							log_header, resource);
					got_it = 1;
					break;
				}
			}
		}

		/* check if it already in the fast flushing list */
		if (got_it) {
			list_for_each_entry(chain_p2, struct bwc_chain, &priv_p->p_cfflush_head, c_fflush_list) {
				if (!strcmp(chain_p2->c_resource, resource)) {
					nid_log_debug("%s: %s found in fast flush chain list.",
							log_header, resource);
					got_it2 = 1;
					break;
				}
			}

			if (!got_it2) {
				__sync_lock_test_and_set(&chain_p->c_fflush_state, BWC_C_FFLUSH_DOING);
				list_add_tail(&chain_p->c_fflush_list, &priv_p->p_cfflush_head);

				/* Channel specific fast flush only means to prepare for bwc merging */
				__bwc_chan_fflush(chain_p);
			}
		}
		priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	}

	priv_p->p_fast_flush = 1;
	return 0;
}

int
bwc_get_fast_flush(struct bwc_interface *bwc_p, char *resource)
{
	char *log_header = "bwc_get_fast_flush";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	int rc = -1;

	nid_log_debug("%s: start ...", log_header);
	/* Getting the fast flush state is always channel specific. */
	if (!resource) {
		nid_log_error("%s: no resource given!", log_header);
		goto out;
	}

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_cfflush_head, c_fflush_list) {
		if (!strcmp(chain_p->c_resource, resource)) {
			rc = (int)chain_p->c_fflush_state;
			break;
		}
	}
	/*
	 * Not found in fflush list, try bwc channel lists to see if it is already gone.
	 * If yes, consider it as BWC_C_FFLUSH_DONE;
	 */
	if (rc == -1) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_chain_head, c_list) {
			if (!strcmp(chain_p->c_resource, resource)) {
				rc = (int)chain_p->c_fflush_state;
				break;
			}
		}
	}
	if (rc == -1) {
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
			if (!strcmp(chain_p->c_resource, resource)) {
				rc = (int)chain_p->c_fflush_state;
				break;
			}
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	if (rc == -1)
		rc = BWC_C_FFLUSH_DONE;
out:
	return rc;
}

int
bwc_stop_fast_flush(struct bwc_interface *bwc_p, char *resource)
{
	char *log_header = "bwc_stop_fast_flush";
	struct bwc_private *priv_p = bwc_p->bw_private;
	struct bwc_chain *chain_p;
	int stop_all = 1;

	nid_log_debug("%s: start ...", log_header);
	if (resource) {
		priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_cfflush_head, c_fflush_list) {
			if (!strcmp(chain_p->c_resource, resource)) {
				list_del(&chain_p->c_fflush_list);
				__sync_lock_test_and_set(&chain_p->c_fflush_state, BWC_C_FFLUSH_NONE);
				break;
			}
		}

		if (!list_empty(&priv_p->p_cfflush_head)) // there are still other channels that need fast flush
			stop_all = 0;
		priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	}

	if (stop_all)
		priv_p->p_fast_flush = 0;

	return 0;
}
