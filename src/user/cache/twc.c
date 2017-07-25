/*
 * twc.c
 * 	Implementation of Write Through Cache (twc) Mode Module
 */

#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid_shared.h"
#include "lck_if.h"
#include "allocator_if.h"
#include "umpk_twc_if.h"
#include "sac_if.h"
#include "ds_if.h"
#include "rw_if.h"
#include "rc_if.h"
#include "wc_if.h"
#include "twc_if.h"
#include "d2an_if.h"
#include "d2bn_if.h"
#include "ddn_if.h"
#include "srn_if.h"
#include "fpn_if.h"
#include "lstn_if.h"
#include "rc_wc_cbn.h"
#include "rc_wcn.h"
#include "smc_if.h"
#include "pp_if.h"

#define TWC_MAX_RESOURCE	128

#define	TWC_PRIV_INDEX		0
#define	TWC_REQNODE_INDEX	1

#define	TWC_RDNODE_INDEX	0
#define	TWC_SREQNODE_INDEX	1

#define TWC_MAX_IOV  512

struct twc_chain {
	struct list_head	c_list;
	struct list_head	c_chan_head;
	struct list_head	c_search_head;
	char			c_resource[NID_MAX_UUID];

	struct pp_page_node	*c_page;
	struct ddn_interface	c_ddn;
	uint8_t			c_active;
	uint8_t			c_busy;
	uint16_t		c_id;

	struct twc_interface	*c_twc;
	struct rc_interface	*c_rc;
	void			*c_rc_handle;
	struct rw_interface	*c_rw;
	pthread_mutex_t		c_lck;
};

struct twc_channel {
	struct twc_chain	*c_chain;
	struct rc_interface	*c_rc;
	void			*c_rc_handle;
	struct rw_interface	*c_rw;
	void			*c_rw_handle;
	struct list_head	c_list;
	char			c_resource[NID_MAX_UUID];
	struct sac_interface	*c_sac;
	struct ddn_interface	c_ddn;
	struct list_head	c_read_head;
	struct list_head	c_write_head;
	struct list_head	c_trim_head;
	struct list_head	c_resp_head;
	uint64_t		c_all_write_counter;	// accumulated number of all write requests received
	uint64_t		c_all_read_counter;	// accumulated number of all read requests received
	uint64_t		c_all_trim_counter;	// accumulated number of all trim requests received
	uint16_t		c_id;
	uint8_t			c_active;
};

struct twc_private {
	char				p_uuid[NID_MAX_UUID];
	struct allocator_interface	*p_allocator;
	struct d2an_interface		p_d2an;
	struct d2bn_interface		p_d2bn;
	struct list_head		p_chain_head;
	struct list_head		p_inactive_head;
	struct lck_interface		p_lck_if;
	struct lck_node			p_lnode;		// for list p_chain_head, p_inactive_head
	pthread_mutex_t			p_new_write_lck;
	pthread_cond_t			p_new_write_cond;
	pthread_mutex_t			p_new_read_lck;
	pthread_cond_t			p_new_read_cond;
	pthread_mutex_t			p_new_trim_lck;
	pthread_cond_t			p_new_trim_cond;
	char				*p_resources[TWC_MAX_RESOURCE];
	struct tp_interface		*p_tp;
	struct srn_interface		*p_srn;				// sub request node
	struct lstn_interface		*p_lstn;
	struct rc_wc_cbn_interface     	p_rc_wc_cbn;
	struct rc_wcn_interface        	p_rc_wcn;
	struct list_head		p_write_head;
	struct list_head		p_trim_head;
	uint64_t			p_resp_counter;
	uint8_t				p_do_fp;
	uint8_t				p_new_write_task;
	uint8_t				p_new_read_task;
	uint8_t				p_new_trim_task;
	uint8_t				p_stop;
	uint8_t				p_pause;
	uint8_t				p_write_stop;
	uint8_t				p_trim_stop;
	uint8_t				p_read_stop;
	uint64_t			p_seq_assigned;
	uint64_t			p_seq_flushed;
	struct pp_interface		*p_pp;
};

static void
twc_end_page(struct twc_interface *twc_p, void *io_handle, void *page_p, int do_buffer)
{
	char *log_header = "twc_end_page";
	struct twc_private *priv_p = twc_p->tw_private;
	struct pp_page_node *pnp = (struct pp_page_node *)page_p;
	struct pp_interface *pp_p = priv_p->p_pp;

	nid_log_debug("%s: start (pnp:%p) ...", log_header, pnp);

	(void)io_handle;
	(void)do_buffer;
	pp_p->pp_op->pp_free_node(pp_p, pnp);
}

static void
twc_get_info_stat(struct twc_interface *twc_p, struct umessage_twc_information_resp_stat *stat_p)
{
	struct twc_private *priv_p = twc_p->tw_private;

	strcpy(stat_p->um_twc_uuid, priv_p->p_uuid);
	stat_p->um_twc_uuid_len = strlen(stat_p->um_twc_uuid);
}

