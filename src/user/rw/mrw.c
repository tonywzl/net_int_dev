/*
 * mrw.c
 * 	Implementation of Meta Server Read Write Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <sys/uio.h>
#include <sys/time.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "mrw_if.h"
#include "fpc_if.h"
#include "fpn_if.h"
#include "mrw_rn_if.h"
#include "mrw_wn_if.h"
#include "mrw_wsn_if.h"
#ifdef METASERVER
#include "ms_intf.h"
#endif


#define MRW_WRITE_OP_MAX 4096		// Max write operation to the MDS
#define MRW_AWRITE_BLOCKS_MAX 4096	// Max write block number for a write operation
#define MRW_FPC_BLOCK_SIZE 4096		// Finger print calculated block size
#define MRW_FPC_BLOCK_SHIFT 12		// Finger print calculated block shift
#define MRW_FLUSH_SIZE_MAX (1 << 30) 	// Max flush size.
#define MRW_DIV_UP(n, m) ((n + m - 1) / m)
#define TIME_CONSUME_ALERT 1000000
#define time_difference(t1, t2) \
	((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

#ifdef METASERVER
static char *default_snapshot_id = NULL;
#endif

struct mrw_worker {
	struct list_head	w_list;
	char			w_volumn_uuid[NID_MAX_UUID];
	char			*w_snap_uuid;
	int			w_device_size;
};

struct mrw_private {
	pthread_mutex_t          p_lck;
	struct list_head         p_worker_head;
	pthread_cond_t           p_wcond;
	pthread_cond_t           p_fcond;
	volatile int             p_sent_wop_num;
	volatile int		 p_sent_wfp_num;
	size_t                   p_max_write_size;
	size_t                   p_fp_size;
	struct mrw_wn_interface  p_mrw_wn;
	struct mrw_wsn_interface p_mrw_wsn;
	struct mrw_rn_interface  p_mrw_rn;
	struct fpc_interface     p_fpc;
	struct fpn_interface     *p_fpn_p;
	int			 p_writing;
	bool		 	 p_fast_flush;
	volatile char		 p_prepare_fast_flush;
	bool			 p_max_write_op_limit;
};

struct mrw_sync_callback_arg {
	pthread_cond_t           rc_sync_rcond;
	char                     rc_busy;
	pthread_mutex_t          rc_lock;
	int                      rc_ret;
};

static void
mrw_get_status (struct mrw_interface *mrw_p, struct mrw_stat *info)
{
	struct mrw_private *priv_p = (struct mrw_private *) mrw_p->rw_private;
	info->sent_wfp_num = priv_p->p_sent_wfp_num;
	info->sent_wop_num = priv_p->p_sent_wop_num;

}

#ifdef METASERVER
static void *
mrw_create_worker(struct mrw_interface *mrw_p, char *volumnuuid, int dev_sz)
{
	char *log_header = "mrw_create_worker";
	struct mrw_private *priv_p = (struct mrw_private *)mrw_p->rw_private;
	struct mrw_worker *wp = NULL;

	nid_log_notice("%s: start ...", log_header);
	wp = calloc(1, sizeof(*wp));
	strcpy(wp->w_volumn_uuid, volumnuuid);
	wp->w_snap_uuid = default_snapshot_id; // TODO: Set snap uuid.
	wp->w_device_size = dev_sz;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&wp->w_list, &priv_p->p_worker_head);
	pthread_mutex_unlock(&priv_p->p_lck);

	return wp;
}

static void
_mrw_sync_callback(int err, void* arg)
{
	struct mrw_sync_callback_arg *argp = arg;
	argp->rc_ret = err;
	pthread_mutex_lock(&argp->rc_lock);
	argp->rc_busy = 0;
	pthread_cond_broadcast(&argp->rc_sync_rcond);
	pthread_mutex_unlock(&argp->rc_lock);
}

static ssize_t
_mrw_pread(struct mrw_interface *mrw_p, void *rw_handle, void *buf,
	size_t count, off_t offset, struct list_head *fp_head)
{

	struct mrw_worker *wp = (struct mrw_worker *)rw_handle;
	struct mrw_sync_callback_arg arg, *argp = &arg;
	struct mrw_private *priv_p = mrw_p->rw_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;

	pthread_mutex_init(&argp->rc_lock, NULL);
	pthread_cond_init(&argp->rc_sync_rcond, NULL);
	argp->rc_busy = 1;

	MS_Read_Async(wp->w_volumn_uuid, wp->w_snap_uuid, buf, offset, count, (Callback)_mrw_sync_callback, (void *)argp, 0);
	pthread_mutex_lock(&argp->rc_lock);
	if (argp->rc_busy) {
		pthread_cond_wait(&argp->rc_sync_rcond, &argp->rc_lock);
	}
	pthread_mutex_unlock(&argp->rc_lock);

	if (fp_head) {
		INIT_LIST_HEAD(fp_head);
		fpc_p->fpc_op->fpc_bcalculate(fpc_p, fpn_p, buf, count, MRW_FPC_BLOCK_SIZE, fp_head);
	}

	return argp->rc_ret == 0 ? (ssize_t)count : -1;
}

static ssize_t
mrw_pread(struct mrw_interface *mrw_p, void *rw_handle, void *buf, size_t count, off_t offset)
{
	return _mrw_pread(mrw_p, rw_handle, buf, count, offset, NULL);
}

static ssize_t
mrw_pread_fp(struct mrw_interface *mrw_p, void *rw_handle, void *buf,
	size_t count, off_t offset, struct list_head *fp_head)
{
	return _mrw_pread(mrw_p, rw_handle, buf, count, offset, fp_head);
}

void
_mrw_sync_calculate_fingerprint(struct mrw_private *priv_p, struct iovec *vec,
	int veccnt, char *fp_buffer, size_t fp_num, struct list_head *fp_head)
{
	int i;
	void *blkbuf, *fpbuf;
	struct iovec *iov;
	size_t iov_offset;
	struct fp_node *fp_nd;

	nid_log_debug("WRITE: __mrw_calculate_fp: fp_num:%lu mrw_arg->wa_iov_cnt:%d mrw_arg->wa_fp_buffer:%p",
	    fp_num, veccnt, fp_buffer);
	fpbuf = fp_buffer;
	for (i=0; i<veccnt; i++) {
		iov = &vec[i];
		iov_offset = 0;
		while (iov_offset < iov->iov_len) {
			blkbuf = iov->iov_base + iov_offset;
			iov_offset += MRW_FPC_BLOCK_SIZE;
			priv_p->p_fpc.fpc_op->fpc_calculate(&priv_p->p_fpc, blkbuf, MRW_FPC_BLOCK_SIZE, (unsigned char*)fpbuf);
			if (fp_head) {
				fp_nd = priv_p->p_fpn_p->fp_op->fp_get_node(priv_p->p_fpn_p);
				memcpy(fp_nd->fp, fpbuf, priv_p->p_fp_size);
				list_add_tail(&fp_nd->fp_list, fp_head);
			}
			fpbuf += priv_p->p_fp_size;
		}
	}
}

static ssize_t
_mrw_pwrite(struct mrw_interface *mrw_p, void *rw_handle, void *buf,
	size_t count, off_t offset, struct list_head *fp_head)
{
	struct mrw_sync_callback_arg arg, *argp = &arg;
	struct mrw_private *priv_p = mrw_p->rw_private;
	struct mrw_worker *wp = (struct mrw_worker*)rw_handle;
	struct iovec  iov[1];
	void *buff = (void *)buf;
	size_t fp_num = count >> MRW_FPC_BLOCK_SHIFT;
	char fpbuf[fp_num * priv_p->p_fp_size];

	assert((count & (MRW_FPC_BLOCK_SIZE - 1)) == 0);
	iov[0].iov_base = buff;
	iov[0].iov_len = count;
	pthread_mutex_init(&argp->rc_lock, NULL);
	pthread_cond_init(&argp->rc_sync_rcond, NULL);
	argp->rc_busy = 1;
	_mrw_sync_calculate_fingerprint(priv_p, iov, 1, fpbuf, fp_num, fp_head);
	nid_log_debug("_mrw_pwrite: volumn_id:%s buf:%p offset:%ld count:%lu",
			wp->w_volumn_uuid, (void *)buf, offset, count);
	MS_Write_Async(wp->w_volumn_uuid, wp->w_snap_uuid, (void *)buf, offset, count,
		(Callback)_mrw_sync_callback, (void *)argp, fpbuf, ioFlagFlush, 0);
	pthread_mutex_lock(&argp->rc_lock);
	if (argp->rc_busy) {
		pthread_cond_wait(&argp->rc_sync_rcond, &argp->rc_lock);
	}
	pthread_mutex_unlock(&argp->rc_lock);

	return argp->rc_ret == 0 ? (ssize_t)count : -1;
}

static ssize_t
mrw_pwrite(struct mrw_interface *mrw_p, void *rw_handle, void *buf, size_t count, off_t offset)
{
	return _mrw_pwrite(mrw_p, rw_handle, buf, count, offset, NULL);
}

static ssize_t
mrw_pwrite_fp(struct mrw_interface *mrw_p, void *rw_handle, void *buf, size_t count,
	off_t offset, struct list_head *fp_head)
{
	return _mrw_pwrite(mrw_p, rw_handle, buf, count, offset, fp_head);
}

static ssize_t
_mrw_pwritev(struct mrw_interface *mrw_p, void *rw_handle,  struct iovec *iov,
	int iovcnt, off_t offset, struct list_head *fp_head)
{
	struct mrw_sync_callback_arg arg, *argp = &arg;
	struct mrw_private *priv_p = mrw_p->rw_private;
	struct mrw_worker *wp = (struct mrw_worker*)rw_handle;
	size_t count = 0;
	int i;
	for (i=0; i<iovcnt; i++) {
	count += iov[i].iov_len;
	}
	assert((count & (MRW_FPC_BLOCK_SIZE - 1)) == 0);
	size_t fp_num = count >> MRW_FPC_BLOCK_SHIFT;
	char fpbuf[fp_num * priv_p->p_fp_size];

	pthread_mutex_init(&argp->rc_lock, NULL);
	pthread_cond_init(&argp->rc_sync_rcond, NULL);
	argp->rc_busy = 1;
	_mrw_sync_calculate_fingerprint(priv_p, (struct iovec *)iov, iovcnt, fpbuf, fp_num, fp_head);
	MS_Writev_Async(wp->w_volumn_uuid, 0, (struct iovec *)iov, offset, iovcnt, (Callback)_mrw_sync_callback, (void *)argp, fpbuf, ioFlagFlush, 0);
	pthread_mutex_lock(&argp->rc_lock);
	if (argp->rc_busy) {
		pthread_cond_wait(&argp->rc_sync_rcond, &argp->rc_lock);
	}
	pthread_mutex_unlock(&argp->rc_lock);

	return argp->rc_ret == 0 ? (ssize_t)count : -1;
}

static ssize_t
mrw_pwritev(struct mrw_interface *mrw_p, void *rw_handle,  struct iovec *iov, int iovcnt, off_t offset)
{
	return _mrw_pwritev(mrw_p, rw_handle, iov, iovcnt, offset, NULL);
}

static ssize_t
mrw_pwritev_fp(struct mrw_interface *mrw_p, void *rw_handle,  struct iovec *iov,
	int iovcnt, off_t offset, struct list_head *fp_head)
{
	return _mrw_pwritev(mrw_p, rw_handle, iov, iovcnt, offset, fp_head);
}

static int
mrw_close(struct mrw_interface *mrw_p, void *rw_handle)
{
	char *log_header = "mrw_close";
	struct mrw_private *priv_p = mrw_p->rw_private;
	struct mrw_worker *wp = (struct mrw_worker *)rw_handle;

	nid_log_notice("%s: close (volumn_id: %s)...", log_header, wp->w_volumn_uuid);
	// Do fast flush before mrw_close, make sure all write operation are flushed to MDS.
	mrw_p->rw_op->rw_ms_flush(mrw_p);

	pthread_mutex_lock(&priv_p->p_lck);
	list_del(&wp->w_list);	// removed from p_chan_head
	free(wp);
	pthread_mutex_unlock(&priv_p->p_lck);
	return 0;
}

static int
mrw_stop(struct mrw_interface *mrw_p)
{
	char *log_header = "mrw_stop";
	struct mrw_private *priv_p = mrw_p->rw_private;
	nid_log_notice("%s: start ...", log_header);
	assert(priv_p);
	Ms_Stop();
	return 0;
}

static void
_mrw_write_callback(int err, void* arg)
{

	struct mrw_wn *mrw_arg = arg;
	struct mrw_wsn *fpw = mrw_arg->wa_fpw_node;
	struct mrw_private *priv_p = fpw->fn_priv_p;
	size_t done_fpnum = mrw_arg->wa_fp_buf_num;
	size_t leftnum;

	leftnum = __sync_sub_and_fetch(&fpw->fn_not_done_fp_num, done_fpnum);
	__sync_sub_and_fetch(&priv_p->p_sent_wfp_num, done_fpnum);
	gettimeofday(&mrw_arg->wa_resp_time, NULL);
	nid_log_debug("WRITE: A sub write %p finished !! Left FP number:%lu", mrw_arg, fpw->fn_not_done_fp_num);
	if (time_difference(mrw_arg->wa_recv_time, mrw_arg->wa_resp_time) > TIME_CONSUME_ALERT) {
		nid_log_warning("_mrw_write_callback: SUB WRITE response delay %ldus, Left fp number:%d left write op number:%d",
				time_difference(mrw_arg->wa_recv_time, mrw_arg->wa_resp_time) ,
				priv_p->p_sent_wfp_num, priv_p->p_sent_wop_num - 1);
	}
	if (leftnum == 0) {
		// All done for this write
		gettimeofday(&fpw->fn_resp_time, NULL);
		if (time_difference(fpw->fn_recv_time, fpw->fn_resp_time) > TIME_CONSUME_ALERT) {
			nid_log_warning("_mrw_write_callback: WRITE response delay %ldus, Left fp number:%d left write op number:%d",
					time_difference(fpw->fn_recv_time, fpw->fn_resp_time),
					priv_p->p_sent_wfp_num, priv_p->p_sent_wop_num - 1);
		}
		fpw->fn_callback_upstair(err, fpw->fn_arg_upstair);
		nid_log_debug("WRITE: A write %p total finished !! Left fp number:%d left write op number:%d", fpw, priv_p->p_sent_wfp_num, priv_p->p_sent_wop_num - 1);
		// Free memory of fpw node
		priv_p->p_mrw_wsn.wsn_op->wsn_put_node(&priv_p->p_mrw_wsn, fpw);
	}

	// TODO Use pp2 put here
	free(mrw_arg->wa_fp_buf);

 	// Free memory of arg node
	priv_p->p_mrw_wn.wn_op->wn_put_node(&priv_p->p_mrw_wn, mrw_arg);
	// When the write operation number == MRW_WRITE_OP_MAX,
	// The write thread maybe sleep, so we need wake up it.
	if (priv_p->p_max_write_op_limit) {
		pthread_mutex_lock(&priv_p->p_lck);
		if (priv_p->p_sent_wop_num-- == MRW_WRITE_OP_MAX) {
			pthread_cond_broadcast(&priv_p->p_wcond);
		}
		pthread_mutex_unlock(&priv_p->p_lck);
	} else {
		__sync_sub_and_fetch(&priv_p->p_sent_wop_num, 1);
	}


}

static struct mrw_wn *
_mrw_mkfp_arg(struct mrw_private *priv_p, void *buf, size_t fp_num, struct mrw_wsn *fpw_node)
{

	size_t i;
	char *blkbuf, *fpbuf;
	struct fp_node *fp_nd;
	struct mrw_wn *	mrw_arg = (struct mrw_wn *)
			priv_p->p_mrw_wn.wn_op->wn_get_node(&priv_p->p_mrw_wn);

	// TODO Use pp2 get here
	mrw_arg->wa_fp_buf = malloc(priv_p->p_fp_size * fp_num);
	mrw_arg->wa_fp_buf_num = fp_num;
	mrw_arg->wa_fpw_node = fpw_node;
	gettimeofday(&mrw_arg->wa_recv_time, NULL);
	fpbuf = mrw_arg->wa_fp_buf;
	blkbuf = buf;
	for (i=0; i<fp_num; i++) {
		priv_p->p_fpc.fpc_op->fpc_calculate(&priv_p->p_fpc, blkbuf, MRW_FPC_BLOCK_SIZE, (unsigned char*)fpbuf);

		// Add new finger print node for return to read content cache.
		fp_nd = priv_p->p_fpn_p->fp_op->fp_get_node(priv_p->p_fpn_p);
		memcpy(fp_nd->fp, fpbuf, priv_p->p_fp_size);
		list_add_tail(&fp_nd->fp_list, &fpw_node->fn_arg_upstair->ag_fp_head);

		assert(priv_p->p_fp_size == NID_SIZE_FP);
		fpbuf += priv_p->p_fp_size;
		blkbuf += MRW_FPC_BLOCK_SIZE;
	}
	return mrw_arg;
}

static inline
void _mrw_write_before(struct mrw_private *priv_p, char *prepare_flushing)
{
	if (priv_p->p_max_write_op_limit) {
		/*Support MAX write operation limitation*/
		pthread_mutex_lock(&priv_p->p_lck);
		while (priv_p->p_sent_wop_num == MRW_WRITE_OP_MAX || priv_p->p_fast_flush == true) {
			pthread_cond_wait(&priv_p->p_wcond, &priv_p->p_lck);
		}
		assert(priv_p->p_fast_flush == false);
		priv_p->p_sent_wop_num ++;
		priv_p->p_writing ++;
		pthread_mutex_unlock(&priv_p->p_lck);
	} else {
		/*Not support MAX write operation limitation*/
		*prepare_flushing = priv_p->p_prepare_fast_flush;
		if (*prepare_flushing) {
			pthread_mutex_lock(&priv_p->p_lck);
			while (priv_p->p_fast_flush == true) {
				pthread_cond_wait(&priv_p->p_wcond, &priv_p->p_lck);
			}
			assert(priv_p->p_fast_flush == false);
			priv_p->p_sent_wop_num ++;
			priv_p->p_writing ++;
			pthread_mutex_unlock(&priv_p->p_lck);
		} else {
			__sync_add_and_fetch(&priv_p->p_sent_wop_num, 1);
		}
	}
}

