/*
 * sds.c
 * 	Implementation of Split Data Stream Module
 */

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#include "list.h"
#include "lck_if.h"
#include "nid_log.h"
#include "nid_shared.h"
#include "pp_if.h"
#include "io_if.h"
#include "lck_if.h"
#include "sds_if.h"

#define __get_header_index(seq, priv_p)	\
		(((seq) >> priv_p->pp_pageszshift) % (priv_p->p_cachesz))

#define __get_header_index2(buffer, priv_p)	\
		((uint64_t)buffer & (priv_p->p_cachesz - 1))

#define __page_low_boundary(seq, priv_p) \
	(((seq) >> priv_p->pp_pageszshift) << priv_p->pp_pageszshift)
	//((seq) & ~(priv_p->pp_pagesz - 1))
	//
#define __page_low_boundary2(buffer, priv_p) \
	((uint64_t)buffer - ((uint64_t)buffer & (priv_p->pp_pagesz -1)))

#define __page_get_position(seq, priv_p) \
	((seq) & (priv_p->pp_pagesz - 1))

struct sds_channel {
	pthread_mutex_t		p_mlck;
	struct lck_interface	p_lck;
	struct lck_node		*p_lnodes;
	struct list_head	*p_headers;
	void			*c_io_handle;
	int			p_using;
	uint64_t		p_sequence;
	uint32_t		p_curr_pos;
	char			*p_curr_page;
	struct pp_page_node	*p_curr_node;
	char			c_do_buffer;
	char			p_stop;
};

struct sds_private {
	struct pp_interface	*p_pp;
	pthread_mutex_t		p_lck;
	uint32_t		pp_pageszshift;
	uint32_t		pp_pagesz;
	uint32_t		p_poolsz;
	uint8_t			p_cachesz;
	uint8_t			p_channel_counter;
	struct sds_channel	*p_channels[NID_MAX_CHANNELS];
};

/*
 * Notice:
 * 	a data stream for response could be a non buffer ds even if the channel
 * 	is a buffer IO one.
 */
static int
sds_create_worker(struct sds_interface *sds_p, void *io_handle, int do_buffer, struct io_interface *io_p)
{
	char *log_header = "sds_create_worker";
	struct sds_private *priv_p = sds_p->d_private;
	struct pp_interface *pp_p = priv_p->p_pp;;
	struct pp_page_node *np = NULL;
	struct sds_channel *chan_p = NULL;
	struct lck_interface *lck_p;
	struct lck_setup lck_setup;
	int i, idx, chan_index = -1;

	nid_log_info("%s: start ...", log_header);
	np = pp_p->pp_op->pp_get_node_nowait(pp_p);
	if (!np) {
		nid_log_error("%s: Cannot get pp node.", log_header);
		return -1;
	}

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < NID_MAX_CHANNELS; i++) {
		nid_log_error("into channel index is %d", i);
		if (!priv_p->p_channels[i]) {
			nid_log_error("create channel index is %d", i);
			chan_p = x_calloc(1, sizeof(*chan_p));
			priv_p->p_channels[i] = chan_p;
			chan_index = i;
			priv_p->p_channel_counter++;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (chan_index < 0) {
		nid_log_error("%s: Reach max SDS channel limitation.", log_header);
		return -1;
	}

	pthread_mutex_init(&chan_p->p_mlck, NULL);
	lck_initialization(&chan_p->p_lck, &lck_setup);
	lck_p = &chan_p->p_lck;
	chan_p->p_lnodes = x_calloc(priv_p->p_cachesz, sizeof(*chan_p->p_lnodes));
	for (i = 0; i < priv_p->p_cachesz; i++)
		lck_node_init(&chan_p->p_lnodes[i]);

	chan_p->p_headers = x_calloc(priv_p->p_cachesz, sizeof(*chan_p->p_headers));
	for (i = 0; i < priv_p->p_cachesz; i++)
		INIT_LIST_HEAD(&chan_p->p_headers[i]);

	chan_p->c_io_handle = io_handle;
	if (do_buffer)
		chan_p->c_do_buffer = 1;

	np->pp_start_seq = 0;
	np->pp_occupied = 0;
	np->pp_flushed = 0;
	np->pp_coalesced = 0;
	np->pp_where = 0;
	nid_log_warning("%s: get page %p", log_header, np->pp_page);
	INIT_LIST_HEAD(&np->pp_flush_head);
	INIT_LIST_HEAD(&np->pp_release_head);
	lck_node_init(&np->pp_lnode);
	if (io_p)
		io_p->io_op->io_start_page(io_p, chan_p->c_io_handle, np, chan_p->c_do_buffer);

	chan_p->p_curr_page = np->pp_page;
	chan_p->p_curr_node = np;
	idx = __get_header_index(np->pp_start_seq, priv_p);
	nid_log_warning("%s: idx is %d", log_header, idx);
	lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx]);
	list_add(&np->pp_list, &chan_p->p_headers[idx]);
	lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx]);
	pthread_mutex_lock(&chan_p->p_mlck);
	chan_p->p_using++;
	pthread_mutex_unlock(&chan_p->p_mlck);
	return chan_index;
}

