/*
 * io.c
 * 	Implementation of  io Module
 */

#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include "nid_log.h"
#include "io_if.h"
#include "rio_if.h"
#include "bio_if.h"

struct ini_interface;
struct io_private {
	int			p_type;
	void			*p_real_io;
	struct ini_interface	**p_ini;
};

static void *
io_create_worker(struct io_interface *io_p, void *owner, struct io_channel_info *io_chan, char *exportname, int *new_worker)
{
	char *log_header = "io_create_worker";
	struct io_private *priv_p = io_p->io_private;
	struct rio_interface *rio_p;
	struct bio_interface *bio_p;
	void *io_handle = NULL;

	nid_log_debug("%s: start...", log_header);
	switch (priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rio_p = (struct rio_interface *)priv_p->p_real_io;
		io_handle = rio_p->r_op->r_create_channel(rio_p, io_chan, exportname, new_worker);
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		io_handle = bio_p->b_op->b_create_channel(bio_p, owner, io_chan, exportname, new_worker);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
	}

	return io_handle;
}

static void *
io_prepare_worker(struct io_interface *io_p, struct io_channel_info *io_chan, char *exportname)
{
	char *log_header = "io_prepare_worker";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;
	void *io_handle = NULL;

	nid_log_debug("%s: start...", log_header);
	switch (priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		io_handle = bio_p->b_op->b_prepare_channel(bio_p, io_chan, exportname);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
	}

	return io_handle;
}

static ssize_t
io_pread(struct io_interface *io_p, void *io_handle, void *buf, size_t count, off_t offset)
{
	char *log_header = "io_pread";
	struct io_private *priv_p = io_p->io_private;
	struct rio_interface *rio_p;
	struct bio_interface *bio_p;
	ssize_t nread;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rio_p = (struct rio_interface *)priv_p->p_real_io;
		nread = rio_p->r_op->r_pread(rio_p, io_handle, buf, count, offset);
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		nread = bio_p->b_op->b_pread(bio_p, io_handle, buf, count, offset);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	}

	return nread;

}

static void
io_read_list(struct io_interface *io_p, void *io_handle, struct list_head *read_head, int read_counter)
{
	char *log_header = "io_read_list";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		nid_log_error("%s: not implemented for rio", log_header);
		assert(0);

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		bio_p->b_op->b_read_list(bio_p, io_handle, read_head, read_counter);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       assert(0);
	}
}

static ssize_t
io_pwrite(struct io_interface *io_p, void *io_handle, void *buf, size_t count, off_t offset)
{
	char *log_header = "io_pwrite";
	struct io_private *priv_p = io_p->io_private;
	struct rio_interface *rio_p;
	ssize_t nwrite;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rio_p = (struct rio_interface *)priv_p->p_real_io;
		nwrite = rio_p->r_op->r_pwrite(rio_p, io_handle, buf, count, offset);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       assert(0);
	}

	return nwrite;

}

static void
io_write_list(struct io_interface *io_p, void *io_handle, struct list_head *write_head, int write_counter)
{
	char *log_header = "io_write_list";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		nid_log_error("%s: not implemented for rio", log_header);
		assert(0);

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		bio_p->b_op->b_write_list(bio_p, io_handle, write_head, write_counter);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       assert(0);
	}
}

static ssize_t
io_trim(struct io_interface *io_p, void *io_handle, off_t offset, size_t len)
{
	char *log_header = "io_trim";
	struct io_private *priv_p = io_p->io_private;
	struct rio_interface *rio_p;
	int rc;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rio_p = (struct rio_interface *)priv_p->p_real_io;
		rc = rio_p->r_op->r_trim(rio_p, io_handle, offset, len);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       assert(0);
	}

	return rc;

}

static void
io_trim_list(struct io_interface *io_p, void *io_handle, struct list_head *trim_head, int trim_counter)
{
	char *log_header = "io_trim_list";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		nid_log_error("%s: not implemented for rio", log_header);
		assert(0);

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		bio_p->b_op->b_trim_list(bio_p, io_handle, trim_head, trim_counter);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       assert(0);
	}
}

