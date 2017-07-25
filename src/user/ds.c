/*
 * Implementation of Data Stream Module
 */

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

#include "nid.h"
#include "nid_shared.h"
#include "nid_log.h"
#include "ds_if.h"
#include "cds_if.h"
#include "sds_if.h"
#include "nid_shared.h"

struct ds_private {
	char			p_name[NID_MAX_UUID];
	char			p_wc_name[NID_MAX_UUID];
	char			p_rc_name[NID_MAX_UUID];
	int			p_type;
	struct cds_interface	*p_cds;
	struct sds_interface	*p_sds;
	void			*p_workers[DS_MAX_WORKERS];
	void			*p_pp;
	uint8_t			p_pagenrshift;
	uint8_t			p_pageszshift;
	char			p_stop;
};

static void
ds_stop(struct ds_interface *ds_p, int chan_index)
{
	char *log_header = "ds_stop";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;

	nid_log_info("%s: start ...", log_header);
	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		cds_p->d_op->d_stop(cds_p, chan_index);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		sds_p->d_op->d_stop(sds_p, chan_index);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}
}

static void
ds_destroy(struct ds_interface *ds_p)
{
	char *log_header = "ds_destroy";
	struct ds_private *priv_p = ds_p->d_private;
	struct sds_interface *sds_p;

	nid_log_warning("%s: start ...", log_header);
	switch (priv_p->p_type) {
	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		sds_p->d_op->d_destroy(sds_p);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}

	free((void *)priv_p);
	ds_p->d_private = NULL;
}

/*
 * only one thread can should this function
 */
static char *
ds_get_buffer(struct ds_interface *ds_p, int chan_index, uint32_t *len, struct io_interface *io_p)
{
	char *log_header = "ds_get_buffer";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	char *buf_p = NULL;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		buf_p = cds_p->d_op->d_get_buffer(cds_p, chan_index, len);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		buf_p = sds_p->d_op->d_get_buffer(sds_p, chan_index, len, io_p);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}
	return buf_p;
}

/*
 * Algorithm:
 * 	To confirm an interval ahead of current sequence to be ready
 * Note:
 * 	Don't need lock here since we assume there is only one thread can do confirm
 */
static void
ds_confirm(struct ds_interface *ds_p, int chan_index, uint32_t len)
{
	char *log_header = "ds_confirm";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		cds_p->d_op->d_confirm(cds_p, chan_index, len);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		sds_p->d_op->d_confirm(sds_p, chan_index, len);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}
}

static void
ds_put_buffer2(struct ds_interface *ds_p, int chan_index, void *buffer, uint32_t len, struct io_interface *io_p)
{
	char *log_header = "ds_put_buffer";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		cds_p->d_op->d_put_buffer2(cds_p, chan_index, buffer, len);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		sds_p->d_op->d_put_buffer2(sds_p, chan_index, buffer, len, io_p);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}
}

static void
ds_put_buffer(struct ds_interface *ds_p, int chan_index, uint64_t start_seq, uint32_t len, struct io_interface *io_p)
{
	char *log_header = "ds_put_buffer";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		cds_p->d_op->d_put_buffer(cds_p, chan_index, start_seq, len);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		sds_p->d_op->d_put_buffer(sds_p, chan_index, start_seq, len, io_p);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}
}

static int
ds_ready(struct ds_interface *ds_p, int chan_index, uint64_t seq)
{
	char *log_header = "ds_ready";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	int rc = 0;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		rc = cds_p->d_op->d_ready(cds_p, chan_index, seq);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		rc = sds_p->d_op->d_ready(sds_p, chan_index, seq);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}

	return rc;
}