static void
twc_get_stat(struct twc_interface *twc_p, struct io_stat *sp)
{
	struct twc_private *priv_p = twc_p->tw_private;
	struct smc_stat_record *sr_p;
	struct sac_stat *stat_p;
	struct twc_chain *chain_p = NULL;

	sp->s_buf_avail	= 0;
	INIT_LIST_HEAD(&sp->s_inactive_head);

	sp->s_io_type_bio = WC_TYPE_TWC;
	sp->s_cur_write_block = priv_p->p_seq_assigned;
	sp->s_cur_flush_block = priv_p->p_seq_flushed;
	sp->s_seq_assigned = priv_p->p_seq_assigned;
	sp->s_seq_flushed = priv_p->p_seq_flushed;
	sp->s_buf_avail = 0;

	list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_inactive_head, c_list) {
		stat_p = x_calloc(1, sizeof(*stat_p));
		sr_p = x_calloc(1, sizeof(*sr_p));
		sr_p->r_type = NID_REQ_STAT_SAC;
		sr_p->r_data = (void *)stat_p;
		stat_p->sa_io_type = IO_TYPE_BUFFER;
		stat_p->sa_uuid = chain_p->c_resource;
		stat_p->sa_stat = NID_STAT_INACTIVE;
		list_add(&sr_p->r_list, &sp->s_inactive_head);
	}

	if (!priv_p->p_pause)
		sp->s_stat = NID_STAT_ACTIVE;
	else
		sp->s_stat = NID_STAT_INACTIVE;
}


// get matched channel and return its buffer io runtime info
static void
twc_get_rw(struct twc_interface *twc_p, char *res_uuid, struct twc_rw_stat *stat_p)
{
	struct twc_private *priv_p = twc_p->tw_private;
	char *log_header = "twc_get_rw";
	struct twc_channel *chan_p;
	struct twc_chain *chain_p = NULL;
	int rc = 1;

	// search match uuid in active/inactive channel
	nid_log_warning("%s: start (uuid:%s) ...", log_header, res_uuid);

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_chain_head, c_list) {
		nid_log_warning("%s: comapring %s with %s", chain_p->c_resource, res_uuid, log_header);
		if (!strcmp(chain_p->c_resource, res_uuid)) {
			rc = 0;
			break;
		}
	}

	if (rc) {
		list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_inactive_head, c_list) {
			nid_log_warning("%s: comapring %s with %s", chain_p->c_resource, res_uuid, log_header);
			if (!strcmp(chain_p->c_resource, res_uuid)) {
				rc = 0;
				break;
			}
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	// find the matched channel chain, get all channel's buffer io runtime info
	if (!rc) {
		stat_p->all_write_counts = 0;
		stat_p->all_read_counts = 0;
		pthread_mutex_lock(&chain_p->c_lck);
		list_for_each_entry(chan_p, struct twc_channel, &chain_p->c_chan_head, c_list) {
			stat_p->all_write_counts += chan_p->c_all_write_counter;
			stat_p->all_read_counts += chan_p->c_all_read_counter;
		}
		pthread_mutex_unlock(&chain_p->c_lck);

		stat_p->res = 1;
		nid_log_warning("%s: write:%lu, read:%lu",
				log_header, stat_p->all_write_counts, stat_p->all_read_counts);
	} else {
		stat_p->res = 0;
	}
}

static char *
twc_get_uuid(struct twc_interface *twc_p)
{
	struct twc_private *priv_p = twc_p->tw_private;
	return priv_p->p_uuid;
}

/*
 * Algorithm:
 * 	if the resource doesn't exist, get a non-used id
 * Return:
 * 	>0: an valid id associated with the resouce
 * 	0: cannot find an non-used id
 */
static uint16_t
__get_resource_id(struct twc_private *priv_p, char *res_p)
{
	int i;
	uint16_t new_id = 0;

	for (i = 1; i < TWC_MAX_RESOURCE; i++) {
		if (priv_p->p_resources[i] && !strcmp(priv_p->p_resources[i], res_p)) {
			return i;
		}
		if (!new_id && !priv_p->p_resources[i]) {
			new_id = i;
		}
	}

	if (new_id) {
		/* got a new resource */
		priv_p->p_resources[new_id] = strdup(res_p);
	}
	return new_id;
}

/*
 * create a channel with owner, it should be active
 * create a channel w/o owner, it should be inactive
 */
