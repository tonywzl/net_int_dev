/*
 * bio.c
 * 	Implementation of Buffer IO Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "list.h"
#include "wc_if.h"
#include "io_if.h"
#include "bio_if.h"

struct bio_channel {
	struct list_head	bc_list;
	struct wc_interface	*bc_wc;
	void			*bc_handle;
	char			*exportname;
	void			*owner;
};

struct bio_private {
	struct list_head	p_channel_head;
	pthread_mutex_t		p_lck;
};

static void *
bio_create_channel(struct bio_interface *bio_p, void *owner, struct io_channel_info *io_info, char *res_p, int *new)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct wc_interface *wc_p = (struct wc_interface *)io_info->io_wc;
	void *wc_handle = io_info->io_wc_handle;
	struct bio_channel *new_chan; // *the_chan;

	*new = 1;
	new_chan = x_calloc(1, sizeof(*new_chan));
	new_chan->exportname = res_p;
	new_chan->owner = owner;
	new_chan->bc_handle = wc_handle;
	new_chan->bc_wc = (struct wc_interface *)wc_p;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&new_chan->bc_list, &priv_p->p_channel_head);
	pthread_mutex_unlock(&priv_p->p_lck);
	return new_chan;
}

static void *
bio_prepare_channel(struct bio_interface *bio_p, struct io_channel_info *io_info, char *res_p)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct wc_interface *wc_p = (struct wc_interface *)io_info->io_wc;
	void *wc_handle = io_info->io_wc_handle;
	struct bio_channel *new_chan;

	new_chan = x_calloc(1, sizeof(*new_chan));
	new_chan->exportname = res_p;
	new_chan->owner = NULL;
	new_chan->bc_handle = wc_handle;
	new_chan->bc_wc = (struct wc_interface *)wc_p;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&new_chan->bc_list, &priv_p->p_channel_head);
	pthread_mutex_unlock(&priv_p->p_lck);

	return new_chan;
}

static uint32_t
bio_pread(struct bio_interface *bio_p, void *io_handle, void *buf, size_t count, off_t offset)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;
	assert(priv_p);
	return wc_p->wc_op->wc_pread(wc_p, wc_handle, buf, count, offset);
}

static void
bio_read_list(struct bio_interface *bio_p, void *io_handle, struct list_head *read_head, int read_counter)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;
	assert(priv_p);
	wc_p->wc_op->wc_read_list(wc_p, wc_handle, read_head, read_counter);
}

static void
bio_write_list(struct bio_interface *bio_p, void *io_handle, struct list_head *write_head, int write_counter)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;
	assert(priv_p);
	wc_p->wc_op->wc_write_list(wc_p, wc_handle, write_head, write_counter);
}

static void
bio_trim_list(struct bio_interface *bio_p, void *io_handle, struct list_head *trim_head, int trim_counter)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;
	assert(priv_p);
	wc_p->wc_op->wc_trim_list(wc_p, wc_handle, trim_head, trim_counter);
}


static int
bio_chan_inactive(struct bio_interface *bio_p, void *io_handle)
{
	char *log_header = "bio_chan_inactive";
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;

	nid_log_debug("%s: start...", log_header);
	assert(priv_p);
	pthread_mutex_lock(&priv_p->p_lck);
	list_del(&chan_p->bc_list);
	pthread_mutex_unlock(&priv_p->p_lck);
	int ret = wc_p->wc_op->wc_chan_inactive(wc_p, wc_handle);
	if (ret) return ret;

	free(chan_p);
	return 0;
}

static int
bio_stop(struct bio_interface *bio_p)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct wc_interface *wc_p;
	struct bio_channel *the_chan;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(the_chan, struct bio_channel, &priv_p->p_channel_head, bc_list) {
		wc_p = the_chan->bc_wc;
		wc_p->wc_op->wc_stop(wc_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return 0;
}

static int
bio_start_page(struct bio_interface *bio_p, void *io_handle, void *page_p, int do_buffer)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;
	assert(priv_p);
	return wc_p->wc_op->wc_start_page(wc_p, wc_handle, page_p, do_buffer);
}

static int
bio_end_page(struct bio_interface *bio_p, void *io_handle, void *page_p, int do_buffer)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;
	assert(priv_p);
	return wc_p->wc_op->wc_end_page(wc_p, wc_handle, page_p, do_buffer);
}

static int
bio_get_chan_stat(struct bio_interface *bio_p, void *io_handle, struct io_chan_stat *sp)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;
	assert(priv_p);
	return wc_p->wc_op->wc_get_chan_stat(wc_p, wc_handle, sp);
}

static int
bio_get_stat(struct bio_interface *bio_p, struct io_stat *sp)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p;
	struct wc_interface *wc_p;
	// TODO : get stat for channels
	if (!list_empty(&priv_p->p_channel_head)) {
		chan_p = list_first_entry(&priv_p->p_channel_head, struct bio_channel, bc_list);
		wc_p = chan_p->bc_wc;
		return wc_p->wc_op->wc_get_stat(wc_p, sp);
	}
	INIT_LIST_HEAD(&sp->s_inactive_head);
	return -1;
}

static int
bio_get_vec_stat(struct bio_interface *bio_p, void *io_handle, struct io_vec_stat *sp)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;
	void *wc_handle = chan_p->bc_handle;
	assert(priv_p);
	return wc_p->wc_op->wc_get_vec_stat(wc_p, wc_handle, sp);
}

static int
bio_fast_flush(struct bio_interface *bio_p, char *resource)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct wc_interface *wc_p;
	struct bio_channel *the_chan;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(the_chan, struct bio_channel, &priv_p->p_channel_head, bc_list) {
		wc_p = the_chan->bc_wc;
		wc_p->wc_op->wc_fast_flush(wc_p, resource);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return 0;
}

static int
bio_stop_fast_flush(struct bio_interface *bio_p, char *resource)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct wc_interface *wc_p;
	struct bio_channel *the_chan;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(the_chan, struct bio_channel, &priv_p->p_channel_head, bc_list) {
		wc_p = the_chan->bc_wc;
		wc_p->wc_op->wc_stop_fast_flush(wc_p, resource);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return 0;
}

static int
bio_vec_start(struct bio_interface *bio_p)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct wc_interface *wc_p;
	struct bio_channel *the_chan;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(the_chan, struct bio_channel, &priv_p->p_channel_head, bc_list) {
		wc_p = the_chan->bc_wc;
		wc_p->wc_op->wc_vec_start(wc_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return 0;
}

static int
bio_vec_stop(struct bio_interface *bio_p)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct wc_interface *wc_p;
	struct bio_channel *the_chan;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(the_chan, struct bio_channel, &priv_p->p_channel_head, bc_list) {
		wc_p = the_chan->bc_wc;
		wc_p->wc_op->wc_vec_stop(wc_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return 0;
}

static uint8_t
bio_get_cutoff(struct bio_interface *bio_p, void *io_handle)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct bio_channel *chan_p = (struct bio_channel *)io_handle;
	struct wc_interface *wc_p = chan_p->bc_wc;

	assert(priv_p);
	void *wc_handle = chan_p->bc_handle;
	return wc_p->wc_op->wc_get_cutoff(wc_p, wc_handle);
}

static int
bio_destroy_wc(struct bio_interface *bio_p, char *wc_uuid)
{
	struct bio_private *priv_p = (struct bio_private *)bio_p->b_private;
	struct wc_interface *wc_p = NULL;
	struct bio_channel *chan_p, *chan_p2;
	struct list_head tmp_head;
	int rc = 1;

	INIT_LIST_HEAD(&tmp_head);
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry_safe(chan_p, chan_p2, struct bio_channel, &priv_p->p_channel_head, bc_list) {
		if (!strcmp(chan_p->bc_wc->wc_op->wc_get_uuid(chan_p->bc_wc), wc_uuid)) {
			if (!wc_p)
				wc_p = chan_p->bc_wc;
			list_del(&chan_p->bc_list);
			list_add_tail(&chan_p->bc_list, &tmp_head);
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (wc_p) {
		rc = wc_p->wc_op->wc_destroy(wc_p);
	}

	list_for_each_entry_safe(chan_p, chan_p2, struct bio_channel, &tmp_head, bc_list) {
		free((void *)chan_p);
	}
	return rc;
}

struct bio_operations bio_op = {
	.b_create_channel = bio_create_channel,
	.b_prepare_channel = bio_prepare_channel,
	.b_pread = bio_pread,
	.b_read_list = bio_read_list,
	.b_write_list = bio_write_list,
	.b_trim_list = bio_trim_list,
	.b_chan_inactive = bio_chan_inactive,
	.b_stop = bio_stop,
	.b_start_page = bio_start_page,
	.b_end_page = bio_end_page,
	.b_get_chan_stat = bio_get_chan_stat,
	.b_get_vec_stat = bio_get_vec_stat,
	.b_get_stat = bio_get_stat,
	.b_fast_flush = bio_fast_flush,
	.b_stop_fast_flush = bio_stop_fast_flush,
	.b_vec_start = bio_vec_start,
	.b_vec_stop = bio_vec_stop,
	.b_get_cutoff = bio_get_cutoff,
	.b_destroy_wc = bio_destroy_wc,
};

int
bio_initialization(struct bio_interface *bio_p, struct bio_setup *setup)
{
	char *log_header = "bio_initialization";
	struct bio_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	bio_p->b_op = &bio_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bio_p->b_private = priv_p;

	INIT_LIST_HEAD(&priv_p->p_channel_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	return 0;
}