static char*
ds_position(struct ds_interface *ds_p, int chan_index, uint64_t seq)
{
	char *log_header = "ds_position";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	char *p = NULL;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		p = cds_p->d_op->d_position(cds_p, chan_index, seq);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		p = sds_p->d_op->d_position(sds_p, chan_index, seq);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}

	return p;
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
ds_position_length(struct ds_interface *ds_p, int chan_index, uint64_t start_seq, uint32_t *len)
{
	char *log_header = "ds_position_length";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	char *p = NULL;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		p = cds_p->d_op->d_position_length(cds_p, chan_index, start_seq, len);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		p = sds_p->d_op->d_position_length(sds_p, chan_index, start_seq, len);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}

	return p;
}

static struct pp_page_node *
ds_page_node(struct ds_interface *ds_p, int chan_index, uint64_t seq)
{
	char *log_header = "ds_page_node";
	struct ds_private *priv_p = ds_p->d_private;
	struct sds_interface *sds_p;
	struct pp_page_node *np;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		nid_log_error("%s: not implemented for cds", log_header);
		assert(0);

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		np = sds_p->d_op->d_page_node(sds_p, chan_index, seq);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}

	return np;

}

static uint64_t
ds_sequence(struct ds_interface *ds_p, int chan_index)
{
	char *log_header = "ds_sequence";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	uint64_t seq = 0;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		seq = cds_p->d_op->d_sequence(cds_p, chan_index);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		seq = sds_p->d_op->d_sequence(sds_p, chan_index);
		break;

	default:
		nid_log_error("%s: wrong type (%d)",
			log_header, priv_p->p_type);
	}

	return seq;
}

static int
ds_sequence_in_row(struct ds_interface *ds_p, int chan_index, uint64_t seq1, uint64_t seq2)
{
	char *log_header = "ds_sequence_in_row";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	int rc = 0;

	assert(chan_index >= 0);
	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		rc = cds_p->d_op->d_sequence_in_row(cds_p, seq1, seq2);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		rc = sds_p->d_op->d_sequence_in_row(sds_p, seq1, seq2);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}

	return rc;
}

static uint32_t
ds_get_pagesz(struct ds_interface *ds_p, int chan_index)
{
	char *log_header = "ds_get_pagesz";
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	uint32_t page_size = 0;

	nid_log_info("%s: chan_index:%d", log_header, chan_index);
	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		page_size = cds_p->d_op->d_get_pagesz(cds_p);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		page_size = sds_p->d_op->d_get_pagesz(sds_p);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}

	return page_size;
}