static void *
twc_create_channel(struct twc_interface *twc_p, void *owner, struct wc_channel_info *wc_info, char *res_p, int *new)
{
	char *log_header = "twc_create_channel";
	struct twc_private *priv_p = (struct twc_private *)twc_p->tw_private;
	struct twc_channel *chan_p = NULL;
	struct twc_chain *chain_p = NULL;
	struct ddn_interface *ddn_p;
	struct ddn_setup ddn_setup;
	struct rw_interface *rw_p = (struct rw_interface *)wc_info->wc_rw;
	struct rc_interface *rc_p = (struct rc_interface *)wc_info->wc_rc;
	void *rc_handle = wc_info->wc_rc_handle;
	int got_it = 0;

	nid_log_notice("%s: exportname:%s", log_header, res_p);

	// p_pause == 1 means it not ready to start yet, while owner need a active channel
	// which make it impossible, can not do it, leave now
	if (priv_p->p_pause && owner)
		goto out;

tryagain:

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	// find resource name in active channel list, if find one, mark new as 0 and jump out
	list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_chain_head, c_list) {
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

	list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_inactive_head, c_list) {
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

	// find channel in inactive list, and owner exist means need to make it active
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
	}

out:
	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_rw = rw_p;
	chan_p->c_rw_handle = rw_p->rw_op->rw_create_worker(rw_p, wc_info->wc_rw_exportname, wc_info->wc_rw_sync, wc_info->wc_rw_direct_io,
			wc_info->wc_rw_alignment, wc_info->wc_rw_dev_size);
	chan_p->c_rc = rc_p;
	chan_p->c_rc_handle = rc_handle;
	strncpy(chan_p->c_resource, res_p, NID_MAX_PATH - 1);
	chan_p->c_sac = (struct sac_interface *)owner;
	INIT_LIST_HEAD(&chan_p->c_read_head);
	INIT_LIST_HEAD(&chan_p->c_write_head);
	INIT_LIST_HEAD(&chan_p->c_resp_head);
	INIT_LIST_HEAD(&chan_p->c_trim_head);

	if (*new) {
		chain_p = x_calloc(1, sizeof(*chain_p));
		chan_p->c_chain = chain_p;
		INIT_LIST_HEAD(&chain_p->c_chan_head);
		INIT_LIST_HEAD(&chain_p->c_search_head);
		pthread_mutex_init(&chain_p->c_lck, NULL);
		chain_p->c_twc = twc_p;
		chain_p->c_rw = rw_p;
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
		ddn_p = &chain_p->c_ddn;
		ddn_setup.allocator = priv_p->p_allocator;
		ddn_setup.set_id = ALLOCATOR_SET_TWC_DDN;
		ddn_setup.seg_size = 1024;
		ddn_initialization(ddn_p, &ddn_setup);

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
twc_prepare_channel(struct twc_interface *twc_p, struct wc_channel_info *wc_info, char *res_p)
{
	char *log_header = "twc_prepare_channel";
	struct twc_private *priv_p = twc_p->tw_private;
	struct twc_channel *chan_p = NULL;
	int new_chan = 0;

	nid_log_notice("%s: resource:%s", log_header, res_p);
	assert(priv_p);
	chan_p = twc_create_channel(twc_p, NULL, wc_info, res_p, &new_chan);
	
	// if channel failed to create or it's a new created channel, trigger assert
	assert(!chan_p || new_chan);
	return chan_p;
}

static void
twc_recover(struct twc_interface *twc_p)
{
	char *log_header = "twc_recover";
	struct twc_private *priv_p = twc_p->tw_private;

	nid_log_notice("%s: start, twc uuid:%s ...", log_header, priv_p->p_uuid);

	// when twc object initialized, it always stay in pause status, 
	// after recover, it'll bring to active state
	priv_p->p_pause = 0;
}

static void
prepare_search_list(struct twc_interface *twc_p, struct request_node *rn_p)	 {
	struct twc_private *priv_p = (struct twc_private *)twc_p->tw_private;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct sub_request_node *sreq_p;

	INIT_LIST_HEAD(&rn_p->r_head);

	sreq_p = srn_p->sr_op->sr_get_node(srn_p);
	sreq_p->sr_buf = rn_p->r_resp_buf_1;
	sreq_p->sr_offset = rn_p->r_offset;
	sreq_p->sr_len = rn_p->r_resp_len_1;
	list_add_tail(&sreq_p->sr_list, &rn_p->r_head);

	if (rn_p->r_resp_len_1 < rn_p->r_len) {
		sreq_p = srn_p->sr_op->sr_get_node(srn_p);
		sreq_p->sr_buf = rn_p->r_resp_buf_2;
		sreq_p->sr_offset = rn_p->r_offset + rn_p->r_resp_len_1;
		sreq_p->sr_len = rn_p->r_len - rn_p->r_resp_len_1;
		list_add_tail(&sreq_p->sr_list, &rn_p->r_head);
	}
}

// read data from channel
static ssize_t
twc_pread(struct twc_interface *twc_p, void *io_handle, void *buf, size_t count, off_t offset)
{
	struct twc_private *priv_p = twc_p->tw_private;
	struct twc_channel *chan_p = (struct twc_channel *)io_handle;
	struct rw_interface *rw_p = chan_p->c_rw;
	ssize_t rc = 0;
	assert(priv_p);

	rc = rw_p->rw_op->rw_pread(rw_p, chan_p->c_rw_handle, buf, count, offset);
	if(rc > 0) {
		chan_p->c_all_read_counter += rc;
	}

	return rc;
}

// send read req list to __twc_read_buf_thread, called by wc_read_list <-- bio_read_list,
// <-- io_read_list <-- sac_pickup_request when get NID_REQ_READ/IO_TYPE_BUFFER
static void
twc_read_list(struct twc_interface *twc_p, void *io_handle, struct list_head *read_head)
{
	struct twc_private *priv_p = twc_p->tw_private;
	struct twc_channel *chan_p = (struct twc_channel *)io_handle;

	// add read req list to channel read list
	pthread_mutex_lock(&chan_p->c_chain->c_lck);
	list_splice_tail_init(read_head, &chan_p->c_read_head);
	pthread_mutex_unlock(&chan_p->c_chain->c_lck);

	pthread_mutex_lock(&priv_p->p_new_read_lck);
	// wake up __twc_read_buf_thread to read data
	if (!priv_p->p_new_read_task) {
		priv_p->p_new_read_task = 1;
		pthread_cond_signal(&priv_p->p_new_read_cond);
	}
	pthread_mutex_unlock(&priv_p->p_new_read_lck);
}

// send write req list to _twc_write_target_thread, wake it up to flush data to target device
static void
twc_write_list(struct twc_interface *twc_p, void *wc_handle, struct list_head *write_head, int write_counter)
{
	struct twc_private *priv_p = twc_p->tw_private;
	struct twc_channel *chan_p = (struct twc_channel *)wc_handle;

	pthread_mutex_lock(&chan_p->c_chain->c_lck);
	list_splice_tail_init(write_head, &chan_p->c_write_head);
	chan_p->c_all_write_counter += write_counter;
	pthread_mutex_unlock(&chan_p->c_chain->c_lck);

	pthread_mutex_lock(&priv_p->p_new_write_lck);
	if (!priv_p->p_new_write_task) {
		priv_p->p_new_write_task = 1;
		pthread_cond_signal(&priv_p->p_new_write_cond);
	}
	pthread_mutex_unlock(&priv_p->p_new_write_lck);
}

// send trim req list to _twc_trim_target_thread, wake it up to send trim command to target device
static void
twc_trim_list(struct twc_interface *twc_p, void *wc_handle, struct list_head *trim_head, int trim_counter)
{
	struct twc_private *priv_p = twc_p->tw_private;
	struct twc_channel *chan_p = (struct twc_channel *)wc_handle;

	pthread_mutex_lock(&chan_p->c_chain->c_lck);
	list_splice_tail_init(trim_head, &chan_p->c_trim_head);
	chan_p->c_all_trim_counter += trim_counter;
	pthread_mutex_unlock(&chan_p->c_chain->c_lck);

	pthread_mutex_lock(&priv_p->p_new_trim_lck);
	if (!priv_p->p_new_trim_task) {
		priv_p->p_new_trim_task = 1;
		pthread_cond_signal(&priv_p->p_new_trim_cond);
	}
	pthread_mutex_unlock(&priv_p->p_new_trim_lck);
}

static int
twc_chan_inactive(struct twc_interface *twc_p, void *wc_handle)
{
	char *log_header = "twc_chan_inactive";
	struct twc_private *priv_p = twc_p->tw_private;
	struct twc_channel *chan_p = (struct twc_channel *)wc_handle;
	struct twc_chain *chain_p = chan_p->c_chain;

	nid_log_notice("%s: start (%s)...", log_header, chain_p->c_resource);

	priv_p->p_lck_if.l_op->l_wlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	assert(list_empty(&chan_p->c_read_head));
	list_del(&chan_p->c_list);

	if (list_empty(&chain_p->c_chan_head) && chain_p->c_busy == 0) {
		if (chain_p->c_active) {
			list_del(&chain_p->c_list);
			chain_p->c_active = 0;
			list_add_tail(&chain_p->c_list, &priv_p->p_inactive_head);
		}
	}
	priv_p->p_lck_if.l_op->l_wunlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	free(chan_p);
	return 0;
}

// set p_stop flag, then send signal to work threads and wait stop
// status of work threads
static int
twc_stop(struct twc_interface *twc_p)
{
	char *log_header = "twc_stop";
	struct twc_private *priv_p = twc_p->tw_private;

	priv_p->p_stop = 1;

	pthread_mutex_lock(&priv_p->p_new_read_lck);
	pthread_cond_signal(&priv_p->p_new_read_cond);
	pthread_mutex_unlock(&priv_p->p_new_read_lck);

	pthread_mutex_lock(&priv_p->p_new_write_lck);
	pthread_cond_signal(&priv_p->p_new_write_cond);
	pthread_mutex_unlock(&priv_p->p_new_write_lck);

	pthread_mutex_lock(&priv_p->p_new_trim_lck);
	pthread_cond_signal(&priv_p->p_new_trim_cond);
	pthread_mutex_unlock(&priv_p->p_new_trim_lck);

	while (!priv_p->p_read_stop || !priv_p->p_write_stop || !priv_p->p_trim_stop) {
		nid_log_notice("%s: waiting to stop (_read_stop:%u, write_stop:%u,  trim_stop:%u...",
			log_header, priv_p->p_read_stop, priv_p->p_write_stop, priv_p->p_trim_stop);
		sleep(1);
	}

	return 0;
}

static void
__twc_do_read(struct tp_jobheader *job)
{
	struct request_node *rn_p = (struct request_node *)job;
	struct twc_interface *twc_p = (struct twc_interface *)rn_p->r_io_obj;
	struct twc_private *priv_p = (struct twc_private *)twc_p->tw_private;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct twc_channel *chan_p = (struct twc_channel *)rn_p->r_io_handle;
	struct rc_interface *rc_p = chan_p->c_rc;
	void *rc_handle_p = chan_p->c_rc_handle;
	struct rw_interface *rw_p = chan_p->c_rw;
	struct sac_interface *sac_p = chan_p->c_sac;
	struct fpn_interface *fpn_p = rw_p->rw_op->rw_get_fpn_p(rw_p);
	struct nid_request *ir_p = &rn_p->r_ir;
	struct sub_request_node *sreq_p, *sreq_p2;
	struct list_head fp_head;
	void *align_buf;
	int do_fp = priv_p->p_do_fp;
	size_t align_count;
	off_t align_offset;

	assert(ir_p->cmd == NID_REQ_READ);

	// prepare search range in rn_p->r_head
	prepare_search_list(twc_p, rn_p);

	// search range in read cache
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
	 * Now, r_head contains all segments which are not found from rc
	 * We have to read them through rw
	 */
	INIT_LIST_HEAD(&fp_head);
	list_for_each_entry_safe(sreq_p, sreq_p2, struct sub_request_node, &rn_p->r_head, sr_list) {

		/*
		 * Update read cache
		 */
		if (rc_p && do_fp) {
			rw_p->rw_op->rw_pread_fp(rw_p, chan_p->c_rw_handle, sreq_p->sr_buf, sreq_p->sr_len,
				sreq_p->sr_offset, &fp_head, &align_buf, &align_count, &align_offset);

			if (align_buf) {
				rc_p->rc_op->rc_update(rc_p, rc_handle_p, align_buf, align_count,
					align_offset, &fp_head);
				free(align_buf);
			} else {
				rc_p->rc_op->rc_update(rc_p, rc_handle_p, sreq_p->sr_buf,
					sreq_p->sr_len, sreq_p->sr_offset, &fp_head);
			}

			/* Free finger print nodes */
			while (!list_empty(&fp_head)) {
				struct fp_node *fpnd = list_first_entry(&fp_head, struct fp_node, fp_list);
				list_del(&fpnd->fp_list);
				fpn_p->fp_op->fp_put_node(fpn_p, fpnd);
			}

		} else if (rc_p) {
			assert(0);
		        // Rick: TODO

		} else {
			rw_p->rw_op->rw_pread(rw_p, chan_p->c_rw_handle, sreq_p->sr_buf,
				sreq_p->sr_len, sreq_p->sr_offset);

		}

		list_del(&sreq_p->sr_list);
		srn_p->sr_op->sr_put_node(srn_p, sreq_p);
	}

	sac_p->sa_op->sa_add_response_node(sac_p, rn_p);
}

/*
 * Algorithm:
 * 	Read from read cache
 */
static void *
_twc_read_buf_thread(void *p)
{
	char *log_header = "_twc_read_buf_thread";
	struct twc_interface *twc_p = (struct twc_interface *)p;
	struct twc_private *priv_p = (struct twc_private *)twc_p->tw_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct twc_channel *chan_p;
	struct twc_chain *chain_p = NULL;
	struct list_head read_head;
	struct request_node *rn_p, *rn_p2;
	int did_it = 0;

	nid_log_notice("%s: start ...", log_header);

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
	list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_chain_head, c_list) {
		pthread_mutex_lock(&chain_p->c_lck);
		if (chain_p->c_busy == 1) {
			pthread_mutex_unlock(&chain_p->c_lck);
			continue;
		}

		list_for_each_entry(chan_p, struct twc_channel, &chain_p->c_chan_head, c_list) {
			if (list_empty(&chan_p->c_read_head))
				continue;

			INIT_LIST_HEAD(&read_head);
			list_splice_tail_init(&chan_p->c_read_head, &read_head);
			if (list_empty(&read_head))
				continue;

			/*
			 * Got something to read for this channel
			 */
			list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &read_head, r_read_list) {
				list_del(&rn_p->r_read_list);
				rn_p->r_header.jh_do = __twc_do_read;
				rn_p->r_header.jh_free = NULL;
				rn_p->r_io_obj = (void *)twc_p;
				tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)rn_p);
			}
			did_it = 1;
		}
		pthread_mutex_unlock(&chain_p->c_lck);
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	goto next_read;