/*
 * only one thread can call this function for each channel
 */
static char*
sds_get_buffer(struct sds_interface *sds_p, int chan_index, uint32_t *len, struct io_interface *io_p)
{
	struct sds_private *priv_p = sds_p->d_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	struct lck_interface *lck_p = &chan_p->p_lck;
	struct pp_page_node *np;
	int idx;
	char *p = NULL;
	char *log_header = "sds_get_buffer";

	/*
	 * if the current page is full, get a new page node from pp
	 */
	nid_log_debug("%s: start ...", log_header);
	assert(chan_p->p_curr_pos <= priv_p->pp_pagesz);

	nid_log_debug("%s: cur_pos %u, pp_pagesz  %u...", log_header, chan_p->p_curr_pos,  priv_p->pp_pagesz);
	if (chan_p->p_curr_pos == priv_p->pp_pagesz) {
		np = pp_p->pp_op->pp_get_node_timed(pp_p, 5);
		if (!np) {
			nid_log_warning("%s: out of page!", log_header);
			goto out;
		}
		np->pp_start_seq = chan_p->p_sequence;
		np->pp_occupied = 0;
		np->pp_flushed = 0;
		np->pp_coalesced = 0;
		np->pp_where = 0;
		nid_log_debug("%s: get page %p", log_header, np->pp_page);
		INIT_LIST_HEAD(&np->pp_flush_head);
		INIT_LIST_HEAD(&np->pp_release_head);
		lck_node_init(&np->pp_lnode);
		if (io_p)
			io_p->io_op->io_start_page(io_p, chan_p->c_io_handle, np, chan_p->c_do_buffer);
		chan_p->p_curr_pos = 0;
		chan_p->p_curr_page = np->pp_page;
		chan_p->p_curr_node = np;
		idx = __get_header_index(np->pp_start_seq, priv_p);
		lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx]);
		list_add(&np->pp_list, &chan_p->p_headers[idx]);
		lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx]);
		pthread_mutex_lock(&chan_p->p_mlck);
		chan_p->p_using++;
		pthread_mutex_unlock(&chan_p->p_mlck);
		nid_log_info("%s: new idx:%d", log_header, idx);

	}
	p = chan_p->p_curr_page + chan_p->p_curr_pos;
	assert( chan_p->p_curr_node->pp_occupied < priv_p->pp_pagesz);
	*len = priv_p->pp_pagesz - chan_p->p_curr_pos;
	nid_log_debug("%s: get buffer is %p", log_header, p);
out:
	return p;
}

/*
 * Algorithm:
 * 	To confirm an interval ahead of current sequence to be ready
 * Note:
 * 	Don't need lock here since we assume there is only one thread can do confirm
 */
static void
sds_confirm(struct sds_interface *sds_p, int chan_index, uint32_t len)
{
	struct sds_private *priv_p = sds_p->d_private;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	chan_p->p_sequence += len;
	chan_p->p_curr_pos += len;
	assert(chan_p->p_curr_pos <= priv_p->pp_pagesz);
}

static void
sds_drop_page(struct sds_interface *sds_p, int chan_index, void *to_drop)
{
	char *log_header = "sds_drop_page";
	struct sds_private *priv_p = sds_p->d_private;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	struct lck_interface *lck_p = &chan_p->p_lck;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct pp_page_node *pnp = (struct pp_page_node *)to_drop;
	int idx;

	nid_log_notice("%s: start ...", log_header);
	idx = __get_header_index(pnp->pp_start_seq, priv_p);
	assert(!list_empty(&chan_p->p_headers[idx]));
	lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx]);
	list_del(&pnp->pp_list);	// drop from chan_p->p_headers[idx]
	lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx]);
	nid_log_info("%s: to decrease p_using:%d, idx1:%d",
		log_header, chan_p->p_using, idx);
	pthread_mutex_lock(&chan_p->p_mlck);
	chan_p->p_using--;
	pthread_mutex_unlock(&chan_p->p_mlck);
	pp_p->pp_op->pp_free_node(pp_p, pnp);
}

