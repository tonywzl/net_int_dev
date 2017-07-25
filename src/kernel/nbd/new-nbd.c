/*
 * Network block device - make block devices work over TCP
 *
 * Note that you can not swap over this thing, yet. Seems to work but
 * deadlocks sometimes - you can not swap over TCP in general.
 * 
 * Copyright 1997-2000, 2008 Pavel Machek <pavel@ucw.cz>
 * Parts copyright 2001 Steven Whitehouse <steve@chygwyn.com>
 *
 * This file is released under GPLv2 or later.
 *
 * (part of code stolen from loop.c)
 */

#include <linux/major.h>

#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <linux/net.h>
#include <linux/kthread.h>

#include <asm/uaccess.h>
#include <asm/types.h>

#include "nbd.h"
#include "nid.h"

#define NBD_MAGIC 0x68797548
#define NBD_VERSION "this-is-atlas-new-nbd-driver-for-build65"

#ifdef NDEBUG
#define dprintk(flags, fmt...)
#else /* NDEBUG */
#define dprintk(flags, fmt...) do { \
	if (debugflags & (flags)) printk(KERN_DEBUG fmt); \
} while (0)
#define DBG_IOCTL       0x0004
#define DBG_INIT        0x0010
#define DBG_EXIT        0x0020
#define DBG_BLKDEV      0x0100
#define DBG_RX          0x0200
#define DBG_TX          0x0400
static unsigned int debugflags = DBG_IOCTL | DBG_INIT | DBG_EXIT;
#endif /* NDEBUG */

#define NBD_REQUEST_WAIT	1*HZ
#define NBD_ACK_WAIT		1*HZ
#define NBD_CONTROL_WAIT	1*HZ
#define NBD_KEEPALIVE_TIME	15*HZ

#define	NBD_SEND_WAIT_SEC	30
#define NBD_RECV_WAIT_SEC	30

/* Prototypes */
static void nbd_remove(struct nbd_device *nbd);
static void nbd_async_reg(struct work_struct *work);

/* Globals */
static unsigned int nbds_max = 256;
static struct nbd_device *nbd_dev;
static int max_reqs = 1024;
static int max_part = 15;
static int part_shift;
static int dyn_dev = 1;
static int max_ack = 16;
static int send_wait = NBD_SEND_WAIT_SEC;
static int recv_wait = NBD_RECV_WAIT_SEC;

/*
 * Use just one lock (or at most 1 per NIC). Two arguments for this:
 * 1. Each NIC is essentially a synchronization point for all servers
 *    accessed through that NIC so there's no need to have more locks
 *    than NICs anyway.
 * 2. More locks lead to more "Dirty cache line bouncing" which will slow
 *    down each lock to the point where they're actually slower than just
 *    a single lock.
 * Thanks go to Jens Axboe and Al Viro for their LKML emails explaining this!
 */
static DEFINE_SPINLOCK(nbd_lock);

#ifndef NDEBUG
static const char *ioctl_cmd_to_ascii(int cmd)
{
	switch (cmd) {
	case NBD_SET_SOCK: return "set-sock";
	case NBD_SET_BLKSIZE: return "set-blksize";
	case NBD_SET_SIZE: return "set-size";
	case NBD_SET_TIMEOUT: return "set-timeout";
	case NBD_SET_FLAGS: return "set-flags";
	case NBD_DO_IT: return "do-it";
	case NBD_CLEAR_SOCK: return "clear-sock";
	case NBD_CLEAR_QUE: return "clear-que";
	case NBD_PRINT_DEBUG: return "print-debug";
	case NBD_SET_SIZE_BLOCKS: return "set-size-blocks";
	case NBD_DISCONNECT: return "disconnect";
	case BLKROSET: return "set-read-only";
	case BLKFLSBUF: return "flush-buffer-cache";
	}
	return "unknown";
}

static const char *nbdcmd_to_ascii(int cmd)
{
	switch (cmd) {
	case NID_REQ_READ: return "read";
	case NID_REQ_WRITE: return "write";
	case NID_REQ_DISC: return "disconnect";
	case NID_REQ_FLUSH: return "flush";
	case NID_REQ_TRIM: return "trim/discard";
	}
	return "invalid";
}
#endif /* NDEBUG */

static void nbd_end_request(struct request *req)
{
	int error = req->errors ? -EIO : 0;
	struct request_queue *q = req->q;
	unsigned long flags;
	struct nbd_device *nbd;

	nbd = (struct nbd_device *)req->rq_disk->private_data;
	dprintk(DBG_BLKDEV, "%s: request %p: %s\n", req->rq_disk->disk_name,
			req, error ? "failed" : "done");

	spin_lock_irqsave(q->queue_lock, flags);
	__blk_end_request_all(req, error);
	nbd->nr_req--;
	spin_unlock_irqrestore(q->queue_lock, flags);
}

static void sock_shutdown(struct nbd_device *nbd, int lock)
{
	/* Forcibly shutdown the socket causing all listeners
	 * to error
	 *
	 * FIXME: This code is duplicated from sys_shutdown, but
	 * there should be a more generic interface rather than
	 * calling socket ops directly here */

	struct file *file;
	if (lock)
		mutex_lock(&nbd->tx_lock);
	if (nbd->sock) {
		dev_warn(disk_to_dev(nbd->disk), "shutting down socket\n");
		kernel_sock_shutdown(nbd->sock, SHUT_RDWR);
		nbd->sock = NULL;
	}
	if (nbd->data_sock) {
		dev_warn(disk_to_dev(nbd->disk), "shutting down data socket\n");
		kernel_sock_shutdown(nbd->data_sock, SHUT_RDWR);
		nbd->data_sock = NULL;
	}

	file = nbd->file;
	nbd->file = NULL;
	if (file)
		fput(file);
	file = nbd->data_file;
	nbd->data_file = NULL;
	if (file)
		fput(file);
	nbd->last_to_send = 0;
	nbd->last_sent = 0;
	nbd->last_to_receive = 0;
	nbd->last_received = 0;
	nbd->last_keepalive = 0;
	nbd->last_got_keepalive = 0;

	if (lock)
		mutex_unlock(&nbd->tx_lock);
}

static void nbd_xmit_timeout(unsigned long arg)
{
	struct task_struct *task = (struct task_struct *)arg;

	dprintk(DBG_IOCTL, "nbd: killing hung xmit (%s, pid: %d)\n",
		task->comm, task->pid);
	force_sig(SIGKILL, task);
}

/*
 *  Send or receive packet.
 */