static int
io_get_type(struct io_interface *io_p)
{
	struct io_private *priv_p = io_p->io_private;
	return priv_p->p_type;
}

static int
io_fast_flush(struct io_interface *io_p, char *resoure)
{
	char *log_header = "io_fast_flush";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;
	int rc;

	nid_log_debug("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rc = -1;
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		rc = bio_p->b_op->b_fast_flush(bio_p, resoure);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       rc = -1;
	}

	return rc;
}

static int
io_stop_fast_flush(struct io_interface *io_p, char *resource)
{
	char *log_header = "io_stop_fast_flush";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;
	int rc;

	nid_log_debug("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rc = -1;
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		rc = bio_p->b_op->b_stop_fast_flush(bio_p, resource);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       rc = -1;
	}

	return rc;
}

static int
io_vec_start(struct io_interface *io_p)
{
	char *log_header = "io_vec_start";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;
	int rc;

	nid_log_debug("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rc = -1;
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		rc = bio_p->b_op->b_vec_start(bio_p);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       rc = -1;
	}

	return rc;
}

static int
io_vec_stop(struct io_interface *io_p)
{
	char *log_header = "io_vec_stop";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;
	int rc;

	nid_log_debug("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rc = -1;
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		rc = bio_p->b_op->b_vec_stop(bio_p);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       rc = -1;
	}

	return rc;
}

static int
io_close(struct io_interface *io_p, void *io_handle)
{
	char *log_header = "io_close";
	struct io_private *priv_p = io_p->io_private;
	struct rio_interface *rio_p;
	struct bio_interface *bio_p;
	int rc;

	nid_log_notice("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rio_p = (struct rio_interface *)priv_p->p_real_io;
		rc = rio_p->r_op->r_close(rio_p, io_handle);
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		rc = bio_p->b_op->b_chan_inactive(bio_p, io_handle);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	}

	return rc;
}

static int
io_stop(struct io_interface *io_p)
{
	char *log_header = "io_stop";
	struct io_private *priv_p = io_p->io_private;
	struct rio_interface *rio_p;
	struct bio_interface *bio_p;
	int rc = -1;

	nid_log_notice("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rio_p = (struct rio_interface *)priv_p->p_real_io;
		rc = rio_p->r_op->r_stop(rio_p);
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		rc = bio_p->b_op->b_stop(bio_p);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	}

	return rc;
}

static void
io_start_page(struct io_interface *io_p, void *io_handle, void *page_p, int do_buffer)
{
	char *log_header = "io_start_page";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		assert(!do_buffer);
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		bio_p->b_op->b_start_page(bio_p, io_handle, page_p, do_buffer);
		break;

	default:
		nid_log_error("%s: wrong io_type:%d", log_header, priv_p->p_type);
		assert(0);
	}
}

static void
io_end_page(struct io_interface *io_p, void *io_handle, void *page_p, int do_buffer)
{
	char *log_header = "io_end_page";
	struct io_private *priv_p = io_p->io_private;
	struct rio_interface *rio_p;
	struct bio_interface *bio_p;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		assert(!do_buffer);
		rio_p = (struct rio_interface *)priv_p->p_real_io;
		rio_p->r_op->r_end_page(rio_p, io_handle , page_p);
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		bio_p->b_op->b_end_page(bio_p, io_handle, page_p, do_buffer);
		break;

	default:
		nid_log_error("%s: wrong io_type:%d", log_header, priv_p->p_type);
		assert(0);
	}
}

static void
io_get_chan_stat(struct io_interface *io_p, void *io_handle, struct io_chan_stat *stat)
{
	char *log_header = "io_get_chan_stat";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;

	nid_log_info("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		bio_p->b_op->b_get_chan_stat(bio_p, io_handle, stat);
		break;

	default:
		nid_log_error("%s: wrong io_type:%d", log_header, priv_p->p_type);
		assert(0);
	}
}

