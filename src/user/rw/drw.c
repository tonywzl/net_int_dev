/*
 * drw.c
 *     Implementation of Device Read Write Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <sys/uio.h>
#include <errno.h>

#include "list.h"
#include "nid_log.h"
#include "drw_if.h"
#include "fpc_if.h"
#include "fpn_if.h"
#include "util_bdev.h"

#define DRW_FPC_BLOCK_SIZE 4096
#define DRW_CAL_DELAY_GAP(base, max_gap) \
	1 + base + rand() % (max_gap - base)

struct drw_worker {
	struct list_head	w_list;
	int			w_fhandle;
	char			*w_exportname;
	char			w_rw_sync;
	char			w_direct_io;
	uint32_t		sec_size;	// block device sector size
	uint64_t 		dev_size;	// device size in bytes
	uint8_t			is_dev;		// target device is real bock device or only a file
};

struct drw_private {
	pthread_mutex_t		p_lck;
	struct list_head	p_worker_head;
	struct fpc_interface	p_fpc;
	struct fpn_interface	*p_fpn_p;
	int			p_fp_size;

	char			p_simulate_async;
	char			p_simulate_random_mess;
	int			p_simulate_random_mess_current_num;
	int			p_simulate_random_mess_num;
	int			p_simulate_delay;
	int			p_simulate_delay_min_gap;
	int			p_simulate_delay_max_gap;
	int			p_simulate_delay_time;
	int			p_simulate_delay_rd_num;
	int			p_simulate_delay_cur_rd_num;
	int			p_simulate_delay_wr_num;
	int			p_simulate_delay_cur_wr_num;

	pthread_mutex_t         p_rlck;
	pthread_mutex_t         p_wlck;
	char			p_stop;
	char			p_write_async_stopped;
	char			p_read_async_stopped;
	struct list_head	p_write_op_head;
	pthread_cond_t		p_wasync_cond;
	struct list_head	p_read_op_head;
	pthread_cond_t		p_rasync_cond;
	uint8_t			p_device_ready;	// flag for target device readiness when device check is enabled
	uint8_t			p_worker_counter;
};

static int
__drw_open_device(char *exportname, char rw_sync, char direct_io)
{
	char *log_header = "__drw_open_device";
	mode_t mode;
	int fd, flags;

	mode = 0600;
	flags = O_RDWR;
	if (rw_sync)
		flags |= O_SYNC;
	if (direct_io)
		flags |= O_DIRECT;

try_open:
	fd = open(exportname, flags, mode);
	if (fd < 0) {
		nid_log_error("%s cannot open %s, errno:%d", log_header, exportname, errno);
		sleep(1);
		goto try_open;
	}

	return fd;
}

static void *
drw_create_worker(struct drw_interface *drw_p, char *exportname, char rw_sync, char direct_io)
{
	char *log_header = "drw_create_worker";
	struct drw_private *priv_p = (struct drw_private *)drw_p->rw_private;
	struct drw_worker *wp;
	int fd = -1;

	nid_log_warning("%s: start ...", log_header);

	if (priv_p->p_device_ready) {
		nid_log_warning("%s: target device:%s is ready", log_header, exportname);
		fd = __drw_open_device(exportname, rw_sync, direct_io);
		if (fd < 0)
			return NULL;
	}

	wp = x_calloc(1, sizeof(*wp));
	wp->w_exportname = exportname;
	wp->w_rw_sync = rw_sync;
	wp->w_direct_io = direct_io;
	wp->w_fhandle = fd;

	if(util_bdev_getinfo(fd, &wp->dev_size, &wp->sec_size, &wp->is_dev)) {
		nid_log_error("%s: failed to apply ioctl on device: %s", log_header, exportname);
		close(fd);
		free(wp);
		wp = NULL;
		goto lbl_exit;
	}

	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&wp->w_list, &priv_p->p_worker_head);
	priv_p->p_worker_counter++;
	pthread_mutex_unlock(&priv_p->p_lck);

lbl_exit:

	return wp;
}

static uint8_t
drw_get_device_ready(struct drw_interface *drw_p)
{
	struct drw_private *priv_p = (struct drw_private *)drw_p->rw_private;

	return priv_p->p_device_ready;
}

static void
drw_do_device_ready(struct drw_interface *drw_p)
{
	char *log_header = "drw_do_device_ready";
	struct drw_private *priv_p = (struct drw_private *)drw_p->rw_private;
	struct drw_worker *wp;
	int fd;

	nid_log_warning("%s: start ...", log_header);

	if (priv_p->p_device_ready)
		return;

	list_for_each_entry(wp, struct drw_worker, &priv_p->p_worker_head, w_list) {
		fd = __drw_open_device(wp->w_exportname, wp->w_rw_sync, wp->w_direct_io);
		if (fd < 0)
			return;
		wp->w_fhandle = fd;
	}

	priv_p->p_device_ready = 1;
}

enum drw_io_type {
	drw_io_read,
	drw_io_write,
	drw_io_writev
};

struct drw_io_op {
	struct list_head	iop_list;
	enum drw_io_type	iop_type;
	void			*iop_buf;
	size_t			iop_cnt;
	off_t			iop_off;
	struct iovec		*iop_iov;
	int			iop_iovcnt;
	rw_callback_fn 		iop_callback;
	struct rw_callback_arg 	*iop_arg;
	struct drw_interface 	*iop_drw_p;
	void			*iop_rw_handle;
	uint64_t 		io_squence;
};

static ssize_t
drw_pread(struct drw_interface *drw_p, void *rw_handle, void *buf, size_t count, off_t offset)
{
	struct drw_private *priv_p = (struct drw_private *)drw_p->rw_private;
	struct drw_worker *wp = (struct drw_worker *)rw_handle;
	int fd = wp->w_fhandle;
	assert(priv_p);
	if (priv_p->p_simulate_delay == 1 || priv_p->p_simulate_delay == 3) {
		if (priv_p->p_simulate_delay_cur_rd_num++ == priv_p->p_simulate_delay_rd_num) {
			nid_log_error("Read USLEEP: %d, %d %d", priv_p->p_simulate_delay_time,
					priv_p->p_simulate_delay_cur_rd_num, priv_p->p_simulate_delay);
			usleep(priv_p->p_simulate_delay_time);
			priv_p->p_simulate_delay_rd_num += DRW_CAL_DELAY_GAP(priv_p->p_simulate_delay_min_gap,
					priv_p->p_simulate_delay_max_gap);
		}
	}
	return pread(fd, buf, count, offset);
}

static int
drw_trim(void *rw_handle, off_t offset, size_t len)
{
	char *log_header = "drw_trim";
	struct drw_worker *wp = (struct drw_worker*)rw_handle;
	int fd = wp->w_fhandle, ret;

	ret = util_bdev_trim(fd, offset, len, wp->dev_size, wp->sec_size, wp->is_dev);
	if(ret) {
		nid_log_error("%s: failed to apply trim command to block device %s",
						log_header, wp->w_exportname);
	}

	return ret;
}


static ssize_t
drw_pwrite(struct drw_interface *drw_p, void *rw_handle, void *buf, size_t count, off_t offset)
{
	struct drw_private *priv_p = drw_p->rw_private;
	struct drw_worker *wp = (struct drw_worker*)rw_handle;
	int fd = wp->w_fhandle;
	assert(priv_p);
	if (priv_p->p_simulate_delay > 1) {
		if (priv_p->p_simulate_delay_cur_wr_num++ == priv_p->p_simulate_delay_wr_num) {
			nid_log_error("Write USLEEP: %d, %d %d", priv_p->p_simulate_delay_time,
								priv_p->p_simulate_delay_cur_wr_num, priv_p->p_simulate_delay);
			usleep(priv_p->p_simulate_delay_time);
			priv_p->p_simulate_delay_wr_num += DRW_CAL_DELAY_GAP(priv_p->p_simulate_delay_min_gap,
					priv_p->p_simulate_delay_max_gap);
		}
	}
	return pwrite(fd, buf, count, offset);
}

static ssize_t
drw_pwritev(struct drw_interface *drw_p, void *rw_handle, struct iovec *iov, int iovcnt, off_t offset)
{
	char *log_header = "drw_pwritev";
	struct drw_private *priv_p = drw_p->rw_private;
	struct drw_worker *wp = (struct drw_worker*)rw_handle;
	int fd = wp->w_fhandle;
	ssize_t ret, io_size = 0;
	int i, retry = 3;

	assert(priv_p);
	if (priv_p->p_simulate_delay > 1) {
		if (priv_p->p_simulate_delay_cur_wr_num++ == priv_p->p_simulate_delay_wr_num) {
			nid_log_error("Writev USLEEP: %d, %d %d", priv_p->p_simulate_delay_time,
								priv_p->p_simulate_delay_cur_wr_num, priv_p->p_simulate_delay);
			usleep(priv_p->p_simulate_delay_time);
			priv_p->p_simulate_delay_wr_num += DRW_CAL_DELAY_GAP(priv_p->p_simulate_delay_min_gap,
					priv_p->p_simulate_delay_max_gap);
		}
	}

	for (i = 0; i < iovcnt; i++) {
		io_size += iov[i].iov_len;
	}

retry:
	ret = pwritev(fd, iov, iovcnt, offset);
	if (ret == -1) {
		nid_log_error("%s, write target device error:%d, retry:%d", log_header, errno, retry);
		if (retry--) {
			usleep(100000);
			goto retry;
		} else {
			goto out;
		}
	} else if (ret != io_size) {
		nid_log_notice("%s, going to retry pwritev, io_size:%ld, ret:%ld", log_header, io_size, ret);
		goto retry;
	}

out:
	return ret;
}

static volatile uint64_t io_read_squence = 0;
static volatile uint64_t io_read_done_squence = 0;
static volatile uint64_t io_write_squence = 0;
static volatile uint64_t io_write_done_squence = 0;

static void
__drw_post_op(struct drw_interface *drw_p,
	void			*rw_handle,
	enum drw_io_type	iop_type,
	void			*iop_buf,
	int			iop_viocnt,
	size_t			iop_bufcnt,
	off_t			iop_off,
	rw_callback_fn 		iop_callback,
	struct rw_callback_arg 	*iop_arg)
{
	char *log_header = "__drw_post_op";

	struct drw_private *priv_p = drw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	struct drw_io_op *iop, *iop1, *iop2;
	ssize_t ret;
	int do_mass = 0;

	iop = x_malloc(sizeof(struct drw_io_op));
	if (!iop ) {
		nid_log_error("%s: x_malloc failed.", log_header);
	}
	iop->iop_arg = iop_arg;
	iop->iop_type = iop_type;
	iop->iop_off = iop_off;
	iop->iop_callback = iop_callback;
	iop->iop_rw_handle = rw_handle;
	iop->iop_drw_p = drw_p;

	if (iop->iop_type == drw_io_write || iop->iop_type == drw_io_read) {
		iop->iop_buf = iop_buf;
		iop->iop_iov = NULL;
		iop->iop_cnt = iop_bufcnt;
		iop->iop_iovcnt = 0;
		if (iop->iop_type == drw_io_read) {
			iop->io_squence = __sync_fetch_and_add(&io_read_squence, 1);
		} else {
			iop->io_squence = __sync_fetch_and_add(&io_write_squence, 1);
		}

	} else if (iop->iop_type == drw_io_writev) {
		iop->iop_buf = NULL;
		iop->iop_iov = iop_buf;
		iop->iop_cnt = 0;
		iop->iop_iovcnt = iop_viocnt;
		iop->io_squence = __sync_fetch_and_add(&io_write_squence, 1);

	} else {
		assert(0);
	}

	(void)fpc_p;(void)fpn_p;(void)ret;
	if (iop->iop_type == drw_io_write || iop->iop_type == drw_io_writev) {
		pthread_mutex_lock(&priv_p->p_wlck);
		iop2 = NULL;
		list_for_each_entry_reverse(iop1, struct drw_io_op, &priv_p->p_write_op_head, iop_list) {
			if (iop1->io_squence > iop->io_squence) {
				iop2 = iop1;
			} else {
				assert(iop1->io_squence != iop->io_squence);
				break;
			}
		}
		if (priv_p->p_simulate_random_mess) {
			priv_p->p_simulate_random_mess_current_num++;
			if (priv_p->p_simulate_random_mess_current_num == priv_p->p_simulate_random_mess_num) {
				priv_p->p_simulate_random_mess_num += 2 + rand() % 4;
				do_mass = 1;
			} else {
				do_mass = 0;
			}
		}

		if (do_mass) {
			if (!iop2) {
				list_add(&iop->iop_list, &priv_p->p_write_op_head);
			} else {
				list_add(&iop->iop_list, &iop2->iop_list);
			}
		} else {
			if (!iop2) {
				list_add_tail(&iop->iop_list, &priv_p->p_write_op_head);
			} else {
				list_add_tail(&iop->iop_list, &iop2->iop_list);
			}
		}
		pthread_cond_signal(&priv_p->p_wasync_cond);
		pthread_mutex_unlock(&priv_p->p_wlck);

	} else {
		pthread_mutex_lock(&priv_p->p_rlck);
		iop2 = NULL;
		list_for_each_entry_reverse(iop1, struct drw_io_op, &priv_p->p_read_op_head, iop_list) {
			if (iop1->io_squence > iop->io_squence) {
				iop2 = iop1;
			} else {
				assert(iop1->io_squence != iop->io_squence);
				break;
			}
		}

		if (!iop2) {
			list_add_tail(&iop->iop_list, &priv_p->p_read_op_head);
		} else {
			list_add_tail(&iop->iop_list, &iop2->iop_list);
		}
		pthread_cond_signal(&priv_p->p_rasync_cond);
		pthread_mutex_unlock(&priv_p->p_rlck);
	}
}

static ssize_t
drw_pread_fp(struct drw_interface *drw_p, void *rw_handle, void *buf, size_t count, off_t offset, struct list_head *fp_head)
{
	struct drw_private *priv_p = drw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	ssize_t ret;

	ret = drw_pread(drw_p, rw_handle, buf, count, offset);
	if (ret > 0) {
		INIT_LIST_HEAD(fp_head);
		fpc_p->fpc_op->fpc_bcalculate(fpc_p, fpn_p, buf, ret, DRW_FPC_BLOCK_SIZE, fp_head);
	}
	return ret;
}


static ssize_t
drw_pwrite_fp(struct drw_interface *drw_p, void *rw_handle, void *buf, size_t count, off_t offset, struct list_head *fp_head)
{
	struct drw_private *priv_p = drw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	ssize_t ret;

	ret = drw_pwrite(drw_p, rw_handle, buf, count, offset);
	if (ret > 0) {
		INIT_LIST_HEAD(fp_head);
		fpc_p->fpc_op->fpc_bcalculate(fpc_p, fpn_p, buf, ret, DRW_FPC_BLOCK_SIZE, fp_head);
	}
	return ret;
}

static ssize_t
drw_pwritev_fp(struct drw_interface *drw_p, void *rw_handle, struct iovec *iov, int iovcnt, off_t offset, struct list_head *fp_head)
{
	struct drw_private *priv_p = drw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	ssize_t ret;

	ret = drw_pwritev(drw_p, rw_handle, iov, iovcnt, offset);

	if (ret > 0) {
		assert((ret & (DRW_FPC_BLOCK_SIZE - 1)) == 0);
		INIT_LIST_HEAD(fp_head);
		fpc_p->fpc_op->fpc_bcalculatev(fpc_p, fpn_p, iov, iovcnt, DRW_FPC_BLOCK_SIZE, fp_head);
	}
	return ret;
}

static int
drw_close(struct drw_interface *drw_p, void *rw_handle)
{
	char *log_header = "drw_close";
	struct drw_private *priv_p = drw_p->rw_private;
	struct drw_worker *wp = (struct drw_worker *)rw_handle;
	int rc = 0;

	nid_log_warning("%s: start (%s)...", log_header, wp->w_exportname);
	assert(priv_p->p_worker_counter);
	pthread_mutex_lock(&priv_p->p_lck);
	list_del(&wp->w_list);	// removed from p_chan_head
	priv_p->p_worker_counter--;
	pthread_mutex_unlock(&priv_p->p_lck);
	if (wp->w_fhandle > 0)
		rc = close(wp->w_fhandle);
	free(wp);
	return rc;
}

static int
drw_stop(struct drw_interface *drw_p)
{
	char *log_header = "drw_stop";
	struct drw_private *priv_p = drw_p->rw_private;
	nid_log_warning("%s: start ...", log_header);
	priv_p->p_stop = 1;
	return 0;
}

static void
drw_destroy(struct drw_interface *drw_p)
{
	char *log_header = "drw_destroy";
	struct drw_private *priv_p = drw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	int nretry = 0;

	nid_log_warning("%s: start ...", log_header);
	drw_stop(drw_p);
retry1:
	pthread_mutex_lock(&priv_p->p_lck);
	if (priv_p->p_worker_counter) {
		pthread_mutex_unlock(&priv_p->p_lck);
		nid_log_warning("%s: worker_counter:%u, going to retry:%d",
				log_header, priv_p->p_worker_counter, ++nretry);
		sleep(1);
		goto retry1;
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (priv_p->p_simulate_async) {
		nretry = 0;
retry2:
		if (!priv_p->p_write_async_stopped || !priv_p->p_read_async_stopped) {
			nid_log_warning("%s: write_async_stopped:%d, read_async_stopped:%d, going to retry:%d",
					log_header,
					(int)priv_p->p_write_async_stopped, (int)priv_p->p_read_async_stopped,
					++nretry);
			sleep(1);
			goto retry2;
		}
		pthread_cond_destroy(&priv_p->p_wasync_cond);
		pthread_cond_destroy(&priv_p->p_rasync_cond);
		pthread_mutex_destroy(&priv_p->p_rlck);
		pthread_mutex_destroy(&priv_p->p_wlck);
	}

	fpc_p->fpc_op->fpc_destroy(fpc_p);
	pthread_mutex_destroy(&priv_p->p_lck);
	free((void *)priv_p);
	drw_p->rw_private = NULL;
	nid_log_warning("%s: end ...", log_header);
}

void
drw_pwrite_async_fp(struct drw_interface *drw_p, void *rw_handle, void *buf,
	size_t count, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{
	struct drw_private *priv_p = drw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	ssize_t ret;

	assert(count > 0);
	if (priv_p->p_simulate_async) {
		__drw_post_op(drw_p, rw_handle, drw_io_write, buf, 0, count, offset, rw_callback, arg);
	} else {
retry:
		ret = drw_pwrite(drw_p, rw_handle, buf, count, offset);
		INIT_LIST_HEAD(&arg->ag_fp_head);
		if (ret > 0) {
			assert((ret & (DRW_FPC_BLOCK_SIZE - 1)) == 0);
			fpc_p->fpc_op->fpc_bcalculate(fpc_p, fpn_p, buf, ret, DRW_FPC_BLOCK_SIZE, &arg->ag_fp_head);
		} else if (errno == EAGAIN) {
			nid_log_error("Writev Error: EAGAIN try write again. ret: %ld", ret);
			goto retry;
		} else {
			nid_log_error("Writev Error: errno:%d, ret: %ld", errno, ret);
		}
		rw_callback(ret == 0 ? -1 : (ret < 0 ? ret: 0), arg);
	}
}

static void
drw_pwritev_async_fp(struct drw_interface *drw_p, void *rw_handle, struct iovec *iov, int iovcnt,
	off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{
	struct drw_private *priv_p = drw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	ssize_t ret;

	assert(iov->iov_len > 0);
	if (priv_p->p_simulate_async) {
		__drw_post_op(drw_p, rw_handle, drw_io_writev, iov, iovcnt, 0, offset, rw_callback, arg);
	} else {
retry:
		ret = drw_pwritev(drw_p, rw_handle, iov, iovcnt, offset);
		INIT_LIST_HEAD(&arg->ag_fp_head);
		if (ret > 0) {
			assert((ret & (DRW_FPC_BLOCK_SIZE - 1)) == 0);
			fpc_p->fpc_op->fpc_bcalculatev(fpc_p, fpn_p, iov, iovcnt, DRW_FPC_BLOCK_SIZE, &arg->ag_fp_head);
		} else if (errno == EAGAIN) {
			nid_log_error("Writev Error: EAGAIN try write again.");
			goto retry;
		} else {
			nid_log_error("Writev Error: errno:%d", errno);
		}
		rw_callback(ret == 0 ? -1 : (ret < 0 ? ret: 0), arg);
	}
}

static void
drw_pread_async_fp(struct drw_interface *drw_p, void *rw_handle, void *buf,
	size_t count, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{
	struct drw_private *priv_p = drw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	ssize_t ret;
	if (priv_p->p_simulate_async) {
		__drw_post_op(drw_p, rw_handle, drw_io_read, buf, 0, count, offset, rw_callback, arg);
	} else {
		ret = drw_pread(drw_p, rw_handle, buf, count, offset);
		INIT_LIST_HEAD(&arg->ag_fp_head);
		if (ret > 0) {
			assert((ret & (DRW_FPC_BLOCK_SIZE - 1)) == 0);
			fpc_p->fpc_op->fpc_bcalculate(fpc_p, fpn_p, buf, ret, DRW_FPC_BLOCK_SIZE, &arg->ag_fp_head);
		}
		rw_callback(ret < 0 ? ret : 0, arg);
	}
}

static void*
_drw_write_async_thread(void *p)
{
	struct drw_private *priv_p = p;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	ssize_t ret;

next_try:
	if (priv_p->p_stop)
		goto out;

	pthread_mutex_lock(&priv_p->p_wlck);
	while (list_empty(&priv_p->p_write_op_head)) {
		pthread_cond_wait(&priv_p->p_wasync_cond, &priv_p->p_wlck);
		if (priv_p->p_stop) {
			pthread_mutex_unlock(&priv_p->p_wlck);
			sleep(1);
			goto next_try;
		}
	}

	while (!list_empty(&priv_p->p_write_op_head)) {
		struct drw_io_op *iop = list_first_entry(&priv_p->p_write_op_head, struct drw_io_op, iop_list);
		if ((!priv_p->p_simulate_random_mess) && iop->io_squence != io_write_done_squence) {
			pthread_cond_wait(&priv_p->p_wasync_cond, &priv_p->p_wlck);
			continue;
		}
		io_write_done_squence++;
		list_del(&iop->iop_list);
		pthread_mutex_unlock(&priv_p->p_wlck);
		INIT_LIST_HEAD(&iop->iop_arg->ag_fp_head);
		if (iop->iop_type == drw_io_write) {

			ret = drw_pwrite(iop->iop_drw_p, iop->iop_rw_handle, iop->iop_buf, iop->iop_cnt, iop->iop_off);
			if (ret > 0) {
				assert((ret & (DRW_FPC_BLOCK_SIZE - 1)) == 0);
				fpc_p->fpc_op->fpc_bcalculate(fpc_p, fpn_p, iop->iop_buf, ret,
						DRW_FPC_BLOCK_SIZE, &iop->iop_arg->ag_fp_head);
			}

		} else if (iop->iop_type == drw_io_writev) {

			ret = drw_pwritev(iop->iop_drw_p, iop->iop_rw_handle,  iop->iop_iov, iop->iop_iovcnt, iop->iop_off);
			if (ret > 0) {
				assert((ret & (DRW_FPC_BLOCK_SIZE - 1)) == 0);
				fpc_p->fpc_op->fpc_bcalculatev(fpc_p, fpn_p, iop->iop_iov, iop->iop_iovcnt,
						DRW_FPC_BLOCK_SIZE, &iop->iop_arg->ag_fp_head);
			}
		}
		iop->iop_callback(ret < 0 ? ret : 0, iop->iop_arg);
		free(iop);
		pthread_mutex_lock(&priv_p->p_wlck);
	}

	pthread_mutex_unlock(&priv_p->p_wlck);
	goto next_try;

out:
	priv_p->p_write_async_stopped = 1;
	return NULL;

}

static void*
_drw_read_async_thread(void *p)
{
	struct drw_private *priv_p = p;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;
	ssize_t ret;

next_try:
	if (priv_p->p_stop)
		goto out;

	pthread_mutex_lock(&priv_p->p_rlck);
	while (list_empty(&priv_p->p_read_op_head)) {
		pthread_cond_wait(&priv_p->p_rasync_cond, &priv_p->p_rlck);
		if (priv_p->p_stop) {
			pthread_mutex_unlock(&priv_p->p_rlck);
			sleep(1);
			goto next_try;
		}
	}
	while (!list_empty(&priv_p->p_read_op_head)) {
		struct drw_io_op *iop = list_first_entry(&priv_p->p_read_op_head, struct drw_io_op, iop_list);
		if (iop->io_squence != io_read_done_squence) {
			pthread_cond_wait(&priv_p->p_rasync_cond, &priv_p->p_rlck);
			continue;
		}
		io_read_done_squence++;
		list_del(&iop->iop_list);
		pthread_mutex_unlock(&priv_p->p_rlck);
		INIT_LIST_HEAD(&iop->iop_arg->ag_fp_head);
		switch (iop->iop_type) {
		case drw_io_read:

			ret = drw_pread(iop->iop_drw_p, iop->iop_rw_handle,  iop->iop_buf, iop->iop_cnt, iop->iop_off);
			if (ret > 0) {
				assert((ret & (DRW_FPC_BLOCK_SIZE - 1)) == 0);
				fpc_p->fpc_op->fpc_bcalculate(fpc_p, fpn_p, iop->iop_buf, ret, DRW_FPC_BLOCK_SIZE, &iop->iop_arg->ag_fp_head);
			}
			break;
		default:
			assert(0);
		}
		iop->iop_callback(ret < 0 ? ret : 0, iop->iop_arg);
		free(iop);
		pthread_mutex_lock(&priv_p->p_rlck);
	}

	pthread_mutex_unlock(&priv_p->p_rlck);
	goto next_try;

out:
	priv_p->p_read_async_stopped = 1;
	return NULL;

}

static void
drw_sync(struct drw_interface *drw_p, void *rw_handle) {
	(void)drw_p;
	struct drw_worker *wp = (struct drw_worker *)rw_handle;
	fsync(wp->w_fhandle);
}

struct drw_operations drw_op = {
	.rw_create_worker = drw_create_worker,

	.rw_pread = drw_pread,
	.rw_pwrite = drw_pwrite,
	.rw_pwritev = drw_pwritev,

	.rw_pread_fp = drw_pread_fp,
	.rw_pwrite_fp = drw_pwrite_fp,
	.rw_pwritev_fp = drw_pwritev_fp,

	.rw_sync = drw_sync,

	.rw_close = drw_close,
	.rw_stop = drw_stop,
	.rw_destroy = drw_destroy,
	.rw_trim = drw_trim,

	.rw_pwrite_async_fp = drw_pwrite_async_fp,
	.rw_pwritev_async_fp = drw_pwritev_async_fp,
	.rw_pread_async_fp = drw_pread_async_fp,

	.rw_get_device_ready = drw_get_device_ready,
	.rw_do_device_ready = drw_do_device_ready,
};

int
drw_initialization(struct drw_interface *drw_p, struct drw_setup *setup)
{
	char *log_header = "drw_initialization";
	struct drw_private *priv_p;
	struct fpc_setup fpc_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	drw_p->rw_op = &drw_op;
	priv_p = x_calloc(1, sizeof(*priv_p));

	if (!setup->device_provision)
		priv_p->p_device_ready = 1;

	drw_p->rw_private = priv_p;
	priv_p->p_fpn_p = setup->fpn_p;
	priv_p->p_fp_size = priv_p->p_fpn_p->fp_op->fp_get_fp_size(priv_p->p_fpn_p);
	priv_p->p_simulate_async = setup->simulate_async;
	if (priv_p->p_simulate_async) {
		priv_p->p_simulate_random_mess = 0;
		priv_p->p_simulate_random_mess_num = 2 + rand() % 4;
		priv_p->p_simulate_random_mess_current_num = 0;
	}
	priv_p->p_simulate_delay = setup->simulate_delay;
	if (priv_p->p_simulate_delay) {
		priv_p->p_simulate_delay_min_gap = setup->simulate_delay_min_gap;
		priv_p->p_simulate_delay_max_gap = setup->simulate_delay_max_gap;
		priv_p->p_simulate_delay_time = setup->simulate_delay_time_us;
		priv_p->p_simulate_delay_rd_num = DRW_CAL_DELAY_GAP(setup->simulate_delay_min_gap,
				setup->simulate_delay_max_gap);
		priv_p->p_simulate_delay_cur_rd_num = 0;
		priv_p->p_simulate_delay_wr_num = DRW_CAL_DELAY_GAP(setup->simulate_delay_min_gap,
				setup->simulate_delay_max_gap);
		priv_p->p_simulate_delay_cur_wr_num = 0;
	}

	INIT_LIST_HEAD(&priv_p->p_worker_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);

	fpc_setup.fpc_algrm = FPC_SHA224;
	fpc_initialization(&priv_p->p_fpc, &fpc_setup);

	if (priv_p->p_simulate_async) {
		pthread_t tid;
		pthread_attr_t attr, attr1;

		INIT_LIST_HEAD(&priv_p->p_write_op_head);
		INIT_LIST_HEAD(&priv_p->p_read_op_head);
		pthread_cond_init(&priv_p->p_wasync_cond, NULL);
		pthread_cond_init(&priv_p->p_rasync_cond, NULL);
		pthread_mutex_init(&priv_p->p_rlck, NULL);
		pthread_mutex_init(&priv_p->p_wlck, NULL);

		pthread_attr_init(&attr);
		pthread_attr_init(&attr1);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);

		pthread_create(&tid, &attr, _drw_write_async_thread, (void*)priv_p);
		pthread_create(&tid, &attr1, _drw_read_async_thread, (void*)priv_p);
	}

	return 0;
}