static void
ds_drop_page(struct ds_interface *ds_p, int chan_index, void *to_drop)
{
	char *log_header = "ds_drop_page";
	struct ds_private *priv_p = (struct ds_private *)ds_p->d_private;
	struct sds_interface *sds_p = priv_p->p_sds;

	switch (priv_p->p_type) {
	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		sds_p->d_op->d_drop_page(sds_p, chan_index, to_drop);
		break;

	default:
		nid_log_error("%s: wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}
}

static void
ds_cleanup(struct ds_interface *ds_p)
{
	char *log_header = "ds_cleanup";
	struct ds_private *priv_p = (struct ds_private *)ds_p->d_private;
	nid_log_info("%s: start ...", log_header);
	free(priv_p);
	ds_p->d_private = NULL;
}

static int
ds_create_worker(struct ds_interface *ds_p, void *io_handle, int do_buffer, struct io_interface *io_p)
{
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	int chan_index = -1;
	char *log_header = "ds_create_worker";

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		chan_index = cds_p->d_op->d_create_worker(cds_p);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		chan_index = sds_p->d_op->d_create_worker(sds_p, io_handle, do_buffer, io_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		assert(0);
	}

	nid_log_info("%s, chan_index:%d", log_header, chan_index);
	return chan_index;
}

static void
ds_release_worker(struct ds_interface *ds_p, int chan_index, struct io_interface *io_p)
{
	struct ds_private *priv_p = ds_p->d_private;
	struct cds_interface *cds_p;
	struct sds_interface *sds_p;
	char *log_header = "ds_release_worker";

	if (chan_index < 0)
		return;

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = priv_p->p_cds;
		cds_p->d_op->d_cleanup(cds_p, chan_index);
		break;

	case DS_TYPE_SPLIT:
		sds_p = priv_p->p_sds;
		sds_p->d_op->d_cleanup(sds_p, chan_index, io_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		assert(0);
	}
}

static char *
ds_get_name(struct ds_interface *ds_p)
{
	struct ds_private *priv_p = ds_p->d_private;
	return priv_p->p_name;
}

static char *
ds_get_wc_name(struct ds_interface *ds_p)
{
	struct ds_private *priv_p = ds_p->d_private;
	return priv_p->p_wc_name;
}

static char *
ds_get_rc_name(struct ds_interface *ds_p)
{
	struct ds_private *priv_p = ds_p->d_private;
	return priv_p->p_rc_name;
}

static struct pp_interface *
ds_get_pp(struct ds_interface *ds_p)
{
	struct ds_private *priv_p = ds_p->d_private;
	return priv_p->p_pp;
}

static int
ds_get_type(struct ds_interface *ds_p)
{
	struct ds_private *priv_p = ds_p->d_private;
	return priv_p->p_type;
}

static void
ds_set_wc(struct ds_interface *ds_p, char *wc_name)
{
	struct ds_private *priv_p = ds_p->d_private;
	strcpy(priv_p->p_wc_name, wc_name);
}

struct ds_operations ds_op = {
	.d_get_buffer = ds_get_buffer,
	.d_confirm = ds_confirm,
	.d_put_buffer = ds_put_buffer,
	.d_put_buffer2 = ds_put_buffer2,
	.d_ready = ds_ready,
	.d_position = ds_position,
	.d_position_length = ds_position_length,
	.d_page_node = ds_page_node,
	.d_sequence = ds_sequence,
	.d_sequence_in_row = ds_sequence_in_row,
	.d_get_pagesz = ds_get_pagesz,
	.d_drop_page = ds_drop_page,
	.d_cleanup = ds_cleanup,
	.d_stop = ds_stop,
	.d_destroy = ds_destroy,
	.d_create_worker = ds_create_worker,
	.d_release_worker = ds_release_worker,
	.d_get_name = ds_get_name,
	.d_get_wc_name = ds_get_wc_name,
	.d_get_rc_name = ds_get_rc_name,
	.d_get_pp = ds_get_pp,
	.d_get_type = ds_get_type,
	.d_set_wc = ds_set_wc,
};

int
ds_initialization(struct ds_interface *ds_p, struct ds_setup *setup)
{
	char *log_header = "ds_initialization";
	struct ds_private *priv_p;
	struct cds_interface *cds_p;
	struct cds_setup cds_setup;
	struct sds_interface *sds_p;
	struct sds_setup sds_setup;

	nid_log_debug("%s: start ...", log_header);
	if (!setup) {
		return -1;
	}

	ds_p->d_op = &ds_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	ds_p->d_private = priv_p;
	priv_p->p_type = setup->type;
	priv_p->p_pp = setup->pool;
	priv_p->p_pagenrshift = setup->pagenrshift;
	priv_p->p_pageszshift = setup->pageszshift;
	strcpy(priv_p->p_name, setup->name);

	switch (priv_p->p_type) {
	case DS_TYPE_CYCLE:
		cds_p = x_calloc(1, sizeof(*cds_p));
		priv_p->p_cds = cds_p;
		cds_setup.pagenrshift = setup->pagenrshift;
		cds_setup.pageszshift = setup->pageszshift;
		cds_initialization(cds_p, &cds_setup);
		break;

	case DS_TYPE_SPLIT:
		if (setup->wc_name)
			strcpy(priv_p->p_wc_name, setup->wc_name);
		if (setup->rc_name)
			strcpy(priv_p->p_rc_name, setup->rc_name);
		sds_p = x_calloc(1, sizeof(*sds_p));
		priv_p->p_sds = sds_p;
		sds_setup.pp = priv_p->p_pp;
		sds_initialization(sds_p, &sds_setup);
		break;

	default:
		nid_log_error("%s: wrong ds type", log_header);
		assert(0);
	}
	return 0;
}
