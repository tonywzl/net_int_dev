/*
 * rw.c
 * 	Implementation of Read Write Module
 */

#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "drw_if.h"
#include "mrw_if.h"
#include "rw_if.h"

#define get_align_offset(alignment, offset, inttype) \
	(alignment == 0 ? offset : (~(((inttype)alignment)- 1)) & offset)

#define get_align_length(alignment, offset, len, newoffset, inttype) \
	(alignment == 0 ? len : (((~(((inttype)alignment)- 1)) & (offset+len-1)) + alignment - newoffset))

struct rw_private {
	char			p_name[NID_MAX_DEVNAME];
	char			p_exportname[NID_MAX_PATH];
	int			p_type;
	struct fpn_interface	*p_fpn_p;
	void			*p_real_rw;
};

struct rw_handle {
	void* 	p_sub_handle;
	int 	p_alignment;
};

static void *
rw_create_worker(struct rw_interface *rw_p, char *exportname, char io_sync, char direct_io, int alignment, int dev_size)
{
	char *log_header = "rw_create_worker";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	struct mrw_interface *mrw_p;
	struct rw_handle *rw_handle;

	rw_handle = x_malloc(sizeof(struct rw_handle));
	if (!rw_handle ) {
		nid_log_error("%s: x_malloc failed.", log_header);
	}
	rw_handle->p_alignment = alignment;
	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		rw_handle->p_sub_handle = drw_p->rw_op->rw_create_worker(drw_p, exportname, io_sync, direct_io);
		assert(rw_handle->p_sub_handle);
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		rw_handle->p_sub_handle = mrw_p->rw_op->rw_create_worker(mrw_p, exportname, dev_size);
		assert(rw_handle->p_sub_handle);
		break;

	default:
		free(rw_handle);
		rw_handle = NULL;
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return rw_handle;
}


static ssize_t
_rw_pread(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count, off_t offset, struct list_head *fp_head, void** align_buf, size_t *align_count, off_t *align_offset)
{
	char *log_header = "rw_pread";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	struct mrw_interface *mrw_p;
	size_t nread;
	struct rw_handle* my_handle = (struct rw_handle*)rw_handle;
	size_t count_align = count;
	off_t offset_align = offset;
	void *buf_align = buf;
	char is_align_read = 0;

	if (my_handle->p_alignment) {
		offset_align = get_align_offset(my_handle->p_alignment, offset, off_t);
		count_align = get_align_length(my_handle->p_alignment, offset, count, offset_align, off_t);
		is_align_read = (offset_align != offset || count_align != count);
	}

	if (is_align_read) {
		// Unlikely going here.
		if (x_posix_memalign((void**)&buf_align, getpagesize(), count_align) != 0) {
			nid_log_error("%s: x_posix_memalign failed.", log_header);
		}
	}

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		if (fp_head) {
		    nread = drw_p->rw_op->rw_pread_fp(drw_p, my_handle->p_sub_handle,
		                        buf_align, count_align, offset_align, fp_head);
		} else {
		    nread = drw_p->rw_op->rw_pread(drw_p, my_handle->p_sub_handle,
		            buf_align, count_align, offset_align);
		}
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		if (fp_head) {
			nread = mrw_p->rw_op->rw_pread_fp(mrw_p, my_handle->p_sub_handle,
					buf_align, count_align, offset_align, fp_head);
		} else {
			nread = mrw_p->rw_op->rw_pread(mrw_p, my_handle->p_sub_handle,
					buf_align, count_align, offset_align);
		}
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
		nread = -1;
	}

	if (align_buf) {
		*align_buf = NULL;
	}
	if (is_align_read) {
		if (nread > 0) {
			// We just do a alignment Read, so we need copy buffer.
			nread = nread - (offset - offset_align);
			if (nread > 0) {
				nread = nread > count ? count : nread;
				memcpy(buf, buf_align + (offset - offset_align), nread);
			} else {
				nread = -1;
			}
		}
		if (fp_head && align_buf) {
			*align_buf = buf_align;
			*align_count = count_align;
			*align_offset = offset_align;
		} else {
			free(buf_align);
		}
	}

	return (ssize_t)nread;
}

