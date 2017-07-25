/*
 * dsrec.c
 * 	Implementation of Device Space Reclaim Module
 */

#include <stdlib.h>
#include <pthread.h>

#include "nid_log.h"
#include "list.h"
#include "cdn_if.h"
#include "blksn_if.h"
#include "rc_if.h"
#include "dsmgr_if.h"
#include "dsrec_if.h"
#include "list_sort.h"
#include "fpc_if.h"

struct dsrec_private {
	struct rc_interface	*p_rc;
	struct cdn_interface	*p_cdn;
	struct blksn_interface	*p_blksn;
	struct dsmgr_interface	*p_dsmgr;
	struct fpc_interface	*p_fpc;
	struct list_head	p_space_head;				// in space order
	struct list_head	p_to_reclaim_head;			// nodes to reclaim
	struct list_head	p_reclaiming_head;			// nodes being reclaimed
	struct list_head	p_lru_head[NID_SIZE_FP_WGT];		// in time order and different weight
	uint64_t		p_rc_size;				// the whole read cache size
	struct dsrec_stat	p_dsrec_stat;				// statistics info of dsrec.
	pthread_mutex_t		p_list_lck;
	pthread_cond_t		p_list_cond;
};

/*
 * Algorithm:
 * 	Insert a one-block node to the lru list.
 *
 */
static void
dsrec_insert_block(struct dsrec_interface *dsrec_p, struct content_description_node *np)
{
	struct dsrec_private *priv_p = (struct dsrec_private *)dsrec_p->sr_private;
	struct fpc_interface *fpc_p = priv_p->p_fpc;
	struct content_description_node *cur_np, *next_np;
	struct list_head *cur_head = NULL;
	int i=0;

	uint8_t fp_wgt = fpc_p->fpc_op->fpc_get_wgt_from_fp(fpc_p, (const char*)np->cn_fp);
	pthread_mutex_lock(&priv_p->p_list_lck);
	list_add_tail(&np->cn_flist, &priv_p->p_lru_head[fp_wgt]);
	np->cn_is_lru = 1;

	priv_p->p_dsrec_stat.p_lru_nr[fp_wgt]++;
	priv_p->p_dsrec_stat.p_lru_total_nr++;

	/* to reclaim if necessary */
	if (priv_p->p_dsrec_stat.p_lru_total_nr >= priv_p->p_dsrec_stat.p_lru_max) {
		assert(priv_p->p_dsrec_stat.p_lru_total_nr == priv_p->p_dsrec_stat.p_lru_max);
		i = 10;
		while (i > 0) {
			cur_head = NULL;
			for (fp_wgt = 0; fp_wgt < NID_SIZE_FP_WGT; fp_wgt++) {
				if (!list_empty(&priv_p->p_lru_head[fp_wgt])) {
					cur_head = &priv_p->p_lru_head[fp_wgt];
					break;
				}
			}
			assert(cur_head != NULL);

			list_for_each_entry_safe(cur_np, next_np, struct content_description_node, cur_head, cn_flist) {
				assert(cur_np->cn_is_lru);// it should be in lru list
				list_del(&cur_np->cn_flist);// removed from lru list
				cur_np->cn_is_lru = 0;
				list_add_tail(&cur_np->cn_flist, &priv_p->p_to_reclaim_head);// to be reclaimed

				priv_p->p_dsrec_stat.p_lru_nr[fp_wgt]--;
				priv_p->p_dsrec_stat.p_lru_total_nr--;

				if (--i <= 0) {
					break;
				}
			}
		}
		pthread_cond_signal(&priv_p->p_list_cond);
	}

	pthread_mutex_unlock(&priv_p->p_list_lck);
}

/*
 * Algorithm:
 * 	Move touched block to the tail of lru list
 *
 * Input:
 * 	np: must be one block space which has already been inserted to lru list before
 * 	Do nothing otherise.
 */
static void
dsrec_touch_block(struct dsrec_interface *dsrec_p, struct content_description_node *old_np, char *new_fp)
{
	struct dsrec_private *priv_p = (struct dsrec_private *)dsrec_p->sr_private;
	struct fpc_interface *fpc_p = priv_p->p_fpc;

	uint8_t old_wgt = fpc_p->fpc_op->fpc_get_wgt_from_fp(fpc_p, old_np->cn_fp);
	uint8_t new_wgt = fpc_p->fpc_op->fpc_get_wgt_from_fp(fpc_p, new_fp);
	new_wgt = new_wgt > old_wgt ? new_wgt : old_wgt;

	pthread_mutex_lock(&priv_p->p_list_lck);
	if (old_np->cn_is_lru) {
		list_del(&old_np->cn_flist);
		fpc_p->fpc_op->fpc_set_wgt_to_fp(fpc_p, old_np->cn_fp, new_wgt);
		list_add_tail(&old_np->cn_flist, &priv_p->p_lru_head[new_wgt]);

		priv_p->p_dsrec_stat.p_lru_nr[old_wgt]--;
		priv_p->p_dsrec_stat.p_lru_nr[new_wgt]++;
	}
	pthread_mutex_unlock(&priv_p->p_list_lck);
}

static struct dsrec_stat
dsrec_get_dsrec_stat(struct dsrec_interface *dsrec_p)
{
	return ((struct dsrec_private *)dsrec_p->sr_private)->p_dsrec_stat;
}