out:
	priv_p->p_read_stop = 1;
	return NULL;
}

// flush to target disk, update read cache
static void
_twc_flush_to_target(struct twc_channel *chan_p, struct iovec *iov, int vec_counter, uint64_t d_offset_low) {
	char *log_header = "_twc_flush_to_target";
	void	*align_buf = NULL;
	size_t	align_count = 0;
	off_t	align_offset = 0;
	void	*rc_handle_p;
	struct rc_interface	*rc_p;
	struct list_head 	fp_head;
	struct rw_interface *rw_p;
	int ret;

	rw_p = chan_p->c_rw;
	rc_p = chan_p->c_rc;
	rc_handle_p = chan_p->c_rc_handle;

	INIT_LIST_HEAD(&fp_head);
	nid_log_notice("%s: prepare write %p", log_header, iov[0].iov_base);
	if(rc_p) {
		ret = rw_p->rw_op->rw_pwritev_fp(rw_p, chan_p->c_rw_handle, iov, vec_counter, d_offset_low,
											 &fp_head, &align_buf, &align_count, &align_offset);
	} else {
		ret = rw_p->rw_op->rw_pwritev(rw_p, chan_p->c_rw_handle, iov, vec_counter, d_offset_low);
	}

	if(ret < 0) {
		int err_no = errno;
		char *err_str = strerror(err_no);
		nid_log_notice("%s: write failed with return code %d, error code %d, error message %s",
					   log_header, ret, err_no, err_str);
		nid_log_notice("%s: write length %d with offset %lu",  log_header, vec_counter, d_offset_low);
		assert(0);
	} else {
		nid_log_notice("%s: write success", log_header);
	}

	if(rc_p) {
		if (align_buf) {
			rc_p->rc_op->rc_update(rc_p, rc_handle_p, align_buf, align_count, align_offset, &fp_head);
			free(align_buf);
		} else {
			rc_p->rc_op->rc_updatev(rc_p, rc_handle_p, iov, vec_counter, d_offset_low, &fp_head);
		}
	}

	// free finger print nodes, that just finish write
	struct fpn_interface *fpn_p = rw_p->rw_op->rw_get_fpn_p(rw_p);
	while (!list_empty(&fp_head)) {
		struct fp_node *fpnd = list_first_entry(&fp_head, struct fp_node, fp_list);
		list_del(&fpnd->fp_list);
		fpn_p->fp_op->fp_put_node(fpn_p, fpnd);
	}
}