static ssize_t
rw_pread(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count, off_t offset)
{
    return _rw_pread(rw_p, rw_handle, buf, count, offset, NULL, NULL, NULL, NULL);
}

static ssize_t
rw_pread_fp(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count, off_t offset, struct list_head* fp_head, void** align_buf, size_t *align_count, off_t *align_offset)
{
    return _rw_pread(rw_p, rw_handle, buf, count, offset, fp_head, align_buf, align_count, align_offset);
}

static ssize_t
_rw_pwrite(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count, off_t offset, struct list_head *fp_head, void** align_buf, size_t *align_count, off_t *align_offset)
{
	char *log_header = "rw_pwrite";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	struct mrw_interface *mrw_p;
	size_t nwrite;
	struct rw_handle* my_handle = (struct rw_handle*)rw_handle;
	size_t count_align;
	off_t offset_align;
	void *buf_align;
	char is_align_write = 0;

	if (my_handle->p_alignment) {
		offset_align = get_align_offset(my_handle->p_alignment, offset, off_t);
		count_align = get_align_length(my_handle->p_alignment, offset, count, offset_align, off_t);
		is_align_write = (offset_align != offset || count_align != count);
	}

	if (is_align_write) {
		// Unlikely going here. Do read write back.
		if (x_posix_memalign((void**)&buf_align, getpagesize(), count_align) != 0) {
			nid_log_error("%s: x_posix_memalign failed.", log_header);
		}
		memset(buf_align, 0, count_align);
		(void)rw_pread(rw_p, rw_handle, buf_align, count_align, offset_align);
		memcpy(buf_align + (offset - offset_align), buf, count);
	}

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		if (fp_head) {
			if (is_align_write)
				nwrite = drw_p->rw_op->rw_pwrite_fp(drw_p, my_handle->p_sub_handle,
					buf_align, count_align, offset_align, fp_head);
			else
				nwrite = drw_p->rw_op->rw_pwrite_fp(drw_p, my_handle->p_sub_handle,
					buf, count, offset, fp_head);
		} else {
			if (is_align_write)
				nwrite = drw_p->rw_op->rw_pwrite(drw_p, my_handle->p_sub_handle,
					buf_align, count_align, offset_align);
			else
				nwrite = drw_p->rw_op->rw_pwrite(drw_p, my_handle->p_sub_handle,
					buf, count, offset);
		}
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		if (fp_head) {
			if (is_align_write)
				nwrite = mrw_p->rw_op->rw_pwrite_fp(mrw_p, my_handle->p_sub_handle,
					buf_align, count_align, offset_align, fp_head);
			else
				nwrite = mrw_p->rw_op->rw_pwrite_fp(mrw_p, my_handle->p_sub_handle,
					buf, count, offset, fp_head);
		} else {
			if (is_align_write)
				nwrite = mrw_p->rw_op->rw_pwrite(mrw_p, my_handle->p_sub_handle,
					buf_align, count_align, offset_align);
			else
				nwrite = mrw_p->rw_op->rw_pwrite(mrw_p, my_handle->p_sub_handle,
					buf, count, offset);
		}
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
		nwrite = -1;
	}

	if (align_buf) {
		*align_buf = NULL;
	}

	if (is_align_write) {
		if (nwrite > 0) {
			nwrite = nwrite - (offset - offset_align);
			if (nwrite > 0)
				nwrite = nwrite > count ? count : nwrite;
			else
				nwrite = -1;
		}
		if (fp_head && align_buf) {
			*align_buf = buf_align;
			*align_count = count_align;
			*align_offset = offset_align;
		} else {
			free(buf_align);
		}
	}

	return (ssize_t)nwrite;
}