static int sock_xmit(struct nbd_device *nbd, int req, int send, void *buf, int size,
		int msg_flags)
{
	struct socket *sock = req ? nbd->sock : nbd->data_sock;
	int result;
	struct msghdr msg;
	struct kvec iov;
	sigset_t blocked, oldset;
	unsigned long pflags = current->flags;

	if (unlikely(!sock)) {
		dev_err(disk_to_dev(nbd->disk),
			"Attempted %s on closed socket in sock_xmit\n",
			(send ? "send" : "recv"));
		return -EINVAL;
	}

	/* Allow interception of SIGKILL only
	 * Don't allow other signals to interrupt the transmission */
	siginitsetinv(&blocked, sigmask(SIGKILL));
	sigprocmask(SIG_SETMASK, &blocked, &oldset);

	current->flags |= PF_MEMALLOC;
	do {
		sock->sk->sk_allocation = GFP_NOIO | __GFP_MEMALLOC;
		iov.iov_base = buf;
		iov.iov_len = size;
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = msg_flags | MSG_NOSIGNAL;

		if (send) {
			struct timer_list ti;

			if (nbd->xmit_timeout) {
				init_timer(&ti);
				ti.function = nbd_xmit_timeout;
				ti.data = (unsigned long)current;
				ti.expires = jiffies + nbd->xmit_timeout;
				add_timer(&ti);
			}
			nbd->last_to_send = jiffies;
			result = kernel_sendmsg(sock, &msg, &iov, 1, size);
			nbd->last_sent = jiffies;
			if (nbd->xmit_timeout)
				del_timer_sync(&ti);
		} else {
			nbd->last_to_receive = jiffies;
			result = kernel_recvmsg(sock, &msg, &iov, 1, size,
						msg.msg_flags);
			nbd->last_received = jiffies;
		}

		if (signal_pending(current)) {
			siginfo_t info;
			dprintk(DBG_IOCTL, "nbd (pid %d: %s) got signal %d\n",
				task_pid_nr(current), current->comm,
				dequeue_signal_lock(current, &current->blocked, &info));
			result = -EINTR;
			sock_shutdown(nbd, !send);
			break;
		}

		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}
		size -= result;
		buf += result;
	} while (size > 0);

	sigprocmask(SIG_SETMASK, &oldset, NULL);
	tsk_restore_flags(current, pflags, PF_MEMALLOC);

	return result;
}

static inline int sock_send_bvec(struct nbd_device *nbd, struct bio_vec *bvec,
		int flags)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	result = sock_xmit(nbd, 0, 1, kaddr + bvec->bv_offset,
			   bvec->bv_len, flags);
	kunmap(bvec->bv_page);
	return result;
}

/* always call with the tx_lock held */
static int nbd_fill_req_header(struct nbd_device *nbd, struct request *req, struct nid_request *request)
{
	u32 size = blk_rq_bytes(req);

	if (req->cmd_type != REQ_TYPE_FS) {
		return -EINVAL;
	}

	nbd_cmd(req) = NID_REQ_READ;
	if (rq_data_dir(req) == WRITE) {
		if ((req->cmd_flags & REQ_DISCARD)) {
			WARN_ON(!(nbd->flags & NBD_FLAG_SEND_TRIM));
			nbd_cmd(req) = NID_REQ_TRIM;
		} else
			nbd_cmd(req) = NID_REQ_WRITE;
		if (nbd->flags & NBD_FLAG_READ_ONLY) {
			dev_err(disk_to_dev(nbd->disk),
				"Write on read-only\n");
			return -EINVAL;
		}
	}

	if (req->cmd_flags & REQ_FLUSH) {
		BUG_ON(unlikely(blk_rq_sectors(req)));
		nbd_cmd(req) = NID_REQ_FLUSH;
	}

	req->errors = 0;

	request->magic = htons(NBD_REQUEST_MAGIC);
	request->cmd = nbd_cmd(req);

	if (nbd_cmd(req) == NID_REQ_FLUSH) {
		/* Other values are reserved for FLUSH requests.  */
		request->len = 0;
		dprintk(DBG_IOCTL, "nbd_fill_req_header: not support NID_REQ_FLUSH yet");
	} else {
		request->offset = cpu_to_be64((u64)blk_rq_pos(req) << 9);
		request->len = htonl(size);
		if (nbd_cmd(req) == NID_REQ_WRITE) {
			request->dseq = cpu_to_be64(nbd->sequence);
			nbd->sequence += size; 
		}
	}
	memcpy(request->handle, &req, sizeof(req));

	dprintk(DBG_TX, "%s: request %p: preparing control (%s@%llu,%uB) %llu\n",
			nbd->disk->disk_name, req,
			nbdcmd_to_ascii(nbd_cmd(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req),
			be64_to_cpu(request->dseq));
	return 0;
}

/* always call with the tx_lock held */
static int nbd_send_req_data(struct nbd_device *nbd, struct nid_request *ireq)
{
	int result, flags;
	int dsize = 0;
	u32 size = 0;
	struct request *req = *(struct request **)(ireq->handle);

	nbd->active_req = req;
	/* Can not trust nbd_cmd(ireq->handle), because read request could
	 * already been finished immediatelly after it's header send out.
	 */
	if (ireq->cmd == NID_REQ_WRITE) {
		struct req_iterator iter;
		struct bio_vec *bvec;
		size = blk_rq_bytes(req);

		/*
		 * we are really probing at internals to determine
		 * whether to set MSG_MORE or not...
		 */
		rq_for_each_segment(bvec, req, iter) {
			flags = 0;
			if (!rq_iter_last(req, iter))
				flags = MSG_MORE;
			dprintk(DBG_TX, "%s: request %p: sending %d bytes data\n",
					nbd->disk->disk_name, req, bvec->bv_len);
			dsize += bvec->bv_len;
			result = sock_send_bvec(nbd, bvec, flags);
			if (result <= 0) {
				dev_err(disk_to_dev(nbd->disk),
					"Send data failed (result %d)\n",
					result);
				goto error_out;
			}
			if (dsize >= size)
				break;
		}
		if (dsize != size) {
			dprintk(DBG_IOCTL, "dsize: %d, size: %d\n", dsize, size);
			BUG_ON(dsize != size);
		}
	} else if (ireq->cmd == NBD_CMD_CONTROL) {
		struct nbd_cntl_request *cntl_req = (struct nbd_cntl_request *)req;	
		int len = cntl_req->cntl_info.len;
		cntl_req->cntl_info.len = htonl(len);
		result = sock_xmit(nbd, 1, 1, &cntl_req->cntl_info, sizeof(cntl_req->cntl_info)+len, 0);
		dprintk(DBG_IOCTL, "nbd_send_req_data: sent out a NBD_CMD_CONTROL\n");
		if (result <= 0) {
			dev_err(disk_to_dev(nbd->disk),
				"Send data failed (result %d)\n",
				result);
			goto error_out;
		}
	}

	nbd->active_req = NULL;
	wake_up_all(&nbd->active_wq);
	return size;

error_out:
	nbd->active_req = NULL;
	wake_up_all(&nbd->active_wq);
	return -EIO;
}

/* always call with the tx_lock held */
static int nbd_send_req(struct nbd_device *nbd, struct request *req)
{
	int result;
	struct nid_request request;
	u32 size = blk_rq_bytes(req);

	request.magic = htons(NBD_REQUEST_MAGIC);
	request.cmd = nbd_cmd(req);

	if (nbd_cmd(req) == NID_REQ_FLUSH) {
		/* Other values are reserved for FLUSH requests.  */
		request.len = 0;
		dprintk(DBG_IOCTL, "nbd_send_req: not support NID_REQ_FLUSH yet");
	} else {
		request.offset = cpu_to_be64((u64)blk_rq_pos(req) << 9);
		request.len = htonl(size);
		if (nbd_cmd(req) == NID_REQ_WRITE) {
			// dprintk(DBG_IOCTL, "nbd_send_req: NID_REQ_WRITE sequence:%lu\n", nbd->sequence);
			request.dseq = cpu_to_be64(nbd->sequence);
		}
	}
	memcpy(request.handle, &req, sizeof(req));

	dprintk(DBG_TX, "%s: request %p: sending control (%s@%llu,%uB)\n",
			nbd->disk->disk_name, req,
			nbdcmd_to_ascii(nbd_cmd(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req));

	result = sock_xmit(nbd, 1, 1, &request, sizeof(request), 0);
	if (result <= 0) {
		dev_err(disk_to_dev(nbd->disk),
			"Send control failed (result %d)\n", result);
		goto error_out;
	}

	result = nbd_send_req_data(nbd, &request);
	if (result < 0) {
		goto error_out;
	}

	return 0;

error_out:
	return -EIO;
}

static struct request *nbd_find_request(struct nbd_device *nbd,
					struct request *xreq)
{
	struct request *req, *tmp;
	int err;

	// err = wait_event_interruptible(nbd->active_wq, nbd->active_req != xreq);
	err = 0;
	while (nbd->active_req == xreq) {
		err = wait_event_interruptible(nbd->active_wq, nbd->active_req != xreq);
	}
	if (unlikely(err)) {
		dprintk(DBG_IOCTL, "%s: nbd_find_request wait_event_interruptible err:%d\n",
			nbd->disk->disk_name, err);		
		goto out;
	}
	
	spin_lock(&nbd->queue_lock);
	list_for_each_entry_safe(req, tmp, &nbd->queue_head, queuelist) {
		if (req != xreq)
			continue;
		list_del_init(&req->queuelist);
		nbd->ack_len--;
		spin_unlock(&nbd->queue_lock);
		//wake_up(&nbd->ack_wq);

		return req;
	}

	dprintk(DBG_IOCTL, "%s:nbd_find_request failed to find request(%p) in queue_head, ack_len:%d, waiting_len:%d\n",
		nbd->disk->disk_name, xreq, nbd->ack_len, nbd->waiting_len);
	dprintk(DBG_IOCTL, "%s: queue_head contains:\n", nbd->disk->disk_name);
	list_for_each_entry_safe(req, tmp, &nbd->queue_head, queuelist) {
		dprintk(DBG_IOCTL, "%p\n", req);
	}
	dprintk(DBG_IOCTL, "%s: queue_head end\n", nbd->disk->disk_name);
	
	dprintk(DBG_IOCTL, "%s: waiting_queue contains:\n", nbd->disk->disk_name);
	list_for_each_entry_safe(req, tmp, &nbd->waiting_queue, queuelist) {
		dprintk(DBG_IOCTL, "%p\n", req);
	}
	dprintk(DBG_IOCTL, "%s: waiting_queue end\n", nbd->disk->disk_name);
	spin_unlock(&nbd->queue_lock);

	err = -ENOENT;

out:
	return ERR_PTR(err);
}

static inline int sock_recv_bvec(struct nbd_device *nbd, struct bio_vec *bvec)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	result = sock_xmit(nbd, 0, 0, kaddr + bvec->bv_offset, bvec->bv_len,
			MSG_WAITALL);
	kunmap(bvec->bv_page);
	return result;
}

/* NULL returned = something went wrong, inform userspace */
static struct request *nbd_read_stat(struct nbd_device *nbd)
{
	int result;
	struct nid_request reply;
	struct request *req;

next:
	result = sock_xmit(nbd, 1, 0, &reply, sizeof(reply), MSG_WAITALL);
	if (result <= 0) {
		dev_err(disk_to_dev(nbd->disk),
			"Receive control failed (result %d)\n", result);
		goto harderror;
	}

	if (reply.cmd == NID_REQ_KEEPALIVE) {
		nbd->last_got_keepalive = jiffies; 
		goto next;
	}

	req = nbd_find_request(nbd, *(struct request **)reply.handle);
	if (IS_ERR(req)) {
		result = PTR_ERR(req);
		if (result != -ENOENT)
			goto harderror;

		dev_err(disk_to_dev(nbd->disk), "Unexpected reply (%p)\n",
			reply.handle);
		// result = -EBADR;
		goto harderror;
	}

	if (ntohl(reply.code)) {
		dev_err(disk_to_dev(nbd->disk), "Other side returned error (%d)\n",
			reply.code);
		req->errors++;
		return req;
	}

	dprintk(DBG_RX, "%s: request %p: got reply\n",
			nbd->disk->disk_name, req);
	if (nbd_cmd(req) == NID_REQ_READ) {
		struct req_iterator iter;
		struct bio_vec *bvec;

		rq_for_each_segment(bvec, req, iter) {
			result = sock_recv_bvec(nbd, bvec);
			if (result <= 0) {
				dev_err(disk_to_dev(nbd->disk), "Receive data failed (result %d)\n",
					result);
				if (nbd->flags & NBD_FLAG_PERSISTENT_HANG) {
					/* Don't give up. Rejoin this request to the sending list to retry. */
					spin_lock(&nbd->queue_lock);
					list_add(&req->queuelist, &nbd->waiting_queue);
					nbd->waiting_len++;
					spin_unlock(&nbd->queue_lock);
					result = -ENONET;
					goto harderror;
				}
				req->errors++;
				return req;
			}
			dprintk(DBG_RX, "%s: request %p: got %d bytes data\n",
				nbd->disk->disk_name, req, bvec->bv_len);
		}
	}
	return req;
harderror:
	nbd->harderror = result;
	return NULL;
}

static ssize_t pid_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct gendisk *disk = dev_to_disk(dev);

	return sprintf(buf, "%ld\n",
		(long) ((struct nbd_device *)disk->private_data)->pid);
}