static void *
_twc_write_target_thread(void *data)
{
	char *log_header = "_twc_write_target_thread";
	struct twc_interface *twc_p = (struct twc_interface *)data;
	struct twc_private *priv_p = (struct twc_private *)twc_p->tw_private;
	struct sac_interface *sac_p;
	struct twc_channel *chan_p;
	struct twc_chain *chain_p = NULL;
	struct request_node *rn_p, *rn_p2;
	struct ds_interface *ds_p;
	char* d_buf;
	int did_it = 0, vec_counter;
	struct iovec iov[TWC_MAX_IOV];

next_write:
	if (priv_p->p_stop) {
		nid_log_notice("%s: stopping ...", log_header);
		goto out;
	}

	// it no write task, wait for get new write task
	if (!did_it) {
		// twc_write_list insert new write request to write list, then wake up the thread
		pthread_mutex_lock(&priv_p->p_new_write_lck);
		while (!priv_p->p_stop && !priv_p->p_new_write_task) {
			pthread_cond_wait(&priv_p->p_new_write_cond, &priv_p->p_new_write_lck);
		}
		priv_p->p_new_write_task = 0;
		pthread_mutex_unlock(&priv_p->p_new_write_lck);
	}

	if (priv_p->p_stop)
		goto next_write;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_chain_head, c_list) {
		pthread_mutex_lock(&chain_p->c_lck);
		if (chain_p->c_busy) {
			pthread_mutex_unlock(&chain_p->c_lck);
			continue;
		}

		list_for_each_entry(chan_p, struct twc_channel, &chain_p->c_chan_head, c_list) {
			if (list_empty(&chan_p->c_write_head)) {
				continue;
			}

			list_splice_tail_init(&chan_p->c_write_head, &priv_p->p_write_head);
		}
		pthread_mutex_unlock(&chain_p->c_lck);
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	// no request to write,
	if (list_empty(&priv_p->p_write_head)) {
		did_it = 0;
		goto next_write;
	}

	// prepare write request data
	list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &priv_p->p_write_head, r_write_list) {
		struct nid_request *ir_p = &rn_p->r_ir;

		chan_p = (struct twc_channel *)rn_p->r_io_handle;
		assert(chan_p);

		ir_p->len = ntohl(0);
		list_del(&rn_p->r_write_list);

		uint32_t len;
		len = rn_p->r_len;
		ds_p = rn_p->r_ds;
		d_buf = ds_p->d_op->d_position_length(ds_p, rn_p->r_ds_index, rn_p->r_seq, &len);

		vec_counter = 0;
		iov[vec_counter].iov_base = d_buf;
		iov[vec_counter].iov_len = len;
		vec_counter++;

		// taking care the second part of the request if there is
		if(rn_p->r_len > len) {
			d_buf = ds_p->d_op->d_position(ds_p, rn_p->r_ds_index, rn_p->r_seq + len);
			iov[vec_counter].iov_base = d_buf;
			iov[vec_counter].iov_len = rn_p->r_len - len;
			vec_counter++;
		}

		// add current request to channel's response link list
		list_add_tail(&rn_p->r_list, &chan_p->c_resp_head);
		priv_p->p_resp_counter++;

		priv_p->p_seq_assigned++;
		_twc_flush_to_target(chan_p, iov, vec_counter, rn_p->r_offset);
		priv_p->p_seq_flushed++;
	}

	// add write response to response list
	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_chain_head, c_list) {
		if (chain_p->c_busy == 1) {
			continue;
		}

		list_for_each_entry(chan_p, struct twc_channel, &chain_p->c_chan_head, c_list) {
			if (!list_empty(&chan_p->c_resp_head)) {
				sac_p = chan_p->c_sac;
				nid_log_notice("%s: write response", log_header);
				sac_p->sa_op->sa_add_response_list(sac_p, &chan_p->c_resp_head);
			}
		}
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	did_it = 1;
	goto next_write;

out:
	priv_p->p_write_stop = 1;
	return NULL;
}