static ssize_t
rw_pwrite(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count, off_t offset)
{
    return _rw_pwrite(rw_p, rw_handle, buf, count, offset, NULL, NULL, NULL, NULL);
}

static ssize_t
rw_pwrite_fp(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count, off_t offset, struct list_head *fp_head, void **align_buf, size_t *align_count, off_t *align_offset)
{
    return _rw_pwrite(rw_p, rw_handle, buf, count, offset, fp_head, align_buf, align_count, align_offset);
}

static ssize_t
_rw_pwritev(struct rw_interface *rw_p, void *rw_handle, struct iovec *iov, int iovcnt, off_t offset, struct list_head *fp_head, void **align_buf, size_t *align_count, off_t *align_offset)
{
	char *log_header = "rw_pwritev";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	struct mrw_interface *mrw_p;
	size_t nwrite;

	struct rw_handle* my_handle = (struct rw_handle*)rw_handle;
	size_t count_align, count=0;
	off_t offset_align, buf_offset;
	char is_align_write = 0;
	void *buf = NULL;
	int i;

	if (my_handle->p_alignment) {
		// Count write length first.
		for (i = 0; i < iovcnt; i++) {
			count += iov[i].iov_len;
			if ((iov[i].iov_len  & (my_handle->p_alignment -1)) != 0) {
				is_align_write = 1;
			}
		}

		offset_align = get_align_offset(my_handle->p_alignment, offset, off_t);
		count_align = get_align_length(my_handle->p_alignment, offset, count, offset_align, off_t);
		if (!is_align_write)
			is_align_write = (offset_align != offset || count_align != count);

		if (is_align_write) {
			// Unlikely going here. Do read write back.
			if (x_posix_memalign((void**)&buf, getpagesize(), count_align) != 0) {
				nid_log_error("%s: x_posix_memalign failed.", log_header);
			}
			memset(buf, 0, count_align);
			
			if (offset_align != offset || count_align != count) {
				(void)rw_pread(rw_p, rw_handle, buf, count_align, offset_align);
			}

			buf_offset = offset - offset_align;
			for (i = 0; i < iovcnt; i++) {
				memcpy(buf+buf_offset, iov[i].iov_base, iov[i].iov_len);
				buf_offset += iov[i].iov_len;
			}
		}
	}

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		if (fp_head) {
			if (is_align_write) {
				nwrite = drw_p->rw_op->rw_pwrite_fp(drw_p, my_handle->p_sub_handle,
				    buf, count_align, offset_align, fp_head);
			} else {
				nwrite = drw_p->rw_op->rw_pwritev_fp(drw_p, my_handle->p_sub_handle,
				    iov, iovcnt, offset, fp_head);
			}
		} else {
			if (is_align_write) {
				nwrite = drw_p->rw_op->rw_pwrite(drw_p, my_handle->p_sub_handle,
				    buf, count_align, offset_align);
			} else {
				nwrite = drw_p->rw_op->rw_pwritev(drw_p, my_handle->p_sub_handle,
				    iov, iovcnt, offset);
			}
		}
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		if (fp_head) {
			if (is_align_write)
				nwrite = mrw_p->rw_op->rw_pwrite_fp(mrw_p, my_handle->p_sub_handle,
						buf, count_align, offset_align, fp_head);
			else
				nwrite = mrw_p->rw_op->rw_pwritev_fp(mrw_p, my_handle->p_sub_handle,
						iov, iovcnt, offset, fp_head);
		} else {
			if (is_align_write)
				nwrite = mrw_p->rw_op->rw_pwrite(mrw_p, my_handle->p_sub_handle,
						buf, count_align, offset_align);
			else
				nwrite = mrw_p->rw_op->rw_pwritev(mrw_p, my_handle->p_sub_handle,
						iov, iovcnt, offset);
		}
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
		nwrite = -1;
	}

	if (align_buf) {
	    *align_buf = NULL;
	}
	if (is_align_write) {
		if (nwrite > 0) {
			nwrite = nwrite - (offset - offset_align);
			if (nwrite > 0)
				nwrite = nwrite > count ? count : nwrite;
			else
				nwrite = -1;
		}
		if (fp_head && align_buf) {
			*align_buf = buf;
			*align_count = count_align;
			*align_offset = offset_align;
		} else {
			free(buf);
		}
	}

	return (ssize_t)nwrite;
}