static struct device_attribute pid_attr = {
	.attr = { .name = "pid", .mode = S_IRUGO},
	.show = pid_show,
};

static int nbd_do_it(struct nbd_device *nbd)
{
	struct request *req;
	int ret = 0;

	BUG_ON(nbd->magic != NBD_MAGIC);

	sk_set_memalloc(nbd->sock->sk);
	nbd->pid = task_pid_nr(current);
	if (!dyn_dev) {
		ret = device_create_file(disk_to_dev(nbd->disk), &pid_attr);
	}
	if (ret) {
		dev_err(disk_to_dev(nbd->disk), "device_create_file failed!\n");
		nbd->pid = 0;
		return ret;
	}

	while ((req = nbd_read_stat(nbd)) != NULL) {
		nbd_end_request(req);
	}

	nbd->pid = 0;
	return 0;
}

static void nbd_clear_que(struct nbd_device *nbd)
{
	struct request *req;

	BUG_ON(nbd->magic != NBD_MAGIC);

	/*
	 * Because we have set nbd->sock to NULL under the tx_lock, all
	 * modifications to the list must have completed by now.  For
	 * the same reason, the active_req must be NULL.
	 *
	 * As a consequence, we don't need to take the spin lock while
	 * purging the list here.
	 */
	BUG_ON(nbd->sock);
	BUG_ON(nbd->active_req);

	while (!list_empty(&nbd->queue_head)) {
		spin_lock(&nbd->queue_lock);
		req = list_entry(nbd->queue_head.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nbd->ack_len--;
		spin_unlock(&nbd->queue_lock);
		//wake_up(&nbd->ack_wq);
		req->errors++;
		nbd_end_request(req);
	}

	while (!list_empty(&nbd->waiting_queue)) {
		spin_lock(&nbd->queue_lock);
		req = list_entry(nbd->waiting_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nbd->waiting_len--;
		spin_unlock(&nbd->queue_lock);
		req->errors++;
		nbd_end_request(req);
	}
}


static void nbd_handle_req_list(struct nbd_device *nbd, struct list_head *working_queue, int nr_reqs)
{
	struct request *req, *next_req;
	int result;
	int i = 0, nr_send = 0;
	struct nid_request *ireq, *req_buf = nbd->req_buf;
	int hsize = 0, dsize = 0;

	list_for_each_entry_safe(req, next_req, working_queue, queuelist) {
		result = nbd_fill_req_header(nbd, req, &req_buf[nr_send]);
		if (result != 0) {
			/* Drop bad requests directly. */
			dprintk(DBG_IOCTL, "%s: drop bad requests %p.\n", nbd->disk->disk_name, req);
			list_del_init(&req->queuelist);
			req->errors++;
			nbd_end_request(req);
			continue;
		}
		if (nbd_cmd(req) == NID_REQ_WRITE) {
			hsize += blk_rq_bytes(req);
		}
		nr_send++;
	}

	if (nr_send == 0) {
		dprintk(DBG_IOCTL, "%s: zero good requests.\n", nbd->disk->disk_name);
		return;
	}

	/* Move requests to ack queue. */
	spin_lock(&nbd->queue_lock);
	list_splice_tail_init(working_queue, &nbd->queue_head);
	nbd->ack_len += nr_send;
	spin_unlock(&nbd->queue_lock);

	mutex_lock(&nbd->tx_lock);

	if (unlikely(!nbd->sock)) {
		mutex_unlock(&nbd->tx_lock);
		dev_err(disk_to_dev(nbd->disk),
			"Attempted send on closed socket, req equeued\n");
		goto error_out;
	}

	/* NOTICE: The 'request' data structure of a read command can be 
	 * finished and get reused immediatelly after it's header send out.
	 */
	result = sock_xmit(nbd, 1, 1, req_buf, sizeof(struct nid_request) * nr_send, 0);
	if (result <= 0) {
		mutex_unlock(&nbd->tx_lock);
		dev_err(disk_to_dev(nbd->disk),
			"Send control failed (result %d)\n", result);
		goto error_out;
	}

	for(i = 0; i < nr_send; i++) {
		ireq = req_buf + i;
		result = nbd_send_req_data(nbd, ireq);
		if (result < 0) {
			mutex_unlock(&nbd->tx_lock);
			goto error_out;
		}
		if (ireq->cmd == NID_REQ_WRITE) {
			dsize += result;
		}
	}
	if (hsize != dsize) {
		dprintk(DBG_IOCTL, "nr_send: %d, hsize: %d, dsize: %d\n", nr_send, hsize, dsize);
		BUG_ON(hsize != dsize);
	}

	mutex_unlock(&nbd->tx_lock);

	return;

error_out:
	/* TODO: Should close socket on error. */

	if (!(nbd->flags & NBD_FLAG_PERSISTENT_HANG)) {
		for(; i < nr_send; i++) { /* FIXME: Should we fail all reqs together? */
			req = *(struct request **)(req_buf[i].handle);

			/* TODO: Is it safe to directly remove the req? */
			spin_lock(&nbd->queue_lock);
			list_del_init(&req->queuelist);
			nbd->ack_len--;
			spin_unlock(&nbd->queue_lock);

			dev_err(disk_to_dev(nbd->disk), "Request send failed\n");
			req->errors++;
			nbd_end_request(req);
		}
	} else {
		dprintk(DBG_IOCTL, "nbd_handle_req_list: splice queue_head with waiting_queue\n");
		spin_lock(&nbd->queue_lock);
		list_splice_tail_init(&nbd->queue_head, nbd->waiting_queue.next);
		nbd->waiting_len += nbd->ack_len;
		nbd->ack_len = 0;
		spin_unlock(&nbd->queue_lock);
	}
	return;
}

static int nbd_thread(void *data)
{
	int nr_reqs;
	struct nbd_device *nbd = data;
	struct gendisk *disk = nbd->disk;
	struct request_queue *queue = disk->queue;
	LIST_HEAD(working_queue);

	dprintk(DBG_IOCTL, "%s: enter nbd_thread. rq_timeout:%u, nr_requests:%lu\n",
		disk->disk_name, queue->rq_timeout, queue->nr_requests);

	set_user_nice(current, -20);
	while (!kthread_should_stop()) {
		int rc = 0;

		/* wait for something to do, timeout in 10 secs */
		rc = wait_event_interruptible_timeout(nbd->waiting_wq,
			kthread_should_stop() ||
			(!list_empty(&nbd->waiting_queue) &&
			(!(nbd->flags & NBD_FLAG_PERSISTENT_HANG) || likely(nbd->sock))),
			NBD_REQUEST_WAIT);
	
#if 0	
		/*
		 * waiting for ack list decreasing enough
		 * but don't stuck here
		 */
		if (nbd->ack_len > max_ack) {
			//dprintk(DBG_IOCTL, "too many msgs waiting for ack, ack:%d, waiting:%d\n",
			//	nbd->ack_len, nbd->waiting_len);
			rc = wait_event_interruptible_timeout(nbd->ack_wq,
				kthread_should_stop() || nbd->ack_len <= max_ack,
				NBD_ACK_WAIT);
		}
		if (nbd->ack_len > max_ack) {
			dprintk(DBG_IOCTL, "%s: too many reqs waiting for ack even after wait, rc:%d, ack:%d, waiting:%d\n",
				disk->disk_name, rc, nbd->ack_len, nbd->waiting_len);
			continue;
		}
#endif

		/* extract request */
		if (list_empty(&nbd->waiting_queue))
			continue;

		spin_lock_irq(&nbd->queue_lock);
		/* Move all pending requests to the working queue. */
		list_splice_init(&nbd->waiting_queue, &working_queue);
		nr_reqs = nbd->waiting_len;
		nbd->waiting_len = 0;
		spin_unlock_irq(&nbd->queue_lock);

		nbd_handle_req_list(nbd, &working_queue, nr_reqs);
	}
	/* The nbd_clear_que() after kthread_stop() will cleanup any pending reqs. */
	return 0;
}

static int nbd_control_thread(void *data)
{
	struct nbd_device *nbd = data;
	struct gendisk *disk = nbd->disk;
	struct request_queue *queue = disk->queue;
	struct request keepalive_req;
	int should_shutdown;
	long int now;
	int rc;
	dprintk(DBG_IOCTL, "%s: enter control_thread. rq_timeout:%u, nr_requests:%lu\n",
		disk->disk_name, queue->rq_timeout, queue->nr_requests);
control_loop:
	should_shutdown = 0;
	rc = wait_event_interruptible_timeout(nbd->control_wq,
		kthread_should_stop(),
		NBD_CONTROL_WAIT);

	if (kthread_should_stop()) {
		dprintk(DBG_IOCTL, "%s: control_thread stop ...\n", disk->disk_name);
		goto out;
	}

	now = jiffies;
	if (likely(nbd->last_to_send) &&				/* already try to send some msg after sock connected */
		unlikely(now - nbd->last_to_send > send_wait * HZ) &&	/* try to send the last msg long time ago */
		unlikely(nbd->last_sent < nbd->last_to_send)) { 	/* still in sending a msg */
		/* stuck at sending a msg */
		should_shutdown = 1;
		dprintk(DBG_IOCTL, "%s: control_thread sock_shutdown due to sending stuck, last_sent:%ld, last_to_send:%ld, now:%ld\n",
			disk->disk_name, nbd->last_sent, nbd->last_to_send, now);
	} else if (likely(nbd->last_got_keepalive) &&
	           likely(nbd->last_got_keepalive + NBD_KEEPALIVE_TIME * 2 < now )) {
		   dprintk(DBG_IOCTL, "%s: control_thread sock_shutdown due to not got keepalive too long, last_got_keepalive:%ld, now:%ld\n",
			disk->disk_name, nbd->last_got_keepalive, now);
		   should_shutdown = 1;
	}
#if 0
	} else if (likely(nbd->last_to_receive) &&
		!list_empty(&nbd->queue_head) &&		/* we are waiting for response */
		 (now - nbd->last_sent > recv_wait * HZ) &&
		 (nbd->last_received < nbd->last_sent)) { 	/* last response received long time ago */
		/* stuck at receiving a msg */
		dprintk(DBG_IOCTL, "%s: control_thread sock_shutdown due to receiving stuck, last_sent:%ld, last_received:%ld, now:%ld\n",
			disk->disk_name, nbd->last_sent, nbd->last_received, now);
		should_shutdown = 1;
	}
#endif
	if (should_shutdown) {
		dprintk(DBG_IOCTL, "%s: control thread sock_shutdown\n", disk->disk_name);
		sock_shutdown(nbd, 1);
	} else if (nbd->last_keepalive + NBD_KEEPALIVE_TIME <= now) {
			//dprintk(DBG_IOCTL, "%s: control thread sending keepalive\n", disk->disk_name);
			blk_rq_init(NULL, &keepalive_req);
			keepalive_req.cmd_type = REQ_TYPE_SPECIAL;
			nbd_cmd(&keepalive_req) = NID_REQ_KEEPALIVE;
			mutex_lock(&nbd->tx_lock);
			nbd_send_req(nbd, &keepalive_req);
			mutex_unlock(&nbd->tx_lock);
			nbd->last_keepalive = now;
	}
	goto control_loop;

out:
	dprintk(DBG_IOCTL, "%s: control thread exiting ...\n", disk->disk_name);
	nbd->control_thread = NULL;
	return 0;
}
/*
 * We always wait for result of write, for now. It would be nice to make it optional
 * in future
 * if ((rq_data_dir(req) == WRITE) && (nbd->flags & NBD_WRITE_NOCHK))
 *   { printk( "Warning: Ignoring result!\n"); nbd_end_request( req ); }
 */

static void do_nbd_request(struct request_queue *q)
		__releases(q->queue_lock) __acquires(q->queue_lock)
{
	struct request *req;
	
	while ((req = blk_fetch_request(q)) != NULL) {
		struct nbd_device *nbd;

		nbd = (struct nbd_device *)req->rq_disk->private_data;
		nbd->nr_req++;
		spin_unlock_irq(q->queue_lock);

		dprintk(DBG_BLKDEV, "%s: request %p: dequeued (flags=%x)\n",
				req->rq_disk->disk_name, req, req->cmd_type);


		BUG_ON(nbd->magic != NBD_MAGIC);

		/*
		 * For persistent mode hang status, don't worry about sock issue here
		 */

		spin_lock_irq(&nbd->queue_lock);
		//if (unlikely(!nbd->sock && nbd->error_now)) { 
		if (unlikely(nbd->error_now)) { 
			spin_unlock_irq(&nbd->queue_lock);
			goto error;
		}
		list_add_tail(&req->queuelist, &nbd->waiting_queue);
		nbd->waiting_len++;
		spin_unlock_irq(&nbd->queue_lock);
		wake_up(&nbd->waiting_wq);
		spin_lock_irq(q->queue_lock);
		continue;

error:
		dev_err(disk_to_dev(nbd->disk),
			"Attempted send on closed socket\n");
		req->errors++;
		nbd_end_request(req);
		spin_lock_irq(q->queue_lock);
	}
}

/* Must be called with tx_lock held */

static int __nbd_ioctl(struct block_device *bdev, struct nbd_device *nbd,
		       unsigned int cmd, unsigned long arg)
{
	struct gendisk *disk = nbd->disk;
	dprintk(DBG_IOCTL, "%s: enter __nbd_ioctl.\n", disk->disk_name);
	/* ATLAS-COMMENT: TODO: Prevent multiple nbd-clients work on the same device. */
#if 0
	if (!nbd->disconnect) {
		if (cmd != NBD_CLEAR_QUE && cmd != NBD_DISCONNECT
			&& cmd != NBD_DO_IT) {
			return -EBUSY;
		}
	}
#endif
	switch (cmd) {
	case NBD_DISCONNECT: {
		struct request sreq;

		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_DISCONNECT.\n", disk->disk_name);
		dev_info(disk_to_dev(nbd->disk), "NBD_DISCONNECT\n");
		if (!nbd->sock) {
			nbd->disconnect = 1; // the caller can call nbd_do_it to cleanup the nbd later
			dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_DISCONNECT null sock\n", disk->disk_name);
			return -EINVAL;
		}

		mutex_unlock(&nbd->tx_lock);
		fsync_bdev(bdev);
		mutex_lock(&nbd->tx_lock);
		blk_rq_init(NULL, &sreq);
		sreq.cmd_type = REQ_TYPE_SPECIAL;
		nbd_cmd(&sreq) = NID_REQ_DISC;

		/* Check again after getting mutex back.  */
		if (!nbd->sock) {
			dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_DISCONNECT null sock 2\n", disk->disk_name);
			return -EINVAL;
		}

		nbd->disconnect = 1;
		nbd_send_req(nbd, &sreq);
		return 0;
	}

	case NBD_CLEAR_SOCK: {
		struct file *file;

		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_CLEAR_SOCK.\n", disk->disk_name);
		nbd->sock = NULL;
		nbd->data_sock = NULL;
		file = nbd->file;
		nbd->file = NULL;
		nbd->data_file = NULL;
		nbd_clear_que(nbd);
		BUG_ON(!list_empty(&nbd->queue_head));
		BUG_ON(!list_empty(&nbd->waiting_queue));
		kill_bdev(bdev);
		if (file)
			fput(file);
		return 0;
	}

	case NBD_SET_SOCK: {
		struct file *file;

		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_SET_SOCK.\n", disk->disk_name);
		if (nbd->file)
			return -EBUSY;

		file = fget(arg);
		if (file) {
			struct inode *inode = file_inode(file);
			if (S_ISSOCK(inode->i_mode)) {
				nbd->sequence = 0;
				nbd->file = file;
				nbd->sock = SOCKET_I(inode);
				if (max_part > 0)
					bdev->bd_invalidated = 1;
				nbd->disconnect = 0; /* we're connected now */
				return 0;
			} else {
				fput(file);
			}
		}
		return -EINVAL;
	}

	case NBD_SET_DATA_SOCK: {
		struct file *data_file;

		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_SET_DATA_SOCK.\n", disk->disk_name);
		if (nbd->data_file) {
			dprintk(DBG_IOCTL, "%s: NBD_SET_DATA_SOCK busy", disk->disk_name);
			return -EBUSY;
		}
		if (!nbd->file) {
			dprintk(DBG_IOCTL, "%s: NBD_SET_DATA_SOCK should be set after NBD_SET_SOCK", disk->disk_name); 
			return -EINVAL;
		}

		data_file = fget(arg);
		if (data_file) {
			struct inode *inode = file_inode(data_file);
			if (S_ISSOCK(inode->i_mode)) {
				nbd->data_file = data_file;
				nbd->data_sock = SOCKET_I(inode);
				if (max_part > 0)
					bdev->bd_invalidated = 1;
				return 0;
			} else {
				fput(data_file);
			}
		}
		return -EINVAL;
	}

	case NBD_SET_BLKSIZE:
		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_SET_BLKSIZE.\n", disk->disk_name);
		nbd->blksize = arg;
		nbd->bytesize &= ~(nbd->blksize-1);
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_SET_SIZE:
		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_SET_SIZE.\n", disk->disk_name);
		nbd->bytesize = arg & ~(nbd->blksize-1);
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_SET_TIMEOUT:
		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_SET_TIMEOUT.\n", disk->disk_name);
		nbd->xmit_timeout = arg * HZ;
		nbd->xmit_timeout = 5 * HZ;	// disable timeout
		return 0;

	case NBD_SET_FLAGS:
		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_SET_FLAGS. (persistent:%d, hang:%d)\n",
			disk->disk_name, arg&NBD_FLAG_PERSISTENT?1:0, arg&NBD_FLAG_PERSISTENT_HANG?1:0);
		nbd->flags = arg;
		if (nbd->flags & NBD_FLAG_PERSISTENT_HANG) {
			nbd->error_now = 0;
		} else {
			nbd->error_now = 0; //Only support NBD_FLAG_PERSISTENT_HANG mode.
		}
		return 0;

	case NBD_SET_SIZE_BLOCKS:
		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_SET_SIZE_BLOCKS.\n", disk->disk_name);
		nbd->bytesize = ((u64) arg) * nbd->blksize;
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_DO_IT: {
		struct file *file;
		int error = 0;
		int need_exit = 0;

		dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_DO_IT. disconnect:%d, inited:%d\n",
			disk->disk_name, nbd->disconnect, nbd->inited);
		if(nbd->disconnect && !nbd->inited) {
			return 0;
		}
		if (!nbd->disconnect) {
		if (nbd->pid)
			return -EBUSY;
		if (!nbd->file)
			return -EINVAL;
		}
		mutex_unlock(&nbd->tx_lock);

		if (!nbd->inited) {
			/*
			 * Only do these setup at the first time
			 */
			if (nbd->flags & NBD_FLAG_READ_ONLY)
				set_device_ro(bdev, true);
			if (nbd->flags & NBD_FLAG_SEND_TRIM)
				queue_flag_set_unlocked(QUEUE_FLAG_DISCARD,
					nbd->disk->queue);
			if (nbd->flags & NBD_FLAG_SEND_FLUSH)
				blk_queue_flush(nbd->disk->queue, REQ_FLUSH);
			else
				blk_queue_flush(nbd->disk->queue, 0);

			nbd->thread = kthread_create(nbd_thread, nbd, "%s",
						nbd->disk->disk_name);
			if (IS_ERR(nbd->thread)) {
				struct task_struct *thread = nbd->thread;
				dprintk(DBG_IOCTL, "%s: __nbd_ioctl: cannot create nbd_thread\n",
					nbd->disk->disk_name);
				mutex_lock(&nbd->tx_lock);
				nbd->thread = NULL;
				return PTR_ERR(thread);
			}
			wake_up_process(nbd->thread);
			nbd->control_thread = kthread_create(nbd_control_thread, nbd, "%s",
						nbd->disk->disk_name);
			if (IS_ERR(nbd->control_thread)) {
				struct task_struct *thread = nbd->control_thread;
				dprintk(DBG_IOCTL, "%s: __nbd_ioctl: cannot create control_thread\n",
					nbd->disk->disk_name);
				mutex_lock(&nbd->tx_lock);
				nbd->control_thread = NULL;
				return PTR_ERR(thread);
			}
			wake_up_process(nbd->control_thread);

			nbd->reg_done = 0;
			nbd->cntl_req_done = 0;
			nbd->cntl_req = NULL;
			nbd->nr_req = 0;
			if (dyn_dev) {
				INIT_WORK(&nbd->work, nbd_async_reg);
				schedule_work(&nbd->work);
			}
			nbd->inited = 1;
		} else {
			/* this must be persistent mode, there could be some requests waiting to retry */	
			if (nbd->flags & NBD_FLAG_PERSISTENT_HANG) {
				dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_DO_IT: re-hang\n", disk->disk_name);
				nbd->error_now = 0;
			}
			wake_up(&nbd->waiting_wq);
		}

		if (!nbd->disconnect) {
			dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. start nbd_do_it\n");
			error = nbd_do_it(nbd);
		} else {
			dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. don't start nbd_do_it\n");
		}
		dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. after nbd_do_it\n");
		
		if (!nbd->reg_done) {
			nbd->harderror = -EBADR;
			dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. nbd_async_reg not finished yet even after nbd_do_it\n");
		}

		if (nbd->disconnect ||
		    !(nbd->flags & NBD_FLAG_PERSISTENT) ||
		    nbd->harderror == -EBADR) {
			need_exit = 1;
		}
		if (need_exit) {
			if (nbd->thread) {
				kthread_stop(nbd->thread);
				nbd->thread = NULL;
			}
			if (nbd->control_thread) {
				kthread_stop(nbd->control_thread);
				nbd->control_thread = NULL;
			}
		}

		dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. to hold mutex_lock, need_exit:%d\n", need_exit);
		mutex_lock(&nbd->tx_lock);
		dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. got mutex_lock, need_exit:%d\n", need_exit);
		if (need_exit) {
			nbd->inited = 0;
		} 
		if (error) {
			/* TODO need cleanup! */
			dprintk(DBG_IOCTL, "nbd_do_it: error: %d!\n", error);
			return error;
		}

		/*
		 * Just do NBD_CLEAR_SOCK's job here.
		 * nbd-client should not call NBD_CLEAR_SOCK ioctl again.
		 */
		dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. to sock_shutdown\n");
		sock_shutdown(nbd, 0);
		file = nbd->file;
		nbd->file = NULL;
		if (file)
			fput(file);
		file = nbd->data_file;
		nbd->data_file = NULL;
		if (file)
			fput(file);
		dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. after sock_shutdown\n");
		if (!need_exit) {
			if (nbd->flags & NBD_FLAG_PERSISTENT_HANG) {
				if (!list_empty(&nbd->queue_head)) {
					/*
					 * splice the waiting ack queue (queue_head) to waiting send queue to resend 
					 * these requests to avoid I/O errors since we are in persistent mode
					 */
					dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. splice queue_head with waiting_queue\n");
					spin_lock(&nbd->queue_lock);
					list_splice(&nbd->queue_head, nbd->waiting_queue.next);
					nbd->waiting_len += nbd->ack_len;
					nbd->ack_len = 0;
					INIT_LIST_HEAD(&nbd->queue_head);
					spin_unlock(&nbd->queue_lock);
				}
			} else if (nbd->flags & NBD_FLAG_PERSISTENT) {
				dprintk(DBG_IOCTL, "__nbd_ioctl: NBD_FLAG_PERSISTENT -- nbd_clear_que\n");
				nbd_clear_que(nbd);
			}
		}

		dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. here 1, need_exit:%d\n", need_exit);
		if (need_exit) {
			spin_lock(&nbd->queue_lock);
			nbd->error_now = 1;
			spin_unlock(&nbd->queue_lock);
			nbd_clear_que(nbd);
			dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. here 1.1 %s", nbd->disk->disk_name);
			//dev_warn(disk_to_dev(nbd->disk), "queue cleared\n");
			dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. here 1.2");
			kill_bdev(bdev);
			dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. here 1.3");
			queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, nbd->disk->queue);
			set_device_ro(bdev, false);
			nbd->flags = 0;
			nbd->bytesize = 0;
			bdev->bd_inode->i_size = 0;
			set_capacity(nbd->disk, 0);

			if (dyn_dev) {
				dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. calling cancel_work_sync\n");
				cancel_work_sync(&nbd->work);	// in case the job not finished yet
				dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. after cancel_work_sync\n");
				nbd_remove(nbd);
			}

			if (max_part > 0)
				ioctl_by_bdev(bdev, BLKRRPART, 0);
		}
		dprintk(DBG_IOCTL, "__nbd_ioctl: case NBD_DO_IT. here 2\n");
		error = nbd->harderror;
		if (nbd->disconnect) {/* user requested, ignore socket errors */
			dprintk(DBG_IOCTL, "nbd_do_it: Detect user requested disconnect.\n");
			error = 0;
		}
		if (!(nbd->flags & NBD_FLAG_PERSISTENT)) {
			nbd->disconnect = 1;
		}
		dprintk(DBG_IOCTL, "nbd_do_it: error code: %d.\n", error);
		return error;
	}

	case NBD_CLEAR_QUE:
		/*
		 * This is for compatibility only.  The queue is always cleared
		 * by NBD_DO_IT or NBD_CLEAR_SOCK.
		 */
		//BUG_ON(!nbd->sock && !list_empty(&nbd->queue_head));

		/* Return error on any new requests. */
		if (nbd->flags & NBD_FLAG_PERSISTENT_HANG) {
			dprintk(DBG_IOCTL, "%s: __nbd_ioctl case NBD_CLEAR_QUE: de-hang\n", disk->disk_name);
			nbd->error_now = 1;
		}
		/* Return error on all old requests. */
		nbd_clear_que(nbd);
		return 0;

	case NBD_PRINT_DEBUG:
		dev_info(disk_to_dev(nbd->disk),
			"next = %p, prev = %p, head = %p\n",
			nbd->queue_head.next, nbd->queue_head.prev,
			&nbd->queue_head);
		return 0;
	}
	return -ENOTTY;
}