static void *
_twc_trim_target_thread(void *data)
{
	char *log_header = "_twc_trim_target_thread";
	struct twc_interface *twc_p = (struct twc_interface *)data;
	struct twc_private *priv_p = (struct twc_private *)twc_p->tw_private;
	struct sac_interface *sac_p;
	struct twc_channel *chan_p;
	struct twc_chain *chain_p = NULL;
	struct request_node *rn_p, *rn_p2;

next_trim:
	if (priv_p->p_stop) {
		nid_log_notice("%s: stopping ...", log_header);
		goto out;
	}

	// it no trim task, wait for get new trim task
	// twc_trim_list insert new trim request to trim list, then wake up the thread
	pthread_mutex_lock(&priv_p->p_new_trim_lck);
	while (!priv_p->p_stop && !priv_p->p_new_trim_task) {
		pthread_cond_wait(&priv_p->p_new_trim_cond, &priv_p->p_new_trim_lck);
	}
	priv_p->p_new_trim_task = 0;
	pthread_mutex_unlock(&priv_p->p_new_trim_lck);

	if (priv_p->p_stop)
		goto next_trim;

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct twc_chain, &priv_p->p_chain_head, c_list) {
		if (chain_p->c_busy || list_empty(&chain_p->c_chan_head)) {
			continue;
		}

		chain_p->c_busy = 1;

		priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

		pthread_mutex_lock(&chain_p->c_lck);
		list_for_each_entry(chan_p, struct twc_channel, &chain_p->c_chan_head, c_list) {
			if (list_empty(&chan_p->c_trim_head)) {
				continue;
			}
			list_for_each_entry_safe(rn_p, rn_p2, struct request_node, &chan_p->c_trim_head, r_trim_list) {
				struct nid_request *ir_p = &rn_p->r_ir;
				chan_p = (struct twc_channel *)rn_p->r_io_handle;
				assert(chan_p);

				list_del(&rn_p->r_trim_list);
				ir_p->code = ntohl(0);
				ir_p->len = ntohl(0);
				list_add_tail(&rn_p->r_list, &chan_p->c_resp_head);
				sac_p = chan_p->c_sac;
				sac_p->sa_op->sa_add_response_list(sac_p, &chan_p->c_resp_head);
				priv_p->p_resp_counter++;
			}
		}

		pthread_mutex_unlock(&chain_p->c_lck);
		priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
		chain_p->c_busy = 0;
	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	goto next_trim;

out:
	priv_p->p_trim_stop = 1;
	return NULL;
}