static void
sds_put_buffer2(struct sds_interface *sds_p, int chan_index, void *buffer, uint32_t len, struct io_interface *io_p)
{
	char *log_header = "sds_put_buffer2";
	struct sds_private *priv_p = sds_p->d_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	struct lck_interface *lck_p = &chan_p->p_lck;
//	uint64_t low_bound1;
	struct pp_page_node *np1;
	int	idx1, i, got_it = 0;


	nid_log_debug("%s: start ...", log_header);
//	low_bound1 = __page_low_boundary2(buffer, priv_p);


	for (i = 0; i < priv_p->p_cachesz; i++){
		lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[i]);
		list_for_each_entry(np1, struct pp_page_node, &chan_p->p_headers[i], pp_list) {
			if ( np1->pp_page <= buffer && (uint64_t)buffer < ((uint64_t)np1->pp_page + (uint64_t)priv_p->pp_pagesz)){

				nid_log_debug("%s: target idx %d", log_header, i);
				nid_log_debug("%s: found page %p", log_header, np1->pp_page);
				got_it = 1;
				break;
			}
		}
		if (got_it){
			lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[i]);
			idx1 = i;
			break;
		}

		lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[i]);
	}


	assert(np1->pp_occupied + len <= priv_p->pp_pagesz);
	np1->pp_occupied += len;
	assert(np1->pp_occupied <= priv_p->pp_pagesz);

	if (np1->pp_occupied == priv_p->pp_pagesz) {
		lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx1]);
		list_del(&np1->pp_list);
		lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx1]);
		nid_log_info("%s: to decrease p_using:%d, idx1:%d",
			log_header, chan_p->p_using, idx1);
		pthread_mutex_lock(&chan_p->p_mlck);
		chan_p->p_using--;
		pthread_mutex_unlock(&chan_p->p_mlck);
		if (io_p)
			io_p->io_op->io_end_page(io_p, chan_p->c_io_handle, np1, chan_p->c_do_buffer);
		else
			pp_p->pp_op->pp_free_node(pp_p, np1);

	}

	nid_log_debug("%s: end ...", log_header);

}

static void
sds_put_buffer(struct sds_interface *sds_p, int chan_index, uint64_t start_seq, uint32_t len, struct io_interface *io_p)
{
	char *log_header = "sds_put_buffer";
	struct sds_private *priv_p = sds_p->d_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	struct lck_interface *lck_p = &chan_p->p_lck;
	uint64_t end_seq = start_seq + len - 1;
	int idx1, idx2;
	uint64_t low_bound1, low_bound2;
	uint32_t node1_put;
	struct pp_page_node *np1, *np2;

	low_bound1 = __page_low_boundary(start_seq, priv_p);
	low_bound2 = __page_low_boundary(end_seq, priv_p);
	idx1 = __get_header_index(start_seq, priv_p);
	idx2 = __get_header_index(end_seq, priv_p);
	assert(!list_empty(&chan_p->p_headers[idx1]));
	assert(!list_empty(&chan_p->p_headers[idx2]));
	lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx1]);
	list_for_each_entry(np1, struct pp_page_node, &chan_p->p_headers[idx1], pp_list) {
		if (np1->pp_start_seq == low_bound1)
			break;
	}
	lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx1]);

	if (idx1 != idx2) {
		lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx2]);
		list_for_each_entry(np2, struct pp_page_node, &chan_p->p_headers[idx2], pp_list) {
			if (np2->pp_start_seq == low_bound2)
				break;
		}
		lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx2]);
	} else {
		np2 = np1;
	}

	if (idx1 == idx2) {
		assert(np1->pp_occupied + len <= priv_p->pp_pagesz);
		np1->pp_occupied += len;
	} else {
		node1_put = (np1->pp_start_seq + priv_p->pp_pagesz) - start_seq;
		assert(node1_put <= priv_p->pp_pagesz);
		np1->pp_occupied += node1_put;
		np2->pp_occupied += len - node1_put;
		nid_log_info("%s: pp_occupied1:%d, pp_occupied2:%d, len:%d, node1_put:%d",
			log_header, np1->pp_occupied, np2->pp_occupied, len, node1_put);
	}

	assert(np1->pp_occupied <= priv_p->pp_pagesz);
	if (np1->pp_occupied == priv_p->pp_pagesz) {
		lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx1]);
		list_del(&np1->pp_list);
		lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx1]);
		nid_log_info("%s: to decrease p_using:%d, idx1:%d",
			log_header, chan_p->p_using, idx1);
		pthread_mutex_lock(&chan_p->p_mlck);
		chan_p->p_using--;
		pthread_mutex_unlock(&chan_p->p_mlck);
		if (io_p)
			io_p->io_op->io_end_page(io_p, chan_p->c_io_handle, np1, chan_p->c_do_buffer);
		else
			pp_p->pp_op->pp_free_node(pp_p, np1);

	}
	if (idx1 != idx2) {
		assert(np2->pp_occupied <= priv_p->pp_pagesz);
		if (np2->pp_occupied == priv_p->pp_pagesz) {
			lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx2]);
			list_del(&np2->pp_list);
			lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx2]);
			nid_log_info("%s: to decrease p_using:%d, idx2:%d, pp_occupied:%d, pp_pagesz:%d",
				log_header, chan_p->p_using, idx2, np2->pp_occupied,  priv_p->pp_pagesz);
			pthread_mutex_lock(&chan_p->p_mlck);
			chan_p->p_using--;
			pthread_mutex_unlock(&chan_p->p_mlck);
			if (io_p)
				io_p->io_op->io_end_page(io_p, chan_p->c_io_handle, np2, chan_p->c_do_buffer);
			else
				pp_p->pp_op->pp_free_node(pp_p, np2);
		}
	}
}