static int nbd_ioctl(struct block_device *bdev, fmode_t mode,
		     unsigned int cmd, unsigned long arg)
{
	struct nbd_device *nbd = bdev->bd_disk->private_data;
	int error;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	BUG_ON(nbd->magic != NBD_MAGIC);

	if (nbd->disk == NULL) {
		dprintk(DBG_IOCTL, "nbd_ioctl: ioctl called on closed disk.\n");
	}

	/* Anyone capable of this syscall can do *real bad* things */
	dprintk(DBG_IOCTL, "%s: nbd_ioctl cmd=%s(0x%x) arg=%lu\n",
		nbd->disk->disk_name, ioctl_cmd_to_ascii(cmd), cmd, arg);

	mutex_lock(&nbd->tx_lock);
	error = __nbd_ioctl(bdev, nbd, cmd, arg);
	mutex_unlock(&nbd->tx_lock);

	return error;
}

static const struct block_device_operations nbd_fops =
{
	.owner =	THIS_MODULE,
	.ioctl =	nbd_ioctl,
};

/* Must be called with tx_lock held. */
static struct gendisk *nbd_add(int i)
{
	struct gendisk *disk = NULL;
	struct nbd_device *nbd;

	if (i >= nbds_max) {
		return NULL;
	}
	nbd = &nbd_dev[i];

