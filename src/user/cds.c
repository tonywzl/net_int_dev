/*
 * cds.c
 * Implementation of Cycle Data Stream Module
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>

#include "nid.h"
#include "nid_shared.h"
#include "nid_log.h"
#include "cds_if.h"
#include "nid_shared.h"

#define PAGE_STAT_INUSE	0x01
//#define DEFAULT_PAGENRSHIFT	2
#define DEFAULT_PAGENRSHIFT	4
#define	DEFAULT_PAGESZSHIFT	23

#define get_page_idx(seq, priv_p)	\
	(((seq) >> priv_p->p_pageszshift) % priv_p->p_pagenr)

#define get_page_idx2(buff, chan_buf, priv_p)	\
	(( (uint64_t)buff - (uint64_t)chan_buf ) >>  priv_p->p_pageszshift)

struct cds_channel {
	char		*c_buf;				// buffer for data stream
	char		*c_page_start[NID_MAX_DSPAGENR];
	char		c_page_status[NID_MAX_DSPAGENR];
	uint32_t	c_page_occupied[NID_MAX_DSPAGENR];
	uint32_t	c_curr_pos;			// in current page
	int		c_curr_page;
	uint64_t	c_sequence;
	pthread_mutex_t	c_lck;
	pthread_cond_t	c_cond;
	char		c_stop;
};

struct cds_private {
	pthread_mutex_t		p_lck;
	uint8_t			p_pagenrshift;
	uint8_t			p_pagenr;
	uint8_t			p_pageszshift;
	uint32_t		p_pagesz;
	uint32_t		p_pageszmask;
	uint32_t		p_bufsz;
	struct cds_channel	*p_channels[NID_MAX_CHANNELS];
};

static int
cds_create_worker(struct cds_interface *cds_p)
{
	char *log_header = "cds_create_worker";
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = NULL;
	int i, chan_index = -1;

	nid_log_info("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < NID_MAX_CHANNELS; i++) {
		if (!priv_p->p_channels[i]) {
			chan_p = x_calloc(1, sizeof(*chan_p));
			priv_p->p_channels[i] = chan_p;
			chan_index = i;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (chan_index < 0)
		return -1;

	//chan_p->c_buf = x_malloc(priv_p->p_bufsz);
	if (x_posix_memalign((void **)&chan_p->c_buf, getpagesize(), priv_p->p_bufsz) !=0 ) {
		nid_log_error("%s: x_posix_memalign failed.", log_header);
	}
	for (i = 0; i < priv_p->p_pagenr; i++) {
		chan_p->c_page_start[i] = chan_p->c_buf + priv_p->p_pagesz * i;
		chan_p->c_page_occupied[i] = 0;
		chan_p->c_page_status[i] = 0;
	}
	chan_p->c_curr_page = 0;
	chan_p->c_curr_pos = 0;
	chan_p->c_page_status[0] |= PAGE_STAT_INUSE;
	pthread_mutex_init(&chan_p->c_lck, NULL);
	pthread_cond_init(&chan_p->c_cond, NULL);
	return chan_index;
}

static void
cds_stop(struct cds_interface *cds_p, int chan_index)
{
	char *log_header = "cds_stop";
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	nid_log_info("%s: start ...", log_header);
	pthread_mutex_lock(&chan_p->c_lck);
	chan_p->c_stop = 1;
	pthread_cond_signal(&chan_p->c_cond);
	pthread_mutex_unlock(&chan_p->c_lck);
}

/*
 * only one thread can call this function for each channel
 */