static int
sds_ready(struct sds_interface *sds_p, int chan_index, uint64_t seq)
{
	struct sds_private *priv_p = sds_p->d_private;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	return (chan_p->p_sequence >= seq) ? 1 : 0;
}

static struct pp_page_node *
_sds_page_node(struct sds_interface *sds_p, int chan_index, uint64_t seq)
{
	struct sds_private *priv_p = sds_p->d_private;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	struct lck_interface *lck_p = &chan_p->p_lck;
	struct pp_page_node *np;
	uint64_t low_bound;
	int idx;

	idx = __get_header_index(seq, priv_p);
	low_bound = __page_low_boundary(seq, priv_p);
	lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[idx]);
	list_for_each_entry(np, struct pp_page_node, &chan_p->p_headers[idx], pp_list) {
		if (np->pp_start_seq == low_bound)
			break;
	}
	lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[idx]);
	return np;
}

static char *
sds_position(struct sds_interface *sds_p, int chan_index, uint64_t seq)
{
	struct sds_private *priv_p = sds_p->d_private;
	struct pp_page_node *np;
	np = _sds_page_node(sds_p, chan_index, seq);
	return np->pp_page + __page_get_position(seq, priv_p);
}

/*
 * Input:
 * 	len: length expected.
 * 	     caller should guarantee that len not larger than page size
 * 	     caller should already call ready function to make sure the interval
 * 	     [start_seq, start_seq+len-1] is ready
 * Output:
 * 	len: caller can use [start_seq, start_seq+len-1]
 */
static char *
sds_position_length(struct sds_interface *sds_p, int chan_index, uint64_t start_seq, uint32_t *len)
{
	struct sds_private *priv_p = sds_p->d_private;
	struct pp_page_node *np;
	uint32_t len1;

	assert(*len <= priv_p->pp_pagesz);
	np = _sds_page_node(sds_p, chan_index, start_seq);
	len1 = (start_seq | (priv_p->pp_pagesz - 1)) - start_seq + 1;
	assert(len1 <= priv_p->pp_pagesz);
	if (len1 < *len)
		*len = len1;
	return np->pp_page + __page_get_position(start_seq, priv_p);
}

static struct pp_page_node *
sds_page_node(struct sds_interface *sds_p, int chan_index, uint64_t seq)
{
	return _sds_page_node(sds_p, chan_index, seq);
}

static uint64_t
sds_sequence(struct sds_interface *sds_p, int chan_index)
{
	struct sds_private *priv_p = sds_p->d_private;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	return chan_p->p_sequence;
}

static int
sds_sequence_in_row(struct sds_interface *sds_p, uint64_t seq1, uint64_t seq2)
{
	assert(sds_p);
	assert(seq1 != seq2);
	return 0;
}