	if (nbd->disk) {
		return nbd->disk;
	}

	disk = alloc_disk(1 << part_shift);
	if (!disk)
		return NULL;
	nbd->disk = disk;
	dprintk(DBG_IOCTL, "nbd_add: alloc_disk: 0x%p\n", nbd->disk);

	/*
	 * The new linux 2.5 block layer implementation requires
	 * every gendisk to have its very own request_queue struct.
	 * These structs are big so we dynamically allocate them.
	 */
	disk->queue = blk_init_queue(do_nbd_request, &nbd_lock);
	if (!disk->queue) {
		put_disk(disk);
		nbd->disk = NULL;
		return NULL;
	}
	dprintk(DBG_IOCTL, "nbd_add: blk_init_queue: 0x%p\n", disk->queue);
	/*
	 * Tell the block layer that we are not a rotational device
	 */
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, disk->queue);
	disk->queue->limits.discard_granularity = 512;
	disk->queue->limits.max_discard_sectors = UINT_MAX;
	disk->queue->limits.discard_zeroes_data = 0;
	blk_queue_max_hw_sectors(disk->queue, 65536);
	disk->queue->limits.max_sectors = 256;

	nbd->file = NULL;
	nbd->magic = NBD_MAGIC;
	nbd->flags = 0;
	INIT_LIST_HEAD(&nbd->waiting_queue);
	spin_lock_init(&nbd->queue_lock);
	INIT_LIST_HEAD(&nbd->queue_head);
	//mutex_init(&nbd->tx_lock);
	init_waitqueue_head(&nbd->active_wq);
	init_waitqueue_head(&nbd->waiting_wq);
	//init_waitqueue_head(&nbd->ack_wq);
	init_waitqueue_head(&nbd->control_wq);
	nbd->blksize = 1024;
	nbd->bytesize = 0;
	disk->major = NBD_MAJOR;
	disk->first_minor = i << part_shift;
	disk->fops = &nbd_fops;
	disk->private_data = nbd;
	sprintf(disk->disk_name, "nbd%d", i);
	set_capacity(disk, 0);
	/*
	 * add_disk() will be called after nbd_do_it() is running.
	 */
	return disk;
}