static ssize_t
rw_pwritev(struct rw_interface *rw_p, void *rw_handle, struct iovec *iov, int iovcnt, off_t offset)
{
	return _rw_pwritev(rw_p, rw_handle, iov, iovcnt, offset, NULL, NULL, NULL, NULL);
}

static ssize_t
rw_pwritev_fp(struct rw_interface *rw_p, void *rw_handle, struct iovec *iov, int iovcnt, off_t offset, struct list_head *fp_head, void** align_buf, size_t *align_count, off_t *align_offset)
{
	return _rw_pwritev(rw_p, rw_handle, iov, iovcnt, offset, fp_head, align_buf, align_count, align_offset);
}

void
rw_pwrite_async(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg) {
	ssize_t ret;
	ret = _rw_pwrite(rw_p, rw_handle, buf, count, offset, NULL, NULL, NULL, NULL);
	rw_callback(ret < 0 ? -1 : 0, arg);
}

void
rw_pwritev_async(struct rw_interface *rw_p, void *rw_handle, struct iovec *iov, int iovcnt, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg) {
	ssize_t ret;
	ret = _rw_pwritev(rw_p, rw_handle, iov, iovcnt, offset, NULL, NULL, NULL, NULL);
	rw_callback(ret < 0 ? -1 : 0, arg);
}

void
rw_pread_async(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg) {
	ssize_t ret;
	ret = _rw_pread(rw_p, rw_handle, buf, count, offset, NULL, NULL, NULL, NULL);
	rw_callback(ret < 0 ? -1 : 0, arg);
}

static int
rw_trim(struct rw_interface *rw_p, void *rw_handle, off_t offset, size_t len)
{
	char *log_header = "rw_trim";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	int rc;
	void* drw_handle = ((struct rw_handle*)rw_handle)->p_sub_handle;

	if(priv_p->p_type == RW_TYPE_DEVICE) {
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		rc = drw_p->rw_op->rw_trim(drw_handle, offset, len);
	} else {
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
		rc = -1;
	}
	return rc;
}

static int
rw_close(struct rw_interface *rw_p, void *rw_handle)
{
	char *log_header = "rw_close";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	struct mrw_interface *mrw_p;
	int rc;
	struct rw_handle* my_handle = (struct rw_handle*)rw_handle;

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		rc = drw_p->rw_op->rw_close(drw_p, my_handle->p_sub_handle);
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		rc = mrw_p->rw_op->rw_close(mrw_p, my_handle->p_sub_handle);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
		rc = -1;
	}
	free(my_handle);
	return rc;
}

static void
rw_destroy(struct rw_interface *rw_p)
{
	char *log_header = "rw_destroy";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;

	nid_log_warning("%s: start ...", log_header);
	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		drw_p->rw_op->rw_destroy(drw_p);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
		assert(0);
	}

	free((void *)priv_p);
	rw_p->rw_private = NULL;
}


static void
__rw_write_callback(int errorcode, struct rw_callback_arg *arg)
{
	struct rw_alignment_write_arg *arg1 = (struct rw_alignment_write_arg *)arg;
	INIT_LIST_HEAD(&arg1->arg_packaged->ag_fp_head);
	arg1->arg_callback_packaged(errorcode, arg);
	free(arg1->arg_buffer_align);
	free(arg);
}