static char*
cds_get_buffer(struct cds_interface *cds_p, int chan_index, uint32_t *len)
{
	char *log_header = "cds_get_buffer";
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	char *start;
	int idx, rc, wait_counter = 0;
	struct timeval now;
	struct timespec ts;

	/*
	 * if the current page is full, take the next one
	 */
	if (chan_p->c_curr_pos == priv_p->p_pagesz) {
		if (++chan_p->c_curr_page == priv_p->p_pagenr) {
			chan_p->c_curr_page = 0;
		}
		chan_p->c_curr_pos = 0;

		/*
		 *  make sure this page is not inuse status
		 */
		idx = chan_p->c_curr_page;
		pthread_mutex_lock(&chan_p->c_lck);
		while ((chan_p->c_page_status[idx] & PAGE_STAT_INUSE) && !chan_p->c_stop) {
			gettimeofday(&now, NULL);
			ts.tv_sec = now.tv_sec + 1;
			ts.tv_nsec = now.tv_usec * 1000;
			rc = pthread_cond_timedwait(&chan_p->c_cond, &chan_p->c_lck, &ts);
			if (rc) {
				nid_log_debug("%s: not free, idx:%d, finished %d bytes (%p)",
					log_header, idx, chan_p->c_page_occupied[idx], priv_p);
				wait_counter++;
			}
		}
		if (chan_p->c_stop) {
			nid_log_debug("%s: got stop, idx:%d, after wait %d secs, (%p)",
				log_header, idx, wait_counter, priv_p);
			pthread_mutex_unlock(&chan_p->c_lck);
			return NULL;
		}
		chan_p->c_page_status[idx] |= PAGE_STAT_INUSE;
		pthread_mutex_unlock(&chan_p->c_lck);
		if (wait_counter)
			nid_log_debug("%s: success, idx:%d, after wait %d secs (%p)",
				log_header, idx, wait_counter, priv_p);
	}

	nid_log_debug ("%s: page start with %p, idx: %d", log_header, chan_p->c_buf, idx);
	start = chan_p->c_page_start[chan_p->c_curr_page] + chan_p->c_curr_pos;
	*len = priv_p->p_pagesz - chan_p->c_curr_pos;
	return start;
}

/*
 * Algorithm:
 * 	To confirm an interval ahead of current sequence to be ready
 * Note:
 * 	Don't need lock here since we assume there is only one thread can do confirm
 */
static void
cds_confirm(struct cds_interface *cds_p, int chan_index, uint32_t len)
{
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	chan_p->c_sequence += len;
	chan_p->c_curr_pos += len;
}

static int
try_to_free_page(struct cds_interface *cds_p, int chan_index, int idx)
{
	char *log_header = "try_to_free_page";
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];

	if (chan_p->c_page_occupied[idx] > priv_p->p_pagesz) {
		nid_log_error("%s: error!, idx:%d, sz:%u, occupied%u (%p)",
			log_header, idx, priv_p->p_pagesz, chan_p->c_page_occupied[idx], priv_p);
		return -1;
	}

	if (chan_p->c_page_occupied[idx] == priv_p->p_pagesz) {
		nid_log_debug("%s: free idx:%d, sz:%u, occupied:%u (%p)",
			log_header, idx, priv_p->p_pagesz, chan_p->c_page_occupied[idx], priv_p);
		pthread_mutex_lock(&chan_p->c_lck);
		chan_p->c_page_occupied[idx] = 0;
		chan_p->c_page_status[idx] &= ~PAGE_STAT_INUSE;
		pthread_cond_signal(&chan_p->c_cond);
		pthread_mutex_unlock(&chan_p->c_lck);
	}
	return 0;
}

static void
cds_put_buffer2(struct cds_interface *cds_p, int chan_index, void *buffer, uint32_t len)
{

	char *log_header = "cds_put_buffer2";

	nid_log_debug("%s: start ...", log_header);
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	int idx1;
	assert ( (uint64_t)buffer >= (uint64_t)chan_p->c_buf && (uint64_t)buffer <= ((uint64_t)chan_p->c_buf + (uint64_t)priv_p->p_bufsz));
	idx1 = get_page_idx2(buffer, chan_p->c_buf, priv_p);
	nid_log_debug("%s: idx:%d", log_header, idx1);
	chan_p->c_page_occupied[idx1] += len;

	if (try_to_free_page(cds_p, chan_index, idx1))
		nid_log_error("%s: wrong, idx1:%d,  len:%u, (%p)",
			log_header, idx1, len, priv_p);
}

static void
cds_put_buffer(struct cds_interface *cds_p, int chan_index, uint64_t start_seq, uint32_t len)
{
	char *log_header = "cds_put_buffer";
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	uint64_t end_seq = start_seq + len - 1;
	int idx1, idx2;

	idx1 = get_page_idx(start_seq, priv_p);
	idx2 = get_page_idx(end_seq, priv_p);
	if (idx1 == idx2) {
		/* within one page */
		chan_p->c_page_occupied[idx1] += len;
	} else {
		/* cross pages */
		nid_log_error("%s: crossing pages idx1:%d, idx2:%d, seq:%lu, len:%u, (%p)",
			log_header, idx1, idx2, start_seq, len, priv_p);
		chan_p->c_page_occupied[idx1] += (start_seq | (priv_p->p_pagesz-1)) - start_seq + 1;
		chan_p->c_page_occupied[idx2] += end_seq - (end_seq & priv_p->p_pageszmask) + 1;;
	}

	if (try_to_free_page(cds_p, chan_index, idx1))
		nid_log_error("%s: wrong, idx1:%d, seq:%lu, len:%u, (%p)",
			log_header, idx1, start_seq, len, priv_p);
	if (idx2 != idx1) {
		if (try_to_free_page(cds_p, chan_index, idx2))
			nid_log_error("%s: wrong, idx2:%d, seq:%lu, len:%u, (%p)",
				log_header, idx2, start_seq, len, priv_p);
	}
}