static void nbd_async_reg(struct work_struct *work)
{
	struct nbd_device *nbd = container_of(work, struct nbd_device, work);
	int ret;

	dprintk(DBG_IOCTL, "Enter nbd_async_reg\n");  
	/*
	 * add_disk() must be called WITHOUT tx_lock to avoid deadlock
	 * with nbd_thread.
	 */
	dprintk(DBG_IOCTL, "nbd_async_reg: add_disk: 0x%p\n", nbd->disk);
	add_disk(nbd->disk);
	dprintk(DBG_IOCTL, "nbd_async_reg: device_create_file\n");
	ret = device_create_file(disk_to_dev(nbd->disk), &pid_attr);
	if (ret) {
		dev_err(disk_to_dev(nbd->disk), "device_create_file failed!\n");
		nbd->pid = 0;
		return;
	}
	
	dprintk(DBG_IOCTL, "nbd_async_reg success!\n");
	nbd->reg_done = 1;
	dev_notice(disk_to_dev(nbd->disk), "register nbd success!\n");
	return;
}

static void nbd_remove(struct nbd_device *nbd)
{
	dprintk(DBG_IOCTL, "Enter nbd_remove\n");  
	if (nbd->disk == NULL){
		dprintk(DBG_IOCTL, "nbd_remove: disk is NULL for nbd %p!\n", nbd);
		return;
	}
	device_remove_file(disk_to_dev(nbd->disk), &pid_attr);
	if (nbd->disk->flags & GENHD_FL_UP){
		dprintk(DBG_IOCTL, "nbd_remove: del_gendisk: 0x%p\n", nbd->disk);
		del_gendisk(nbd->disk);
	}
	if (nbd->disk->queue){
		dprintk(DBG_IOCTL, "nbd_remove: cleanup_queue: 0x%p\n", nbd->disk->queue);
		blk_cleanup_queue(nbd->disk->queue);
	}
	put_disk(nbd->disk);
	nbd->disk = NULL;
}

