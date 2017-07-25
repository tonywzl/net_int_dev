/*
 * rio.c
 * 	Implementation of Resource IO Module
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <pthread.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "pp_if.h"
#include "rw_if.h"
#include "io_if.h"
#include "rio_if.h"
#include "ds_if.h"
#include "sdsg_if.h"
#include "cdsg_if.h"


struct rio_channel {
	struct list_head	c_list;
	struct rw_interface	*c_rw;
	void			*c_rw_handle;
	char			*c_exportname;
	struct pp_interface	*c_pp;
};

struct rio_private {
	pthread_mutex_t		p_lck;
	struct list_head	p_chan_head;
};

static void *
rio_create_channel(struct rio_interface *rio_p, struct io_channel_info *io_info, char *exportname, int *new_chan)
{
	char *log_header = "rio_create_channel";
	struct rio_private *priv_p = rio_p->r_private;
	struct rio_channel *chan_p = NULL;
	struct sdsg_interface *sdsg_p;
	struct cdsg_interface *cdsg_p;
	struct ds_interface *ds_p;


	nid_log_warning("%s: start (io_rw_handle:%p)...", log_header, io_info->io_rw_handle);
	*new_chan = 1;	// always new
	chan_p = x_calloc(1, sizeof(*chan_p));
	nid_log_notice("%s: step 1 (chan_p:%p, rw_handle:%p)...", log_header, chan_p, chan_p->c_rw_handle);
	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&chan_p->c_list, &priv_p->p_chan_head);
	pthread_mutex_unlock(&priv_p->p_lck);
	nid_log_notice("%s: step 2 (chan_p:%p, rw_handle:%p)...", log_header, chan_p, chan_p->c_rw_handle);
	chan_p->c_rw_handle = io_info->io_rw_handle;
	nid_log_notice("%s: step 3 (chan_p:%p, rw_handle:%p)...", log_header, chan_p, chan_p->c_rw_handle);
	chan_p->c_rw = io_info->io_rw;
	chan_p->c_exportname = exportname;
	nid_log_notice("%s: end (chan_p:%p, rw_handle:%p)...", log_header, chan_p, chan_p->c_rw_handle);

	sdsg_p = io_info->io_sdsg;
	cdsg_p = io_info->io_cdsg;

	if (sdsg_p)
		ds_p = sdsg_p->dg_op->dg_search_and_create_sds(sdsg_p, io_info->io_ds_name);
	else
		ds_p = cdsg_p->dg_op->dg_search_and_create_cds(cdsg_p, io_info->io_ds_name);

	chan_p->c_pp = ds_p->d_op->d_get_pp(ds_p);
	return chan_p;
}

static ssize_t
rio_pread(struct rio_interface *rio_p, void *io_handle, void *buf, size_t count, off_t offset)
{
	struct rio_private *priv_p = rio_p->r_private;
	struct rio_channel *chan_p = (struct rio_channel *)io_handle;
	struct rw_interface *rw_p = chan_p->c_rw;
	assert(priv_p);
	return rw_p->rw_op->rw_pread(rw_p, chan_p->c_rw_handle, buf, count, offset);
}

static ssize_t
rio_pwrite(struct rio_interface *rio_p, void *io_handle, void *buf, size_t count, off_t offset)
{
	struct rio_private *priv_p = rio_p->r_private;
	struct rio_channel *chan_p = (struct rio_channel *)io_handle;
	struct rw_interface *rw_p = chan_p->c_rw;
	assert(priv_p);
	return rw_p->rw_op->rw_pwrite(rw_p, chan_p->c_rw_handle, buf, count, offset);
}

static int
rio_trim(struct rio_interface *rio_p, void *io_handle, off_t offset, size_t len)
{
	struct rio_private *priv_p = rio_p->r_private;
	struct rio_channel *chan_p = (struct rio_channel *)io_handle;
	struct rw_interface *rw_p = chan_p->c_rw;
	assert(priv_p);
	return rw_p->rw_op->rw_trim(rw_p, chan_p->c_rw_handle, offset, len);
}

static int
rio_close(struct rio_interface *rio_p, void *io_handle)
{
	char *log_header = "rio_close";
	struct rio_private *priv_p = rio_p->r_private;
	struct rio_channel *chan_p = (struct rio_channel *)io_handle;
	struct rw_interface *rw_p = chan_p->c_rw;
	void *rw_handle = chan_p->c_rw_handle;
	int rc = 0;

	nid_log_notice("%s: start (%s)...", log_header, chan_p->c_exportname);
	pthread_mutex_lock(&priv_p->p_lck);
	rc = rw_p->rw_op->rw_close(rw_p, rw_handle);
	list_del(&chan_p->c_list);	// removed from p_chan_head
	free(chan_p);
	pthread_mutex_unlock(&priv_p->p_lck);
	return rc;
}

static int
rio_stop(struct rio_interface *rio_p)
{
	char *log_header = "rio_stop";
	struct rio_private *priv_p = rio_p->r_private;
	nid_log_notice("%s: start ...", log_header);
	assert(priv_p);
	return 0;
}

static void
rio_end_page(struct rio_interface *rio_p, void *io_handle, void *page_p)
{
	assert(rio_p);
	struct rio_channel *chan_p = (struct rio_channel *)io_handle;
	struct pp_interface *pp_p = (struct pp_interface *)chan_p->c_pp;
	struct pp_page_node *np = (struct pp_page_node *)page_p;
	if (pp_p)
		pp_p->pp_op->pp_free_node(pp_p, np);
}

struct rio_operations rio_op = {
	.r_create_channel = rio_create_channel,
	.r_pread = rio_pread,
	.r_pwrite = rio_pwrite,
	.r_trim = rio_trim,
	.r_close = rio_close,
	.r_stop = rio_stop,
	.r_end_page = rio_end_page,
};

int
rio_initialization(struct rio_interface *rio_p, struct rio_setup *setup)
{
	struct rio_private *priv_p;
	char *log_header = "rio_initialization";

	nid_log_notice("%s: start ...", log_header);
	assert(setup);
	rio_p->r_op = &rio_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	INIT_LIST_HEAD(&priv_p->p_chan_head);
	rio_p->r_private = priv_p;
	pthread_mutex_init(&priv_p->p_lck, NULL);
	return 0;
}