static int
twc_destroy(struct twc_interface *twc_p)
{
	char *log_header = "twc_destroy";
	struct twc_private *priv_p = (struct twc_private *)twc_p->tw_private;
	int rc = 1;

	if (!list_empty(&priv_p->p_chain_head)) {
		nid_log_error("%s: active chain list not empty, cannot destroy!", log_header);
		return rc;
	}
	if (!list_empty(&priv_p->p_inactive_head)) {
		nid_log_error("%s: inactive chain list not empty, cannot destroy!", log_header);
		return rc;
	}

	rc = 0;
	twc_stop(twc_p);

	/*
	 * cleanup work
	 */
	priv_p->p_lck_if.l_op->l_destroy(&priv_p->p_lck_if);
	pthread_mutex_destroy(&priv_p->p_new_write_lck);
	pthread_cond_destroy(&priv_p->p_new_write_cond);
	pthread_mutex_destroy(&priv_p->p_new_read_lck);
	pthread_cond_destroy(&priv_p->p_new_read_cond);
	pthread_mutex_destroy(&priv_p->p_new_trim_lck);
	pthread_cond_destroy(&priv_p->p_new_trim_cond);

	free((void *)priv_p);

	return rc;
}

struct twc_operations twc_op = {
	.tw_create_channel = twc_create_channel,
	.tw_prepare_channel = twc_prepare_channel,
	.tw_recover = twc_recover,
	.tw_pread = twc_pread,
	.tw_read_list = twc_read_list,
	.tw_write_list = twc_write_list,
	.tw_trim_list = twc_trim_list,
	.tw_chan_inactive = twc_chan_inactive,
	.tw_stop = twc_stop,
	.tw_get_info_stat = twc_get_info_stat,
	.tw_destroy = twc_destroy,
	.tw_get_uuid = twc_get_uuid,
	.tw_get_rw = twc_get_rw,
	.tw_get_stat = twc_get_stat,
	.tw_end_page = twc_end_page,
};