static void
io_get_vec_stat(struct io_interface *io_p, void *io_handle, struct io_vec_stat *stat)
{
	char *log_header = "io_get_vec_stat";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;

	nid_log_info("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		stat->s_io_type_rio = 1;
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		bio_p->b_op->b_get_vec_stat(bio_p, io_handle, stat);
		stat->s_io_type_bio = 1;
		break;

	default:
		nid_log_error("%s: wrong io_type:%d", log_header, priv_p->p_type);
		assert(0);
	}
}

static void
io_get_stat(struct io_interface *io_p, struct io_stat *stat)
{
	char *log_header = "io_get_stat";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;

	nid_log_info("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		stat->s_io_type_rio = 1;
		break;

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		bio_p->b_op->b_get_stat(bio_p, stat);
		stat->s_io_type_bio = 1;

		break;

	default:
		nid_log_error("%s: wrong io_type:%d", log_header, priv_p->p_type);
		assert(0);
	}
}

static uint8_t
io_get_cutoff(struct io_interface *io_p, void *io_handle)
{
	char *log_header = "io_get_cutoff";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;
	uint8_t cutoff = 0;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		nid_log_error("%s: not implemented for rio", log_header);
		assert(0);

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		cutoff = bio_p->b_op->b_get_cutoff(bio_p, io_handle);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       assert(0);
	}
	return cutoff;
}

static int
io_destroy_wc(struct io_interface *io_p, char *wc_uuid)
{
	char *log_header = "io_destroy_wc";
	struct io_private *priv_p = io_p->io_private;
	struct bio_interface *bio_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		nid_log_error("%s: not implemented for rio", log_header);
		assert(0);

	case IO_TYPE_BUFFER:
		bio_p = (struct bio_interface *)priv_p->p_real_io;
		rc = bio_p->b_op->b_destroy_wc(bio_p, wc_uuid);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       assert(0);
	}
	return rc;
}

struct io_operations io_op = {
	.io_create_worker = io_create_worker,
	.io_prepare_worker = io_prepare_worker,
	.io_get_type = io_get_type,
	.io_pread = io_pread,
	.io_read_list = io_read_list,
	.io_pwrite = io_pwrite,
	.io_write_list = io_write_list,
	.io_trim = io_trim,
	.io_trim_list = io_trim_list,
	.io_close = io_close,
	.io_stop = io_stop,
	.io_start_page = io_start_page,
	.io_end_page = io_end_page,
	.io_get_chan_stat = io_get_chan_stat,
	.io_get_stat = io_get_stat,
	.io_fast_flush = io_fast_flush,
	.io_stop_fast_flush = io_stop_fast_flush,
	.io_vec_start = io_vec_start,
	.io_vec_stop = io_vec_stop,
	.io_get_vec_stat = io_get_vec_stat,
	.io_get_cutoff = io_get_cutoff,
	.io_destroy_wc = io_destroy_wc,
};

int
io_initialization(struct io_interface *io_p, struct io_setup *setup)
{
	char *log_header = "io_initialization";
	struct io_private *priv_p;
	struct rio_interface *rio_p;
	struct rio_setup rio_setup;
	struct bio_interface *bio_p;
	struct bio_setup bio_setup;
	int rc = 0;

	nid_log_notice("%s: start ...", log_header);
	assert(setup);
	io_p->io_op = &io_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	io_p->io_private = priv_p;
	priv_p->p_type = setup->io_type;
	priv_p->p_ini = setup->ini;

	switch(priv_p->p_type) {
	case IO_TYPE_RESOURCE:
		rio_p = x_calloc(1, sizeof(*rio_p));
		priv_p->p_real_io = rio_p;
		rio_initialization(rio_p, &rio_setup);
		break;

	case IO_TYPE_BUFFER:
		bio_p = x_calloc(1, sizeof(*bio_p));
		priv_p->p_real_io = bio_p;
		bio_initialization(bio_p, &bio_setup);
		break;

	default:
	       nid_log_error("%s: got wrong type", log_header);
	       rc = -1;
	}

	return rc;
}