static struct kobject *nbd_probe(dev_t dev, int *part, void *data)
{
	struct kobject *kobj;
	struct gendisk *disk;
	struct nbd_device *nbd;
	int i;

	i = MINOR(dev) >> part_shift;
	dprintk(DBG_IOCTL, "nbd_probe: probing nbd%d, minor:%d\n", i, MINOR(dev));

	if (i >= nbds_max) {
		return NULL;
	}
	nbd = &nbd_dev[i];
	nbd->sequence = 0;

	mutex_lock(&nbd->tx_lock);

	if (nbd->disk) {
		disk = nbd->disk;
		dprintk(DBG_IOCTL, "nbd use half setup disk: %p.\n", disk);
	} else {
		disk = nbd_add(i);
		dprintk(DBG_IOCTL, "nbd use new disk: %p.\n", disk);
	}
	if (disk == NULL) {
		mutex_unlock(&nbd->tx_lock);
		return NULL;
	}

	kobj = get_disk(disk);

	mutex_unlock(&nbd->tx_lock);

	*part = 0;

	return kobj;
}

/*
 * And here should be modules and kernel interface 
 *  (Just smiley confuses emacs :-)
 */

static int __init nbd_init(void)
{
	int err = -ENOMEM;
	int i = 0;
	struct nid_request *req_buf;

	dprintk(DBG_INIT, "nbd_init: Enter nbd_init with version %s\n", NBD_VERSION);
	BUILD_BUG_ON(sizeof(struct nid_request) != 32);

	if (max_part < 0) {
		dprintk(DBG_IOCTL, "nbd: max_part must be >= 0\n");  
		return -EINVAL;
	}

	nbd_dev = kcalloc(nbds_max, sizeof(*nbd_dev), GFP_KERNEL);
	if (!nbd_dev)
		return -ENOMEM;

	for (i = 0; i < nbds_max; i++) {
		req_buf = kcalloc(max_reqs, sizeof(*req_buf), GFP_KERNEL);
		if (!req_buf) {
			err = -ENOMEM;
			goto out;
		}
		nbd_dev[i].req_buf = req_buf;
	}

	part_shift = 0;
	if (max_part > 0) {
		part_shift = fls(max_part);

		/*
		 * Adjust max_part according to part_shift as it is exported
		 * to user space so that user can know the max number of
		 * partition kernel should be able to manage.
		 *
		 * Note that -1 is required because partition 0 is reserved
		 * for the whole disk.
		 */
		max_part = (1UL << part_shift) - 1;
	}

	if ((1UL << part_shift) > DISK_MAX_PARTS)
		return -EINVAL;

	if (nbds_max > 1UL << (MINORBITS - part_shift))
		return -EINVAL;

	if (register_blkdev(NBD_MAJOR, "nbd")) {
		err = -EIO;
		goto out;
	}

	dprintk(DBG_INIT, "nbd_init: registered device at major %d\n", NBD_MAJOR);
	dprintk(DBG_INIT, "nbd: debugflags=0x%x\n", debugflags);
	dprintk(DBG_INIT, "nbd: max_ack=%d\n", max_ack);
	dprintk(DBG_INIT, "nbd: dyn_dev=%d\n", dyn_dev);

	if (dyn_dev) {
		for (i = 0; i < nbds_max; i++) {
			mutex_init(&nbd_dev[i].tx_lock);
			nbd_dev[i].disconnect = 1;
		}
		/* Disk allocation delayed to register each nbd time*/
		blk_register_region(MKDEV(NBD_MAJOR, 0), 1UL << MINORBITS,
				  THIS_MODULE, nbd_probe, NULL, NULL);

		return 0;
	}

	for (i = 0; i < nbds_max; i++) {
		struct gendisk *disk = alloc_disk(1 << part_shift);
		if (!disk)
			goto out;
		nbd_dev[i].disk = disk;

		/*
		 * The new linux 2.5 block layer implementation requires
		 * every gendisk to have its very own request_queue struct.
		 * These structs are big so we dynamically allocate them.
		 */
		disk->queue = blk_init_queue(do_nbd_request, &nbd_lock);
		if (!disk->queue) {
			put_disk(disk);
			goto out;
		}
		/*
		 * Tell the block layer that we are not a rotational device
		 */
		queue_flag_set_unlocked(QUEUE_FLAG_NONROT, disk->queue);
		disk->queue->limits.discard_granularity = 512;
		disk->queue->limits.max_discard_sectors = UINT_MAX;
		disk->queue->limits.discard_zeroes_data = 0;
		blk_queue_max_hw_sectors(disk->queue, 65536);
		disk->queue->limits.max_sectors = 256;
	}

	for (i = 0; i < nbds_max; i++) {
		struct gendisk *disk = nbd_dev[i].disk;
		nbd_dev[i].file = NULL;
		nbd_dev[i].magic = NBD_MAGIC;
		nbd_dev[i].flags = 0;
		INIT_LIST_HEAD(&nbd_dev[i].waiting_queue);
		spin_lock_init(&nbd_dev[i].queue_lock);
		INIT_LIST_HEAD(&nbd_dev[i].queue_head);
		mutex_init(&nbd_dev[i].tx_lock);
		init_waitqueue_head(&nbd_dev[i].active_wq);
		init_waitqueue_head(&nbd_dev[i].waiting_wq);
		nbd_dev[i].disconnect = 1;
		nbd_dev[i].blksize = 1024;
		nbd_dev[i].bytesize = 0;
		disk->major = NBD_MAJOR;
		disk->first_minor = i << part_shift;
		disk->fops = &nbd_fops;
		disk->private_data = &nbd_dev[i];
		sprintf(disk->disk_name, "nbd%d", i);
		set_capacity(disk, 0);
		add_disk(disk);
	}

	return 0;
out:
	while (i--) {
		BUG_ON(dyn_dev);
		blk_cleanup_queue(nbd_dev[i].disk->queue);
		put_disk(nbd_dev[i].disk);
	}
	for (i = 0; i < nbds_max; i++) {
		req_buf = nbd_dev[i].req_buf;
		kfree(req_buf);
	}
	kfree(nbd_dev);
	return err;
}