static void
rw_pwritev_async_fp(struct rw_interface *rw_p, void *rw_handle, struct iovec *iov,
	int iovcnt, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{
	char *log_header = "rw_pwritev";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	struct mrw_interface *mrw_p;
	struct rw_handle* my_handle = (struct rw_handle*)rw_handle;
	size_t count_align, count = 0;
	off_t offset_align, buf_offset;
	char is_align_write = 0, need_read_again = 1;
	void *buf_align = NULL;
	int i;
	struct rw_alignment_write_arg *newarg;

	if (my_handle->p_alignment) {
		// Count write length first.
		for (i = 0; i < iovcnt; i++) {
			count += iov[i].iov_len;
			if ((iov[i].iov_len  & (my_handle->p_alignment - 1)) != 0) {
				is_align_write = 1;
				need_read_again = 0;
			}
		}

		offset_align = get_align_offset(my_handle->p_alignment, offset, off_t);
		count_align = get_align_length(my_handle->p_alignment, offset, count, offset_align, off_t);
		is_align_write = is_align_write == 1 ? 1 : (offset_align != offset || count_align != count);

		if (is_align_write) {
			// Unlikely going here. Do read write back.
			if (x_posix_memalign((void**)&buf_align, getpagesize(), count_align) !=0) {
				nid_log_error("%s: x_posix_memalign failed.", log_header);
			}
			memset(buf_align, 0, count_align);
			newarg = x_malloc(sizeof(*newarg));
			if (!newarg ) {
				nid_log_error("%s: x_malloc failed.", log_header);
			}
			newarg->arg_packaged = arg;
			newarg->arg_buffer_align = buf_align;
			newarg->arg_callback_packaged = rw_callback;
			newarg->arg_this.ag_type = RW_CB_TYPE_ALIGNMENT;
			newarg->arg_count_align = count_align;
			newarg->arg_offset_align = offset_align;

			if (need_read_again) {
				(void)rw_pread(rw_p, rw_handle, buf_align, count_align, offset_align);
			}

			buf_offset = offset - offset_align;
			for (i = 0; i < iovcnt; i++) {
				memcpy(buf_align + buf_offset, iov[i].iov_base, iov[i].iov_len);
				buf_offset += iov[i].iov_len;
			}
		}
	}

	switch (priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		if (is_align_write) {
			drw_p->rw_op->rw_pwrite_async_fp(drw_p, my_handle->p_sub_handle,
				buf_align, count_align, offset_align, __rw_write_callback, &newarg->arg_this);
		} else {
			drw_p->rw_op->rw_pwritev_async_fp(drw_p, my_handle->p_sub_handle,
				iov, iovcnt, offset, rw_callback, arg);
		}
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		if (is_align_write) {
			mrw_p->rw_op->rw_pwrite_async_fp(mrw_p, my_handle->p_sub_handle,
				buf_align, count_align, offset_align, __rw_write_callback, &newarg->arg_this);
		} else {
			mrw_p->rw_op->rw_pwritev_async_fp(mrw_p, my_handle->p_sub_handle,
				iov, iovcnt, offset, rw_callback, arg);
		}
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rw_pwrite_async_fp(struct rw_interface *rw_p, void *rw_handle, void *buf, size_t count,
	off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{
	char *log_header = "rw_pwrite_async_fp";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	struct mrw_interface *mrw_p;
	struct rw_handle* my_handle = (struct rw_handle*)rw_handle;
	size_t count_align = count;
	off_t offset_align = offset;
	void *buf_align = NULL;
	char is_align_write = 0;
	struct rw_alignment_write_arg *newarg;

	if (my_handle->p_alignment) {
		offset_align = get_align_offset(my_handle->p_alignment, offset, off_t);
		count_align = get_align_length(my_handle->p_alignment, offset, count, offset_align, off_t);
		is_align_write = (offset_align != offset || count_align != count);
	}

	if (is_align_write) {
		// Unlikely going here. Do read write back.
		if (x_posix_memalign((void**)&buf_align, getpagesize(), count_align)!=0) {
			nid_log_error("%s: x_posix_memalign failed.", log_header);
		}
		memset(buf_align, 0, count_align);
		newarg = x_malloc(sizeof(*newarg));
		if (!newarg ) {
			nid_log_error("%s: x_malloc failed.", log_header);
		}
		newarg->arg_packaged = arg;
		newarg->arg_buffer_align = buf;
		newarg->arg_callback_packaged = rw_callback;
		newarg->arg_this.ag_type = RW_CB_TYPE_ALIGNMENT;
		newarg->arg_count_align = count_align;
		newarg->arg_offset_align = offset_align;

		(void)rw_pread(rw_p, rw_handle, buf_align, count_align, offset_align);
		memcpy(buf_align + (offset - offset_align) , buf, count);
	}

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		if (is_align_write) {
			drw_p->rw_op->rw_pwrite_async_fp(drw_p, my_handle->p_sub_handle,
				buf_align, count_align, offset_align, __rw_write_callback, &newarg->arg_this);
		} else {
			drw_p->rw_op->rw_pwrite_async_fp(drw_p, my_handle->p_sub_handle,
				buf, count, offset, rw_callback, arg);
		}
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		if (is_align_write) {
			mrw_p->rw_op->rw_pwrite_async_fp(mrw_p, my_handle->p_sub_handle,
				buf_align, count_align, offset_align, __rw_write_callback, &newarg->arg_this);
		} else {
			mrw_p->rw_op->rw_pwrite_async_fp(mrw_p, my_handle->p_sub_handle,
				buf, count, offset, rw_callback, arg);
		}
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

}


static void
__rw_read_callback(int errorcode, struct rw_callback_arg *arg) {
	struct rw_alignment_read_arg *arg1 = (struct rw_alignment_read_arg *)arg;
	size_t nread = arg1->arg_count_align;

	INIT_LIST_HEAD(&arg1->arg_packaged->ag_fp_head);
	if (errorcode == 0) {
		// We just do a alignment Read, so we need copy buffer.
		nread = nread - (arg1->arg_offset - arg1->arg_offset_align);
		if (nread > 0) {
			nread = nread > arg1->arg_count ? arg1->arg_count : nread;
			memcpy(arg1->arg_buf, arg1->arg_buffer_align + (arg1->arg_offset - arg1->arg_offset_align), nread);
		} else {
			nread = -1;
		}
	}

	arg1->arg_callback_packaged(errorcode, arg);
	free(arg1->arg_buffer_align);
	free(arg);
}

void
rw_pread_async_fp(struct rw_interface *rw_p, void *handle, void *buf, size_t count,
	off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{
	char *log_header = "rw_pread";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface *drw_p;
	struct mrw_interface *mrw_p;
	struct rw_handle* my_handle = (struct rw_handle*)handle;
	size_t count_align;
	off_t offset_align;
	char is_align_read = 0;
	void *buf_align = buf;
	struct rw_alignment_read_arg *newarg = NULL;

	offset_align = get_align_offset(my_handle->p_alignment, offset, off_t);
	count_align = get_align_length(my_handle->p_alignment, offset, count, offset_align, off_t);
	is_align_read = (offset_align != offset || count_align != count);

	if (is_align_read) {
		// Unlikely going here.
		if (x_posix_memalign((void**)&buf_align, getpagesize(), count_align)!=0 ) {
			nid_log_error("%s: x_posix_memalign failed.", log_header);
		}
		newarg = x_malloc(sizeof(*newarg));
		if (!newarg ) {
			nid_log_error("%s: x_malloc failed.", log_header);
		}
		newarg->arg_packaged = arg;
		newarg->arg_buffer_align = buf_align;
		newarg->arg_callback_packaged = rw_callback;
		newarg->arg_count = count;
		newarg->arg_offset = offset;
		newarg->arg_buf = buf;
		newarg->arg_offset_align = offset_align;
		newarg->arg_count_align = count_align;
		newarg->arg_this.ag_type = RW_CB_TYPE_ALIGNMENT;
	}

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		if (is_align_read) {
			drw_p->rw_op->rw_pread_async_fp(drw_p, my_handle->p_sub_handle,
				buf_align, count_align, offset_align, __rw_read_callback, &newarg->arg_this);
		} else {
			drw_p->rw_op->rw_pread_async_fp(drw_p, my_handle->p_sub_handle,
				buf_align, count_align, offset_align, rw_callback, arg);
		}
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		if (is_align_read) {
			mrw_p->rw_op->rw_pread_async_fp(mrw_p, my_handle->p_sub_handle,
				buf_align, count_align, offset_align, __rw_read_callback, &newarg->arg_this);
		} else {
			mrw_p->rw_op->rw_pread_async_fp(mrw_p, my_handle->p_sub_handle,
				buf_align, count_align, offset_align, rw_callback, arg);
		}
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

}

static void
rw_fetch_fp (struct rw_interface *rw_p, void *handle, off_t voff, size_t len, rw_callback2_fn func, void *arg, void *fpout, bool *fpFound) {
	char *log_header = "rw_fetch_fp";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct mrw_interface *mrw_p;
	struct rw_handle* my_handle = (struct rw_handle*)handle;

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		nid_log_error("%s: not support type (%d)", log_header, priv_p->p_type);
		assert(0);
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		mrw_p->rw_op->rw_fetch_fp(mrw_p, my_handle->p_sub_handle, voff, len, func, arg, fpout, fpFound);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rw_fetch_data (struct rw_interface *rw_p, struct iovec* vec, size_t count, void *fp, rw_callback2_fn func, void *arg) {
	char *log_header = "rw_fetch_data";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct mrw_interface *mrw_p;

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		nid_log_error("%s: not support type (%d)", log_header, priv_p->p_type);
		assert(0);
		break;

	case RW_TYPE_MSERVER:
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		mrw_p->rw_op->rw_fetch_data(vec, count, fp, func, arg);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static int
rw_get_type(struct rw_interface *rw_p)
{
	return ((struct rw_private *)rw_p->rw_private)->p_type;
}

static char *
rw_get_name(struct rw_interface *rw_p)
{
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	return priv_p->p_name;
}

static char *
rw_get_exportname(struct rw_interface *rw_p)
{
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	char *ret_name = NULL;

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		ret_name = priv_p->p_exportname;
		break;
	case RW_TYPE_MSERVER:
		ret_name = priv_p->p_exportname;
		break;
	default:
		break;
	}

	return ret_name;
}

static void*
rw_get_rw_obj(struct rw_interface *rw_p)
{
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	return priv_p->p_real_rw;
}

static void
rw_ms_flush(struct rw_interface *rw_p)
{
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct mrw_interface *mrw_p;
	if ( priv_p->p_type == RW_TYPE_MSERVER) {
		mrw_p = (struct mrw_interface *)priv_p->p_real_rw;
		mrw_p->rw_op->rw_ms_flush(mrw_p);
	}
}

static struct fpn_interface *
rw_get_fpn_p(struct rw_interface *rw_p)
{
	return ((struct rw_private *)rw_p->rw_private)->p_fpn_p;
}

static uint8_t
rw_get_device_ready(struct rw_interface *rw_p)
{
	char *log_header = "rw_get_device_ready";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface * drw_p;
	uint8_t ready = 0;

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		ready = drw_p->rw_op->rw_get_device_ready(drw_p);
		break;

	case RW_TYPE_MSERVER:
		ready = 1;
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return ready;
}

static void
rw_do_device_ready(struct rw_interface *rw_p)
{
	char *log_header = "rw_do_device_ready";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface * drw_p;

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		drw_p->rw_op->rw_do_device_ready(drw_p);
		break;

	case RW_TYPE_MSERVER:
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}


static void
rw_sync(struct rw_interface *rw_p, void *rw_handle) {
	char *log_header = "rw_sync";
	struct rw_private *priv_p = (struct rw_private *)rw_p->rw_private;
	struct drw_interface * drw_p;
	struct rw_handle* my_handle = (struct rw_handle*)rw_handle;

	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_p = (struct drw_interface *)priv_p->p_real_rw;
		drw_p->rw_op->rw_sync(drw_p, my_handle->p_sub_handle);
		break;

	case RW_TYPE_MSERVER:
		nid_log_error("%s: not support type (%d)", log_header, priv_p->p_type);
		assert(0);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

}

struct rw_operations rw_op = {
	.rw_create_worker = rw_create_worker,
	.rw_close = rw_close,
	.rw_trim = rw_trim,
	.rw_destroy = rw_destroy,

	.rw_pread = rw_pread,
	.rw_pwrite = rw_pwrite,
	.rw_pwritev = rw_pwritev,

	.rw_pwrite_async = rw_pwrite_async,
	.rw_pwritev_async = rw_pwritev_async,
	.rw_pread_async = rw_pread_async,
	.rw_sync = rw_sync,

	.rw_pread_fp = rw_pread_fp,
	.rw_pwrite_fp = rw_pwrite_fp,
	.rw_pwritev_fp = rw_pwritev_fp,

	.rw_pwrite_async_fp = rw_pwrite_async_fp,
	.rw_pwritev_async_fp = rw_pwritev_async_fp,
	.rw_pread_async_fp = rw_pread_async_fp,

	.rw_fetch_data = rw_fetch_data,
	.rw_fetch_fp = rw_fetch_fp,

	.rw_get_type = rw_get_type,
	.rw_get_name = rw_get_name,
	.rw_get_exportname = rw_get_exportname,
	.rw_get_fpn_p = rw_get_fpn_p,
	.rw_get_rw_obj = rw_get_rw_obj,
	.rw_ms_flush = rw_ms_flush,

	.rw_get_device_ready = rw_get_device_ready,
	.rw_do_device_ready = rw_do_device_ready

};

int
rw_initialization(struct rw_interface *rw_p, struct rw_setup *setup)
{
	char *log_header = "rw_initialization";
	struct rw_private *priv_p;
	struct drw_interface *drw_p;
	struct drw_setup drw_setup;
	struct mrw_interface *mrw_p;
	struct mrw_setup mrw_setup;
	int rc = 0;

	nid_log_info("%s: start ...", log_header);
	rw_p->rw_op = &rw_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	rw_p->rw_private = priv_p;
	priv_p->p_type = setup->type;
	priv_p->p_fpn_p = setup->fpn_p;
	strcpy(priv_p->p_name, setup->name);
	switch(priv_p->p_type) {
	case RW_TYPE_DEVICE:
		drw_setup.fpn_p = setup->fpn_p;
		strcpy(drw_setup.exportname, setup->exportname);
		strcpy(drw_setup.name, priv_p->p_name);
		drw_setup.device_provision = setup->device_provision;
		drw_setup.simulate_async = setup->simulate_async;
		drw_setup.simulate_delay = setup->simulate_delay;
		drw_setup.simulate_delay_max_gap = setup->simulate_delay_max_gap;
		drw_setup.simulate_delay_min_gap = setup->simulate_delay_min_gap;
		drw_setup.simulate_delay_time_us = setup->simulate_delay_time_us;
		drw_p = x_calloc(1, sizeof(*drw_p));
		drw_initialization(drw_p, &drw_setup);
		priv_p->p_real_rw = (void *)drw_p;
		strcpy(priv_p->p_exportname, setup->exportname);
		break;

	case RW_TYPE_MSERVER:
		mrw_setup.fpn_p = setup->fpn_p;
		mrw_setup.allocator = setup->allocator;
		mrw_p = x_calloc(1, sizeof(*mrw_p));
		mrw_initialization(mrw_p, &mrw_setup);
		priv_p->p_real_rw = (void *)mrw_p;
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
		rc = -1;
	}

	return rc;
}