static inline
void _mrw_write_end(struct mrw_private *priv_p, char prepare_flushing)
{
	if (priv_p->p_max_write_op_limit || prepare_flushing) {
		/*Support MAX write operation limitation*/
		pthread_mutex_lock(&priv_p->p_lck);
		priv_p->p_writing --;
		if (priv_p->p_fast_flush) {
			pthread_cond_signal(&priv_p->p_fcond);
		}
		pthread_mutex_unlock(&priv_p->p_lck);
	}
}

static void
mrw_pwrite_async_fp(struct mrw_interface *mrw_p, void *rw_handle, void *buf,
	size_t count, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{

	struct mrw_private *priv_p = mrw_p->rw_private;
	struct mrw_worker *wp = (struct mrw_worker*)rw_handle;
	struct mrw_wn *mrw_arg;
	struct mrw_wsn *fpw_node;
	int fp_num = count >> MRW_FPC_BLOCK_SHIFT, fp_num1;
	int split_num, i;
	void *buf1;
	size_t count1, more, more1;
	off_t offset1;
	char prepare_flushing;

	INIT_LIST_HEAD(&arg->ag_fp_head);
	fpw_node = (struct mrw_wsn *) priv_p->p_mrw_wsn.wsn_op->wsn_get_node(&priv_p->p_mrw_wsn);
	fpw_node->fn_not_done_fp_num = fp_num;
	__sync_add_and_fetch(&priv_p->p_sent_wfp_num, fp_num);
	fpw_node->fn_arg_upstair = arg;
	fpw_node->fn_callback_upstair = rw_callback;
	fpw_node->fn_priv_p = priv_p;
	gettimeofday(&fpw_node->fn_recv_time, NULL);

	split_num = MRW_DIV_UP(fp_num, MRW_AWRITE_BLOCKS_MAX);
	buf1 = (void *)buf;
	offset1 = offset;
	more = count;
	for (i=0; i<split_num; i++) {

		if (split_num - 1 == i) {
			fp_num1 = fp_num % MRW_AWRITE_BLOCKS_MAX;
			count1 = count % priv_p->p_max_write_size;
		} else {
			fp_num1 = MRW_AWRITE_BLOCKS_MAX;
			count1 = priv_p->p_max_write_size;
		}
		mrw_arg = _mrw_mkfp_arg(priv_p, buf1, fp_num1, fpw_node);
		more -= count1;
		more1 = more;
		nid_log_debug("WRITE: mrw_pwrite_async_fp: buf1:%p offset1:%ld count1:%lu fp_num1:%d",
				buf1, offset1, count1, fp_num1);
		_mrw_write_before(priv_p, &prepare_flushing);
		MS_Write_Async(wp->w_volumn_uuid, wp->w_snap_uuid, buf1, offset1, count1,
			(Callback)_mrw_write_callback, (void *)mrw_arg, mrw_arg->wa_fp_buf, 0, &more1);
		_mrw_write_end(priv_p, prepare_flushing);
		buf1 += priv_p->p_max_write_size;
		offset1 += priv_p->p_max_write_size;
	}
}

void
_mrw_calculate_fp_async(struct mrw_private *priv_p, struct mrw_wn *mrw_arg, size_t fp_num)
{
	int i;
	char *blkbuf, *fpbuf;
	struct iovec *iov;
	size_t iov_offset;
	struct fp_node *fp_nd;
	struct mrw_wsn *fpw = mrw_arg->wa_fpw_node;

	// TODO Use pp2 replace malloc
	mrw_arg->wa_fp_buf = malloc(priv_p->p_fp_size * fp_num);
	mrw_arg->wa_fp_buf_num = fp_num;
	nid_log_debug("WRITE: __mrw_calculate_fp: fp_num:%lu mrw_arg->wa_iov_cnt:%d mrw_arg->wa_fp_buffer:%p",
			fp_num, mrw_arg->wa_iov_cnt, mrw_arg->wa_fp_buf);

	fpbuf = mrw_arg->wa_fp_buf;
	for (i=0; i<mrw_arg->wa_iov_cnt; i++) {
		iov = &mrw_arg->wa_iov[i];
		iov_offset = 0;
		while (iov_offset < iov->iov_len) {
			blkbuf = iov->iov_base + iov_offset;
			iov_offset += MRW_FPC_BLOCK_SIZE;
			priv_p->p_fpc.fpc_op->fpc_calculate(&priv_p->p_fpc, blkbuf, MRW_FPC_BLOCK_SIZE, (unsigned char*)fpbuf);

			// Add new finger print node for return to read content cache.
			fp_nd = priv_p->p_fpn_p->fp_op->fp_get_node(priv_p->p_fpn_p);
			memcpy(fp_nd->fp, fpbuf, priv_p->p_fp_size);
			list_add_tail(&fp_nd->fp_list, &fpw->fn_arg_upstair->ag_fp_head);
			fpbuf += priv_p->p_fp_size;
		}
	}
}

static void
mrw_pwritev_async_fp(struct mrw_interface *mrw_p, void *rw_handle, struct iovec *iov,
	int iovcnt, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{
	struct mrw_private *priv_p = mrw_p->rw_private;
	struct mrw_worker *wp = (struct mrw_worker*)rw_handle;
	struct mrw_wn *mrw_arg;
	struct mrw_wsn *fpw_node;
	size_t j, split_num, fp_num, fp_num_arg, vconsumed, totalconsumed, more = 0, iov_len_j, fp_numj;
	int i;
	int arg_iov_index;
	struct iovec *iov_buf;
	off_t consumed_offset = offset;
	char prepare_flushing;

	INIT_LIST_HEAD(&arg->ag_fp_head);
	fpw_node = (struct mrw_wsn *) priv_p->p_mrw_wsn.wsn_op->wsn_get_node(&priv_p->p_mrw_wsn);
	fpw_node->fn_arg_upstair = arg;
	fpw_node->fn_callback_upstair = rw_callback;
	fpw_node->fn_priv_p = priv_p;
	gettimeofday(&fpw_node->fn_recv_time, NULL);

	// Calculate left bytes first.
	for (i=0; i<iovcnt; i++) {
		more += iov[i].iov_len;
	}
	nid_log_debug("WRITE:%p mrw_pwritev_async_fp: offset:%ld iov-count:%d, fp_num_arg:%lu size:%lu, priv_p->p_sent_wop_num:%d, priv_p->p_sent_wfp_num:%d",
			fpw_node, offset, iovcnt, more/4096, more, priv_p->p_sent_wop_num, priv_p->p_sent_wfp_num);
	// Set total fp number for this write.
	fpw_node->fn_not_done_fp_num = more >> MRW_FPC_BLOCK_SHIFT;
	__sync_add_and_fetch(&priv_p->p_sent_wfp_num, fpw_node->fn_not_done_fp_num);
	totalconsumed = 0;
	mrw_arg =  (struct mrw_wn *)priv_p->p_mrw_wn.wn_op->wn_get_node(&priv_p->p_mrw_wn);
	arg_iov_index = 0;
	mrw_arg->wa_fpw_node = fpw_node;
	mrw_arg->wa_offset = consumed_offset;
	gettimeofday(&mrw_arg->wa_recv_time, NULL);
	fp_num_arg = 0;
	mrw_arg->wa_iov_cnt = 0;
	for (i=0; i<iovcnt; i++) {
		// Calculate fp number for a iovec,
		// If the iovec len is more than max write size, we need split it to many iov.
		assert((iov[i].iov_len & (MRW_FPC_BLOCK_SIZE - 1)) == 0);
		fp_num = iov[i].iov_len >> MRW_FPC_BLOCK_SHIFT;
		split_num = MRW_DIV_UP(fp_num, MRW_AWRITE_BLOCKS_MAX);

		iov_buf = (struct iovec *)&iov[i];
		vconsumed = 0;

		for (j = 0; j < split_num; j++) {
			if (j == split_num - 1) {
				iov_len_j = iov_buf->iov_len - vconsumed;
				fp_numj = ((fp_num - 1) % MRW_AWRITE_BLOCKS_MAX) + 1;
			} else {
				iov_len_j = priv_p->p_max_write_size;
				fp_numj = MRW_AWRITE_BLOCKS_MAX;
			}

			// If a write blocks number is bigger than MRW_AWRITE_BLOCKS_MAX write
			// or mrw_arg iov is equal MRW_AWRITE_IOV_MAX,
			// Create new write arg.
			if (fp_num_arg + fp_numj > MRW_AWRITE_BLOCKS_MAX || arg_iov_index == MRW_AWRITE_IOV_MAX
			        || (totalconsumed + iov_len_j) > MRW_FLUSH_SIZE_MAX) {

				_mrw_calculate_fp_async(priv_p, mrw_arg, fp_num_arg);
				mrw_arg->wa_more = more;
				nid_log_debug("WRITE1:mrw_pwritev_async_fp: sub-offset:%ld sub-iov-count:%d, fp_num_arg:%lu size:%lu",
				        mrw_arg->wa_offset, mrw_arg->wa_iov_cnt, fp_num_arg, fp_num_arg*4096);
				_mrw_write_before(priv_p, &prepare_flushing);
				MS_Writev_Async(wp->w_volumn_uuid, 0, mrw_arg->wa_iov, mrw_arg->wa_offset, mrw_arg->wa_iov_cnt,
					(Callback)_mrw_write_callback, mrw_arg, mrw_arg->wa_fp_buf, 0, &mrw_arg->wa_more);
				_mrw_write_end(priv_p, prepare_flushing);
				mrw_arg = (struct mrw_wn *)
					priv_p->p_mrw_wn.wn_op->wn_get_node(&priv_p->p_mrw_wn);
				arg_iov_index = 0;
				mrw_arg->wa_fpw_node = fpw_node;
				mrw_arg->wa_offset = consumed_offset;
				mrw_arg->wa_iov_cnt = 0;
				gettimeofday(&mrw_arg->wa_recv_time, NULL);
				fp_num_arg = 0;
			}

			mrw_arg->wa_iov[arg_iov_index].iov_len = iov_len_j;
			mrw_arg->wa_iov[arg_iov_index].iov_base = iov_buf->iov_base + vconsumed;
			mrw_arg->wa_iov_cnt = ++arg_iov_index;

			fp_num_arg += fp_numj;
			vconsumed += iov_len_j;
			totalconsumed += iov_len_j;
			consumed_offset += iov_len_j;
			more -= iov_len_j;
		}
	}

	_mrw_calculate_fp_async(priv_p, mrw_arg, fp_num_arg);
	mrw_arg->wa_more = more;
	nid_log_debug("WRITE2:mrw_pwritev_async_fp: sub-offset:%ld sub-iov-count:%d, fp_num_arg:%lu size:%lu",
	                        mrw_arg->wa_offset, mrw_arg->wa_iov_cnt, fp_num_arg, fp_num_arg*4096);

	_mrw_write_before(priv_p, &prepare_flushing);
	MS_Writev_Async(wp->w_volumn_uuid, 0, mrw_arg->wa_iov, mrw_arg->wa_offset, mrw_arg->wa_iov_cnt,
		(Callback)_mrw_write_callback, mrw_arg, mrw_arg->wa_fp_buf, 0, &mrw_arg->wa_more);
	_mrw_write_end(priv_p, prepare_flushing);
	assert(more == 0);
}

static void
_mrw_read_callback(int err, void* arg)
{
	struct mrw_rn *mrw_rarg = arg;
	struct mrw_private *priv_p = mrw_rarg->rc_priv_p;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	struct fpn_interface *fpn_p = priv_p->p_fpn_p;

	if (err >= 0) {
		INIT_LIST_HEAD(&mrw_rarg->rc_arg_upstair->ag_fp_head);
		fpc_p->fpc_op->fpc_bcalculate(fpc_p, fpn_p, mrw_rarg->rc_buf, mrw_rarg->rc_count,
		    MRW_FPC_BLOCK_SIZE, &mrw_rarg->rc_arg_upstair->ag_fp_head);
	} else {
		nid_log_error("_mrw_read_callback: READ return error : %d.", err);
	}
	nid_log_debug("READ1: mrw_pread_async_fp callback: count:%lu err:%d", mrw_rarg->rc_count, err);
	gettimeofday(&mrw_rarg->rc_resp_time, NULL);
	if (time_difference(mrw_rarg->rc_recv_time, mrw_rarg->rc_resp_time) > TIME_CONSUME_ALERT) {
		nid_log_warning("_mrw_write_callback: A READ response delay %ldus, count:%lu, err:%d ",
				time_difference(mrw_rarg->rc_recv_time, mrw_rarg->rc_resp_time),
				mrw_rarg->rc_count, err);
	}

	mrw_rarg->rc_callback_upstair(err, mrw_rarg->rc_arg_upstair);
	mrw_rarg->rc_priv_p->p_mrw_rn.rn_op->rn_put_node(&mrw_rarg->rc_priv_p->p_mrw_rn, mrw_rarg);
}

static void
mrw_pread_async_fp(struct mrw_interface *mrw_p, void *rw_handle, void *buf, size_t count, off_t offset, rw_callback_fn rw_callback, struct rw_callback_arg *arg)
{
	struct mrw_private *priv_p = (struct mrw_private *)mrw_p->rw_private;
	struct mrw_worker *wp = (struct mrw_worker *)rw_handle;
	struct mrw_rn *mrw_rarg;

	nid_log_debug("READ0: mrw_pread_async_fp: offset:%ld count:%lu", offset, count);
	INIT_LIST_HEAD(&arg->ag_fp_head);
	mrw_rarg = (struct mrw_rn *)
    		priv_p->p_mrw_rn.rn_op->rn_get_node(&priv_p->p_mrw_rn);
	mrw_rarg->rc_arg_upstair = arg;
	mrw_rarg->rc_callback_upstair = rw_callback;
	mrw_rarg->rc_buf = buf;
	mrw_rarg->rc_count = count;
	mrw_rarg->rc_priv_p = priv_p;
	gettimeofday(&mrw_rarg->rc_recv_time, NULL);
	MS_Read_Async(wp->w_volumn_uuid, wp->w_snap_uuid, buf, offset, count,
		(Callback)_mrw_read_callback, (void *)mrw_rarg, 0);
}

static void
mrw_fetch_fp(struct mrw_interface *mrw_p, void *rw_handle, off_t voff,
                size_t len, rw_callback2_fn func, void *arg, void *fpout, bool *fpFound)
{
	(void)mrw_p;
	struct mrw_worker *wp = (struct mrw_worker *)rw_handle;
	MS_Read_Fp_Async(wp->w_volumn_uuid, wp->w_snap_uuid, voff, len, (Callback)func, arg, fpout, fpFound, 0);
}

static void
mrw_fetch_data(struct iovec* vec, size_t count, void *fp,
		rw_callback2_fn func, void *arg)
{
	MS_Read_Data_Async(vec, count, fp, (Callback)func, arg, 0);
}

static void
mrw_ms_flush_callback(MsRet ret, void *arg)
{
	struct mrw_interface *mrw_p = arg;
	struct mrw_private *priv_p = (struct mrw_private *)mrw_p->rw_private;

	if (ret != retOk) {
		nid_log_error("MS Flush got error message: %d.", ret);
	}
	assert(priv_p->p_sent_wop_num == 0);
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_fast_flush = false;
	__sync_lock_test_and_set(&priv_p->p_prepare_fast_flush, false);
	pthread_cond_broadcast(&priv_p->p_wcond);
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
mrw_ms_flush(struct mrw_interface *mrw_p)
{
	struct mrw_private *priv_p = (struct mrw_private *)mrw_p->rw_private;
	if (priv_p->p_max_write_op_limit == false) {
		pthread_mutex_lock(&priv_p->p_lck);
		while (priv_p->p_prepare_fast_flush) {
			/*Not allow two flush operation flush at the same time.*/
			pthread_cond_wait(&priv_p->p_wcond, &priv_p->p_lck);
		}
		pthread_mutex_unlock(&priv_p->p_lck);
		__sync_lock_test_and_set(&priv_p->p_prepare_fast_flush, true);
		// USleep, up this thread, so that make new write thread can process writing lock model.
		usleep(10);
	}

	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_fast_flush = true;
	while (priv_p->p_writing) {
		pthread_cond_wait(&priv_p->p_fcond, &priv_p->p_lck);
	}
	assert(priv_p->p_writing == 0);
	if (priv_p->p_sent_wop_num == 0) {
		/*All data have been flushed, do need flush anymore*/
		priv_p->p_fast_flush = false;
		__sync_lock_test_and_set(&priv_p->p_prepare_fast_flush, false);
		pthread_mutex_unlock(&priv_p->p_lck);
		return;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	nid_log_warning("Flush MDS. Waiting call back write operation number:%d", priv_p->p_sent_wop_num);
	MS_Flush(mrw_ms_flush_callback, mrw_p);
	pthread_mutex_lock(&priv_p->p_lck);
	while (priv_p->p_fast_flush) {
		pthread_cond_wait(&priv_p->p_wcond, &priv_p->p_lck);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

}

struct mrw_operations mrw_op = {
	.rw_create_worker = mrw_create_worker,
	.rw_pread = mrw_pread,
	.rw_pwrite = mrw_pwrite,
	.rw_pwritev = mrw_pwritev,

	.rw_pread_fp = mrw_pread_fp,
	.rw_pwrite_fp = mrw_pwrite_fp,
	.rw_pwritev_fp = mrw_pwritev_fp,

	.rw_pwrite_async_fp = mrw_pwrite_async_fp,
	.rw_pwritev_async_fp = mrw_pwritev_async_fp,
	.rw_pread_async_fp = mrw_pread_async_fp,

	.rw_fetch_fp = mrw_fetch_fp,
	.rw_fetch_data = mrw_fetch_data,

	.rw_close = mrw_close,
	.rw_stop = mrw_stop,
	.rw_ms_flush = mrw_ms_flush,

	.rw_get_status	= mrw_get_status,
};

#else

static void *
mrw_create_worker(struct mrw_interface *mrw_p, char *volumeid, int dev_size)
{
	char *log_header = "mrw_create_worker";
	(void)mrw_p; (void) dev_size;
	nid_log_error("%s: Can't create mrw without METASERVER feature support, volume_id: %d\n", log_header, atoi(volumeid));
	assert(0);
	return NULL;
}

struct mrw_operations mrw_op = {
      .rw_create_worker 	= mrw_create_worker,
      .rw_get_status		= mrw_get_status,
};

#endif

int
mrw_initialization(struct mrw_interface *mrw_p, struct mrw_setup *setup)
{
	char *log_header = "mrw_initialization";
	struct mrw_private *priv_p;
	struct mrw_wn_setup wn_setup;
	struct mrw_wsn_setup wsn_setup;
	struct mrw_rn_setup rn_setup;
	struct fpc_setup fpc_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	mrw_p->rw_op = &mrw_op;
	priv_p = calloc(1, sizeof(*priv_p));
	mrw_p->rw_private = priv_p;
	priv_p->p_fpn_p = setup->fpn_p;
	priv_p->p_max_write_size = MRW_AWRITE_BLOCKS_MAX * MRW_FPC_BLOCK_SIZE;
	priv_p->p_fp_size = priv_p->p_fpn_p->fp_op->fp_get_fp_size(priv_p->p_fpn_p);
	priv_p->p_sent_wfp_num = 0;
	priv_p->p_sent_wop_num = 0;
	priv_p->p_writing = 0;
	priv_p->p_fast_flush = false;
	priv_p->p_max_write_op_limit = false;
	priv_p->p_prepare_fast_flush = false;
	INIT_LIST_HEAD(&priv_p->p_worker_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	pthread_cond_init(&priv_p->p_wcond, NULL);
	pthread_cond_init(&priv_p->p_fcond, NULL);

	// TODO Replace this with pp2 get
	wn_setup.seg_size = 512;
	wn_setup.allocator = setup->allocator;
	mrw_wn_initialization(&priv_p->p_mrw_wn, &wn_setup);

	wsn_setup.seg_size = 512;
	wsn_setup.allocator = setup->allocator;
	mrw_wsn_initialization(&priv_p->p_mrw_wsn, &wsn_setup);

	rn_setup.seg_size = 512;
	rn_setup.allocator = setup->allocator;
	mrw_rn_initialization(&priv_p->p_mrw_rn, &rn_setup);

	fpc_setup.fpc_algrm = FPC_SHA224;
	fpc_initialization(&priv_p->p_fpc, &fpc_setup);

#ifdef METASERVER
	Ms_Start();
#endif
	return 0;
}