static void __exit nbd_cleanup(void)
{
	int i;
	struct nid_request *req_buf;

	for (i = 0; i < nbds_max; i++) {
		struct gendisk *disk = nbd_dev[i].disk;
		nbd_dev[i].magic = 0;
		/* Cleanup any half setup nbd disk that never reach NBD_DO_IT.*/
		if (disk) {
			if (disk->flags & GENHD_FL_UP){
				dprintk(DBG_INIT, "nbd_cleanup: del_gendisk: 0x%p\n", disk);
				del_gendisk(disk);
			}
			if (disk->queue){
				dprintk(DBG_INIT, "nbd_cleanup: cleanup_queue: 0x%p\n", disk->queue);
				blk_cleanup_queue(disk->queue);
			}
			put_disk(disk);
		}
	}
	if (dyn_dev) {
		blk_unregister_region(MKDEV(NBD_MAJOR, 0), nbds_max);
	}
	unregister_blkdev(NBD_MAJOR, "nbd");
	for (i = 0; i < nbds_max; i++) {
		req_buf = nbd_dev[i].req_buf;
		kfree(req_buf);
	}
	kfree(nbd_dev);
	dprintk(DBG_IOCTL, "nbd: unregistered device at major %d\n", NBD_MAJOR);  
}

module_init(nbd_init);
module_exit(nbd_cleanup);

MODULE_DESCRIPTION("Network Block Device");
MODULE_LICENSE("GPL");

module_param(nbds_max, int, 0444);
MODULE_PARM_DESC(nbds_max, "number of network block devices to initialize (default: 16)");
module_param(max_part, int, 0444);
MODULE_PARM_DESC(max_part, "number of partitions per device (default: 0)");
module_param(dyn_dev, int, 0444);
MODULE_PARM_DESC(dyn_dev, "Only activate an nbd disk after it's properly setup (default: 0)");
module_param(max_ack, int, 0444);
MODULE_PARM_DESC(max_ack, "when requests in the ack list reach this number, the nbd_thread won't accept any more request (default: 16)");
module_param(send_wait, int, 0444);
MODULE_PARM_DESC(send_wait, "After we are waiting <send_wait> seconds to send next request with pending requests to send, consider the connection down.");
module_param(recv_wait, int, 0444);
MODULE_PARM_DESC(recv_wait, "After we are waiting <recv_wait> seconds for response with pending requests to complete, consider the connection down.");
#ifndef NDEBUG
module_param(debugflags, int, 0644);
MODULE_PARM_DESC(debugflags, "flags for controlling debug output");
#endif