struct dsrec_operations dsrec_op = {
	.sr_insert_block = dsrec_insert_block,
	.sr_touch_block = dsrec_touch_block,
	.sr_get_dsrec_stat = dsrec_get_dsrec_stat,
};

/*
 * The comparison function @cmp must return a negative value if @a
 * should sort before @b, and a positive value if @a should sort after
 * @b. If @a and @b are equivalent, and their original relative
 * ordering is to be preserved, @cmp must return 0.
*/
static int
sort_cmp(void  *priv, struct list_head *a, struct list_head *b){
	struct content_description_node *cd_np, *cd_np2;
	uint64_t index1, index2;

	assert(!priv);
	assert(!list_empty(a)||!list_empty(b));
	cd_np = list_entry(a, struct content_description_node, cn_flist);
	cd_np2 = list_entry(b, struct content_description_node, cn_flist);
	index1 = (uint64_t)cd_np->cn_data;
	index2 = (uint64_t)cd_np2->cn_data;
	if (index1 < index2) {
		return 1;
	} else if(index1 > index2) {
		return -1;
	} else {
		return 0;
	}
}

/*
 * Algorithm:
 * 	Try to reclaim contingous blocks as many as possible
 */
static void *
_dsrec_reclaim_thread(void *p)
{
	char *log_header = "_dsrec_reclaim_thread";
	struct dsrec_interface *dsrec_p = (struct dsrec_interface *)p;
	struct dsrec_private *priv_p = (struct dsrec_private *)dsrec_p->sr_private;
	struct rc_interface *rc_p = priv_p->p_rc;
	struct dsmgr_interface *dsmgr_p = priv_p->p_dsmgr;
	struct blksn_interface *blksn_p = priv_p->p_blksn;
	struct cdn_interface *cdn_p = priv_p->p_cdn;

	struct content_description_node *cd_np, *cd_np2;
	struct block_size_node *bs_np;

	nid_log_notice("%s: start ...", log_header);

next_reclaim:
	pthread_mutex_lock(&priv_p->p_list_lck);
	while (list_empty(&priv_p->p_to_reclaim_head)) {
		pthread_cond_wait(&priv_p->p_list_cond, &priv_p->p_list_lck);
	}
	list_splice_tail_init(&priv_p->p_to_reclaim_head, &priv_p->p_reclaiming_head);
	pthread_mutex_unlock(&priv_p->p_list_lck);

	list_for_each_entry(cd_np, struct content_description_node, &priv_p->p_reclaiming_head, cn_flist) {
		rc_p->rc_op->rc_drop_block(rc_p, cd_np);			// update read cache which is using this block
	}

	list_sort(NULL, &priv_p->p_reclaiming_head, sort_cmp);
	bs_np = NULL;
	list_for_each_entry_safe(cd_np, cd_np2, struct content_description_node, &priv_p->p_reclaiming_head, cn_flist) {
		if (!bs_np) {
			bs_np = blksn_p->bsn_op->bsn_get_node(blksn_p);
			bs_np->bsn_size = 1;	// one block
			bs_np->bsn_index = (uint64_t)cd_np->cn_data;

		} else if ((uint64_t)cd_np->cn_data == bs_np->bsn_index + bs_np->bsn_size) {
			bs_np->bsn_size++;

		} else {
			dsmgr_p->dm_op->dm_put_space(dsmgr_p, bs_np);
			bs_np = blksn_p->bsn_op->bsn_get_node(blksn_p);
			bs_np->bsn_size = 1;	// one block
			bs_np->bsn_index = (uint64_t)cd_np->cn_data;
		}
		list_del(&cd_np->cn_flist);
		__sync_fetch_and_add(&priv_p->p_dsrec_stat.p_rec_nr, 1);

		assert(cd_np->cn_is_ondisk);
		cdn_p->cn_op->cn_put_node(cdn_p, cd_np);
	}
	if ( bs_np ) {
		dsmgr_p->dm_op->dm_put_space(dsmgr_p, bs_np);
	}
	goto next_reclaim;

	return NULL;
}

int
dsrec_initialization(struct dsrec_interface *dsrec_p, struct dsrec_setup *setup)
{
	char *log_header = "dsrec_initialization";
	struct dsrec_private *priv_p;
	pthread_attr_t attr;
	pthread_t thread_id;
	uint8_t fp_wgt = 0;

	nid_log_notice("%s: start ...", log_header);
	assert(setup);
	dsrec_p->sr_op = &dsrec_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	dsrec_p->sr_private = priv_p;

	priv_p->p_cdn = setup->cdn;
	priv_p->p_blksn = setup->blksn;
	priv_p->p_dsmgr = setup->dsmgr;
	priv_p->p_rc = setup->rc;
	priv_p->p_rc_size = setup->rc_size;
	priv_p->p_fpc = setup->fpc;
	priv_p->p_dsrec_stat.p_lru_max = (priv_p->p_rc_size * 9) / 10;
	INIT_LIST_HEAD(&priv_p->p_space_head);
	INIT_LIST_HEAD(&priv_p->p_to_reclaim_head);
	INIT_LIST_HEAD(&priv_p->p_reclaiming_head);
	for (fp_wgt=0; fp_wgt<NID_SIZE_FP_WGT; fp_wgt++){
		INIT_LIST_HEAD(&priv_p->p_lru_head[fp_wgt]);
	}

	pthread_mutex_init(&priv_p->p_list_lck, NULL);
	pthread_cond_init(&priv_p->p_list_cond, NULL);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _dsrec_reclaim_thread, dsrec_p);
	return 0;
}