static int
cds_ready(struct cds_interface *cds_p, int chan_index, uint64_t seq)
{
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	return (chan_p->c_sequence >= seq) ? 1 : 0;
}

static char*
cds_position(struct cds_interface *cds_p, int chan_index, uint64_t seq)
{
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	return chan_p->c_buf + (seq & (priv_p->p_bufsz - 1));
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
static char*
cds_position_length(struct cds_interface *cds_p, int chan_index, uint64_t start_seq, uint32_t *len)
{
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	int idx;

	idx = get_page_idx(start_seq, priv_p);
	if (idx == priv_p->p_pagenr - 1) {
		uint32_t len1;
		len1 = (start_seq | (priv_p->p_pagesz - 1)) - start_seq + 1;
		if (len1 < *len)
			*len = len1;
	}
	return chan_p->c_buf + (start_seq & (priv_p->p_bufsz - 1));
}


static uint64_t
cds_sequence(struct cds_interface *cds_p, int chan_index)
{
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	return chan_p->c_sequence;
}

static int
cds_sequence_in_row(struct cds_interface *cds_p, uint64_t seq1, uint64_t seq2)
{
	struct cds_private *priv_p = cds_p->d_private;
	int idx1, idx2;
	assert(priv_p);
	idx1 = get_page_idx(seq1, priv_p);
	idx2 = get_page_idx(seq2, priv_p);
	return (idx2 == idx1 + 1) ? 1 : 0;
}

static uint32_t
cds_get_pagesz(struct cds_interface *cds_p)
{
	struct cds_private *priv_p = cds_p->d_private;
	return priv_p->p_pagesz;
}

static void
cds_cleanup(struct cds_interface *cds_p, int chan_index)
{
	struct cds_private *priv_p = cds_p->d_private;
	struct cds_channel *chan_p = priv_p->p_channels[chan_index];
	free(chan_p->c_buf);
	free(chan_p);
	priv_p->p_channels[chan_index] = NULL;
}

struct cds_operations cds_op = {
	.d_create_worker = cds_create_worker,
	.d_get_buffer = cds_get_buffer,
	.d_confirm = cds_confirm,
	.d_put_buffer = cds_put_buffer,
	.d_put_buffer2 = cds_put_buffer2,
	.d_ready = cds_ready,
	.d_position = cds_position,
	.d_position_length = cds_position_length,
	.d_sequence = cds_sequence,
	.d_sequence_in_row = cds_sequence_in_row,
	.d_get_pagesz = cds_get_pagesz,
	.d_cleanup = cds_cleanup,
	.d_stop = cds_stop,
};

int
cds_initialization(struct cds_interface *cds_p, struct cds_setup *setup)
{
	struct cds_private *priv_p;

	nid_log_debug("cds_initialization start ...");
	if (!setup) {
		return -1;
	}

	cds_p->d_op = &cds_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	cds_p->d_private = priv_p;
	pthread_mutex_init(&priv_p->p_lck, NULL);
	priv_p->p_pagenrshift = DEFAULT_PAGENRSHIFT;
	if (setup->pagenrshift && setup->pagenrshift <= NID_MAX_DSPAGENRSHIFT)
		priv_p->p_pagenrshift = setup->pagenrshift;
	priv_p->p_pagenr = 1 << priv_p->p_pagenrshift;

	priv_p->p_pageszshift = DEFAULT_PAGESZSHIFT;
	if (setup->pageszshift && setup->pageszshift <= NID_MAX_DSPAGESZSHIFT)
		priv_p->p_pageszshift = setup->pageszshift;
	priv_p->p_pagesz = 1 << priv_p->p_pageszshift;
	priv_p->p_pageszmask = ~(priv_p->p_pagesz - 1);

	priv_p->p_bufsz = priv_p->p_pagesz * priv_p->p_pagenr;
	nid_log_debug("cds_initialization: nr:%d, sz:%d", priv_p->p_pagenr, priv_p->p_pagesz);
	return 0;
}