int
twc_initialization(struct twc_interface *twc_p, struct twc_setup *setup)
{
	char *log_header = "twc_initialization";
	struct twc_private *priv_p;
	pthread_attr_t attr;
	pthread_t thread_id;
	struct d2an_setup d2an_setup;
	struct d2bn_interface *d2bn_p;
	struct d2bn_setup d2bn_setup;
	struct d2an_interface *d2an_p;
	struct rc_wc_cbn_setup rmc_setup;
	struct rc_wcn_setup rm_setup;

	nid_log_notice("%s: start ...", log_header);
	assert(setup);
	twc_p->tw_op = &twc_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	twc_p->tw_private = priv_p;

	priv_p->p_pause = 1;	// not ready to start yet
	strcpy(priv_p->p_uuid, setup->uuid);
	priv_p->p_allocator = setup->allocator;
	priv_p->p_tp = setup->tp;
	priv_p->p_pp = setup->pp;
	priv_p->p_srn = setup->srn;
	priv_p->p_do_fp = setup->do_fp;
	priv_p->p_lstn = setup->lstn;

	// as if d2an used for response
	d2an_setup.allocator = priv_p->p_allocator;
	d2an_setup.set_id = ALLOCATOR_SET_TWC_D2AN;
	d2an_setup.seg_size = 128;	
	d2an_p = &priv_p->p_d2an;
	d2an_initialization(d2an_p, &d2an_setup);

	d2bn_setup.allocator = priv_p->p_allocator;
	d2bn_setup.set_id = ALLOCATOR_SET_TWC_D2BN;
	d2bn_setup.seg_size = 128;	
	d2bn_p = &priv_p->p_d2bn;
	d2bn_initialization(d2bn_p, &d2bn_setup);

	// initialize two Data (d2) type C Node Module
	// this used for manage read cache
	rmc_setup.allocator = priv_p->p_allocator;
	rmc_setup.seg_size = 128;
	rmc_setup.set_id = ALLOCATOR_SET_TWC_RCTWC_CB;
	rc_wc_cbn_initialization(&priv_p->p_rc_wc_cbn, &rmc_setup);

	rm_setup.allocator = priv_p->p_allocator;
	rm_setup.seg_size = 128;
	rm_setup.set_id = ALLOCATOR_SET_TWC_RCTWC;
	rc_wcn_initialization(&priv_p->p_rc_wcn, &rm_setup);

	INIT_LIST_HEAD(&priv_p->p_chain_head);
	INIT_LIST_HEAD(&priv_p->p_inactive_head);
	INIT_LIST_HEAD(&priv_p->p_write_head);

	pthread_mutex_init(&priv_p->p_new_write_lck, NULL);
	pthread_cond_init(&priv_p->p_new_write_cond, NULL);
	pthread_mutex_init(&priv_p->p_new_read_lck, NULL);
	pthread_cond_init(&priv_p->p_new_read_cond, NULL);
	pthread_mutex_init(&priv_p->p_new_trim_lck, NULL);
	pthread_cond_init(&priv_p->p_new_trim_cond, NULL);

	struct lck_setup lck_setup;
	lck_initialization(&priv_p->p_lck_if, &lck_setup);
	lck_node_init(&priv_p->p_lnode);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _twc_read_buf_thread, twc_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _twc_write_target_thread, twc_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _twc_trim_target_thread, twc_p);

	return 0;
}