static uint32_t
sds_get_pagesz(struct sds_interface *sds_p)
{
	struct sds_private *priv_p = sds_p->d_private;
	return priv_p->pp_pagesz;
}

static void
sds_stop(struct sds_interface *sds_p, int chan_index)
{
	char *log_header = "sds_stop";
	struct sds_private *priv_p = sds_p->d_private;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	nid_log_warning("%s: start ...", log_header);
	chan_p->p_stop = 1;
}

static void
sds_cleanup(struct sds_interface *sds_p, int chan_index, struct io_interface *io_p)
{
	char *log_header = "sds_cleanup";
	struct sds_private *priv_p = sds_p->d_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct sds_channel *chan_p = priv_p->p_channels[chan_index];
	struct lck_interface *lck_p = &chan_p->p_lck;
	struct pp_page_node *np = NULL, *np1;
	int i;

	nid_log_warning("%s: start ...", log_header);
	for (i = 0; i < priv_p->p_cachesz; i++) {
		if (list_empty(&chan_p->p_headers[i]))
			continue;
		list_for_each_entry_safe(np, np1, struct pp_page_node, &chan_p->p_headers[i], pp_list) {
			lck_p->l_op->l_wlock(lck_p, &chan_p->p_lnodes[i]);
			list_del(&np->pp_list);
			lck_p->l_op->l_wunlock(lck_p, &chan_p->p_lnodes[i]);
			if (io_p)
				io_p->io_op->io_end_page(io_p, chan_p->c_io_handle, np, chan_p->c_do_buffer);
			else
				pp_p->pp_op->pp_free_node(pp_p, np);
		}
	}
	lck_p->l_op->l_destroy(lck_p);
	free(chan_p->p_headers);
	free(chan_p->p_lnodes);
	free(chan_p);
	priv_p->p_channels[chan_index] = NULL;
	assert(priv_p->p_channel_counter);
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_channel_counter--;
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
sds_destroy(struct sds_interface *sds_p)
{
	char *log_header = "sds_destroy";
	struct sds_private *priv_p = sds_p->d_private;
	int nretry = 0;

	nid_log_warning("%s: start ...", log_header);
retry:
	pthread_mutex_lock(&priv_p->p_lck);
	if (priv_p->p_channel_counter) {
		pthread_mutex_unlock(&priv_p->p_lck);
		nid_log_warning("%s: channel_counter:%u, going to retry:%d",
				log_header, priv_p->p_channel_counter, ++nretry);
		sleep(1);
		goto retry;
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	pthread_mutex_destroy(&priv_p->p_lck);
	free((void *)priv_p);
	sds_p->d_private = NULL;
	nid_log_warning("%s: end ...", log_header);
}

struct sds_operations sds_op = {
	.d_create_worker = sds_create_worker,
	.d_get_buffer = sds_get_buffer,
	.d_confirm = sds_confirm,
	.d_drop_page = sds_drop_page,
	.d_put_buffer = sds_put_buffer,
	.d_put_buffer2 = sds_put_buffer2,
	.d_ready = sds_ready,
	.d_position = sds_position,
	.d_position_length = sds_position_length,
	.d_page_node = sds_page_node,
	.d_sequence = sds_sequence,
	.d_sequence_in_row = sds_sequence_in_row,
	.d_get_pagesz = sds_get_pagesz,
	.d_stop = sds_stop,
	.d_cleanup = sds_cleanup,
	.d_destroy = sds_destroy,
};

int
sds_initialization(struct sds_interface *sds_p, struct sds_setup *setup)
{
	char *log_header = "sds_initialization";
	struct sds_private *priv_p;
	struct pp_interface *pp_p;

	nid_log_info("%s: start ...(sds_p:%p)", log_header, sds_p);
	assert(setup);
	sds_p->d_op = &sds_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	sds_p->d_private = priv_p;
	pthread_mutex_init(&priv_p->p_lck, NULL);
	priv_p->p_pp = setup->pp;
	pp_p = priv_p->p_pp;
	if (pp_p) {
		priv_p->pp_pageszshift = pp_p->pp_op->pp_get_pageszshift(pp_p);
		priv_p->pp_pagesz = priv_p->p_pp->pp_op->pp_get_pagesz(priv_p->p_pp);
		priv_p->p_poolsz = priv_p->p_pp->pp_op->pp_get_poolsz(priv_p->p_pp);
	}
	priv_p->p_cachesz = 4;
	return 0;
}
