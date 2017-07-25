/*
 * dev.c
 * 	Implementation of Device module
 */

#include "nid.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "ddg_if.h"
#include "dev_if.h"
#include <linux/version.h>

#define NID_MAX_PART	15
#define NID_MAGIC 0x41544C41

#ifdef NODEBUG
#define nid_log(flags, fmt...)
#else /* NODEBUG */
#define nid_log(flags, fmt...) do { \
        if (logflags & (flags)) printk(KERN_DEBUG fmt); \
} while (0)
#define LOG_IOCTL       0x0004
#define LOG_INIT        0x0010
#define LOG_EXIT        0x0020
#define LOG_BLKDEV      0x0100
#define LOG_RX          0x0200
#define LOG_TX          0x0400
static unsigned int logflags = LOG_IOCTL | LOG_INIT | LOG_EXIT;
#endif /* NODEBUG */

#define NID_REQUEST_WAIT	15*HZ

static DEFINE_SPINLOCK(nid_queue_lock);
static int max_reqs = 1024;

extern unsigned int nids_max;
extern const struct block_device_operations nid_fops;


static int __nid_device_stop(struct dev_interface *dev_p);

struct dev_private {
	int			p_idx;		// index assigned by ddg
	int			p_minor;	// device minor number
	int			p_rsfd;
	int			p_dsfd;
	struct ddg_interface	*p_ddg;
	struct nid_device 	*p_nid;
	struct gendisk		*p_disk;
	int			p_part_shift;
	char			p_bundle_id[NID_MAX_UUID];
	wait_queue_head_t	p_rex_wq;
	uint8_t			p_receiver_done;
	uint8_t			p_dat_receiver_done;
	uint8_t			p_sender_done;
	uint8_t			p_dat_sender_done;
};

static struct block_device*
__nid_bdget(int minor)
{
	dev_t dev;
	dev = MKDEV(NID_MAJOR, minor);
	return bdget(dev);
}

static void
__sock_shutdown(struct nid_device *nid)
{
	/*
	 * Forcibly shutdown the socket causing all listeners to error
	 * and let the _nid_ctl_receiver_thread do cleanup work.
	 */
	mutex_lock(nid->nid_mutex);
	if (nid->ctl_sock) {
		nid_log_warning("shutting down ctl socket\n");
		kernel_sock_shutdown(nid->ctl_sock, SHUT_RDWR);
	}
	if (nid->data_sock) {
		nid_log_warning("shutting down data socket\n");
		kernel_sock_shutdown(nid->data_sock, SHUT_RDWR);
	}
	mutex_unlock(nid->nid_mutex);
}

static void
__nid_sock_timeout(unsigned long arg)
{
	struct task_struct *task = (struct task_struct *)arg;

	nid_log(LOG_IOCTL, "nid: killing hung io (%s, pid: %d)\n",
		task->comm, task->pid);
	force_sig(SIGKILL, task);
}

/*
 * NID READ request life cycle:
 * fetch_request -> ctl_snd_queue -> ctl_rcv_queue -> dat_rcv_queue -> end_io.
 *
 * NID WRITE request life cycle:
 * fetch_request -> ctl_snd_queue -> dat_snd_queue -> ctl_rcv_queue -> end_io.
 *
 * NID CTL request life cycle:
 * fetch_request -> ctl_snd_queue -> ctl_rcv_queue -> end_io.
 */
static void
__nid_end_io(struct request *req)
{
	char *log_header = "__nid_end_io";
	int error = req->errors ? -EIO : 0;
	struct request_queue *q = req->q;
	unsigned long flags;
	struct dev_interface *dev_p;
	struct dev_private *priv_p;
	struct nid_device *nid;

	dev_p = (struct dev_interface *)req->rq_disk->private_data;
	priv_p = (struct dev_private *)dev_p->dv_private;
	nid = priv_p->p_nid;
	if (error) {
		nid_log_warning("%s: minor:%d request %p failed", log_header, priv_p->p_minor, req);
	} else {
		nid_log_debug("%s: minor:%d request %p done", log_header, priv_p->p_minor, req);
	}
	spin_lock_irqsave(q->queue_lock, flags);
	__blk_end_request_all(req, error);
	nid->nr_req--;
	spin_unlock_irqrestore(q->queue_lock, flags);
}

static int
__sock_rw(struct nid_device *nid, int req, int send, void *buf, int size, int msg_flags)
{
	struct socket *sock = req ? nid->ctl_sock : nid->data_sock;
	int result;
	struct msghdr msg;
	struct kvec iov;
	sigset_t blocked, oldset;
	unsigned long pflags = current->flags;

	if (unlikely(!sock)) {
		dev_err(disk_to_dev(nid->disk),
			"Attempted %s on closed socket in __sock_rw\n",
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

			if (nid->xmit_timeout) {
				init_timer(&ti);
				ti.function = __nid_sock_timeout;
				ti.data = (unsigned long)current;
				ti.expires = jiffies + nid->xmit_timeout;
				add_timer(&ti);
			}
			result = kernel_sendmsg(sock, &msg, &iov, 1, size);
			if (nid->xmit_timeout)
				del_timer_sync(&ti);
		} else {
			result = kernel_recvmsg(sock, &msg, &iov, 1, size,
						msg.msg_flags);
		}

		if (signal_pending(current)) {
			nid_log(LOG_IOCTL, "nid (pid %d: %s) got signal.\n",
				task_pid_nr(current), current->comm);
			result = -EINTR;
			//sock_cleanup(nid, !send);
			break;
		}

		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}

		if (!req) {
			if (send)
				nid->dat_snd_sequence += result;
			else
				nid->dat_rcv_sequence += result;
		}

		size -= result;
		buf += result;
	} while (size > 0);

	sigprocmask(SIG_SETMASK, &oldset, NULL);
	tsk_restore_flags(current, pflags, PF_MEMALLOC);

	return result;
}

static inline int
__sock_recv_bio(struct nid_device *nid, struct bio_vec *bvec)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	result = __sock_rw(nid, 0, 0, kaddr + bvec->bv_offset, bvec->bv_len,
			MSG_WAITALL);
	kunmap(bvec->bv_page);
	return result;
}

static inline int
__sock_send_bio(struct nid_device *nid, struct bio_vec *bvec,
		int flags)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	result = __sock_rw(nid, 0, 1, kaddr + bvec->bv_offset,
			   bvec->bv_len, flags);
	kunmap(bvec->bv_page);
	return result;
}

static const char *
__nidcmd_to_str(int cmd)
{
	switch (cmd) {
	case NID_REQ_READ: return "read";
	case NID_REQ_WRITE: return "write";
	case NID_REQ_DELETE_DEVICE: return "disconnect";
	case NID_REQ_FLUSH: return "flush";
	case NID_REQ_TRIM: return "trim/discard";
	}
	return "invalid";
}

/* Always call with the nid_mutex held */
static int
__nid_fill_req_header(struct nid_device *nid, struct request *req, struct nid_request *ireq)
{
	u32 size = blk_rq_bytes(req);

	if (req->cmd_type != REQ_TYPE_FS) {
		return -EINVAL;
	} /* REQ_TYPE_SPECIAL */

	nid_cmd(req) = NID_REQ_READ;
	if (rq_data_dir(req) == WRITE) {
		if ((req->cmd_flags & REQ_DISCARD)) {
			nid_cmd(req) = NID_REQ_TRIM;
		} else {
			nid_cmd(req) = NID_REQ_WRITE;
		}

		if (nid->flags & NID_FLAG_READ_ONLY) {
			dev_err(disk_to_dev(nid->disk),
				"Write on read-only\n");
			return -EINVAL;
		}
	}

	/*
	if (req->cmd_flags & REQ_FLUSH) {
		BUG_ON(unlikely(blk_rq_sectors(req)));
		nid_cmd(req) = NID_REQ_FLUSH;
	}
	*/

	req->errors = 0;

	ireq->magic = htons(NID_REQUEST_MAGIC);
	ireq->cmd = nid_cmd(req);

	ireq->offset = cpu_to_be64((u64)blk_rq_pos(req) << 9);
	ireq->len = htonl(size);
	if (nid_cmd(req) == NID_REQ_WRITE) {
		ireq->dseq = cpu_to_be64(nid->sequence);
		nid->sequence += size;
	}
	memcpy(ireq->handle, &req, sizeof(req));

	nid_log(LOG_TX, "%s: ireq %p: preparing control (%s@%llu,%uB) %llu\n",
			nid->disk->disk_name, req,
			__nidcmd_to_str(nid_cmd(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req),
			be64_to_cpu(ireq->dseq));
	return 0;
}

static void
__nid_dump_queues(struct nid_device *nid)
{
	struct request *req, *tmp;

	nid_log(LOG_IOCTL, "%s: ctl_rcv_queue contains:\n", nid->disk->disk_name);
	list_for_each_entry_safe(req, tmp, &nid->ctl_rcv_queue, queuelist) {
		nid_log(LOG_IOCTL, "%p\n", req);
	}
	nid_log(LOG_IOCTL, "%s: ctl_rcv_queue end\n", nid->disk->disk_name);

	nid_log(LOG_IOCTL, "%s: dat_rcv_queue contains:\n", nid->disk->disk_name);
	list_for_each_entry_safe(req, tmp, &nid->dat_rcv_queue, queuelist) {
		nid_log(LOG_IOCTL, "%p\n", req);
	}
	nid_log(LOG_IOCTL, "%s: dat_rcv_queue end\n", nid->disk->disk_name);

	nid_log(LOG_IOCTL, "%s: ctl_snd_queue contains:\n", nid->disk->disk_name);
	list_for_each_entry_safe(req, tmp, &nid->ctl_snd_queue, queuelist) {
		nid_log(LOG_IOCTL, "%p\n", req);
	}
	nid_log(LOG_IOCTL, "%s: ctl_snd_queue end\n", nid->disk->disk_name);

	nid_log(LOG_IOCTL, "%s: dat_snd_queue contains:\n", nid->disk->disk_name);
	list_for_each_entry_safe(req, tmp, &nid->dat_snd_queue, queuelist) {
		nid_log(LOG_IOCTL, "%p\n", req);
	}
	nid_log(LOG_IOCTL, "%s: dat_snd_queue end\n", nid->disk->disk_name);

}

static struct request *
__nid_find_request(struct nid_device *nid, struct request *xreq)
{
	char *log_header = "__nid_find_request";
	struct request *req, *tmp;
	int err;

	err = 0;
	while (nid->curr_req == xreq) {
		err = wait_event_interruptible(nid->curr_wq, nid->curr_req != xreq);
	}
	if (unlikely(err)) {
		nid_log_error("%s: wait_event_interruptible (%s) err:%d\n",
			log_header, nid->disk->disk_name, err);
		goto out;
	}

	spin_lock(&nid->queue_lock);
	list_for_each_entry_safe(req, tmp, &nid->ctl_rcv_queue, queuelist) {
		if (req != xreq)
			continue;
		list_del_init(&req->queuelist);
		nid->ctl_rcv_len--;
		spin_unlock(&nid->queue_lock);

		return req;
	}

	nid_log_notice("%s: failed to find request(%p) in %s ctl_rcv_queue, ctl_rcv_len:%d, ctl_snd_len:%d\n",
		log_header, xreq, nid->disk->disk_name, nid->ctl_rcv_len, nid->ctl_snd_len);
	__nid_dump_queues(nid);
	spin_unlock(&nid->queue_lock);
	err = -ENOENT;

out:
	return ERR_PTR(err);
}

static int
__nid_read_resp(struct dev_interface *dev_p)
{
	char *log_header = "__nid_read_resp";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct ddg_interface *ddg_p = (struct ddg_interface *)priv_p->p_ddg;
	struct nid_device *nid = priv_p->p_nid;
	int minor = priv_p->p_minor;
	int result;
	struct nid_request reply;
	struct request *req;

next:
	result = __sock_rw(nid, 1, 0, &reply, sizeof(reply), MSG_WAITALL);
	if (result <= 0) {
		dev_err(disk_to_dev(nid->disk),
			"Receive control failed (result %d)\n", result);
		goto harderror;
	}

	/* Keepalive handling, Notify user agent. */
	if (reply.cmd == NID_REQ_KEEPALIVE) {
		nid->last_keepalive = jiffies;
		ddg_p->dg_op->dg_got_keepalive_channel(ddg_p, minor, &reply);
		nid->r_kcounter++;
		goto next;
	} else if (reply.cmd == NID_REQ_UPGRADE || reply.cmd == NID_REQ_FUPGRADE) {
		nid_log_debug("%s: cmd(7:U, 8:F): %d", log_header, reply.cmd);
		ddg_p->dg_op->dg_upgrade_channel_done(ddg_p, minor, reply.code);
		goto next;
	} else if (reply.cmd == NID_REQ_WRITE) {
		nid->r_wcounter++;
	} else if (reply.cmd == NID_REQ_TRIM) {
		nid->r_tcounter++;
	}

	req = __nid_find_request(nid, *(struct request **)reply.handle);
	if (IS_ERR(req)) {
		result = PTR_ERR(req);
		if (result != -ENOENT)
			goto harderror;

		dev_err(disk_to_dev(nid->disk), "Unexpected reply (%p)\n",
			reply.handle);
		// result = -EBADR;
		goto harderror;
	}

	if (ntohl(reply.code)) {
		dev_err(disk_to_dev(nid->disk), "Other side returned error (%d)\n",
			reply.code);
		req->errors++;
		__nid_end_io(req);
		return 0;
	}

	nid_log(LOG_RX, "%s: request %p: got reply\n",
			nid->disk->disk_name, req);
	if (nid_cmd(req) == NID_REQ_READ) {
		/* The receive order is critical! */
		spin_lock(&nid->queue_lock);
		list_add_tail(&req->queuelist, &nid->dat_rcv_queue);
		nid->dat_rcv_len++;
		nid->dat_rcv_len_total++;
		nid->r_rcounter++;
		spin_unlock(&nid->queue_lock);
		wake_up(&nid->dat_rcv_wq);
		return 0;
	}

	/* End WRITE & CTL requests here. */
	__nid_end_io(req);
	return 0;
harderror:
	nid->harderror = result;
	return 1;
}

static int
_nid_ctl_receiver_thread(void *data)
{
	char *log_header = "_nid_ctl_receiver_thread";
	struct dev_interface *dev_p = (struct dev_interface *)data;
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	int rc = 0;

	nid_log_notice("%s: start (minor:%d)", log_header, priv_p->p_minor);
	BUG_ON(nid->magic != NID_MAGIC);
	nid_log(LOG_IOCTL, "nid_dev:%p", nid);
	BUG_ON(nid->data_sock == NULL);
	BUG_ON(priv_p->p_receiver_done);

	sk_set_memalloc(nid->data_sock->sk);
	nid->pid = task_pid_nr(current);

	set_user_nice(current, -20);

	while (__nid_read_resp(dev_p) == 0) {
		BUG_ON(priv_p->p_receiver_done);
		;
	}

	nid_log_notice("%s: minor:%d to stop", log_header, priv_p->p_minor);
	BUG_ON(priv_p->p_receiver_done);
	rc = __nid_device_stop(dev_p);
	if (rc != 0) {
		nid_log_notice("%s: disconnecting minor:%d  ...", log_header, priv_p->p_minor);
	}
	nid_log_notice("%s: minor:%d leave", log_header, priv_p->p_minor);

	nid->pid = 0;
	BUG_ON(priv_p->p_receiver_done);
	priv_p->p_receiver_done = 1;
	return 0;
}

static int
_nid_dat_receiver_thread(void *data)
{
	char *log_header = "_nid_dat_receiver_thread";
	struct dev_interface *dev_p = (struct dev_interface *)data;
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	struct request *req;
	struct gendisk *disk = priv_p->p_disk;
	int result;

	BUG_ON(nid->magic != NID_MAGIC);
	BUG_ON(nid->data_sock == NULL);
	BUG_ON(priv_p->p_dat_receiver_done);

	nid_log_notice("%s: enter %s.", log_header, disk->disk_name);
	set_user_nice(current, -19);

	while (!kthread_should_stop()) {
		int rc = 0;

		/* wait for something to do, timeout in 10 secs */
		rc = wait_event_interruptible_timeout(nid->dat_rcv_wq,
			kthread_should_stop() || !list_empty(&nid->dat_rcv_queue),
			NID_REQUEST_WAIT);

		if (kthread_should_stop()) {
			nid_log_notice("%s: stop %s ...", log_header, disk->disk_name);
			break;
		}

		if (!nid->data_sock) {
			nid_log_notice("%s: Error %s, no data_sock...", log_header, disk->disk_name);
			continue;
		}

		/* extract request */
		spin_lock(&nid->queue_lock);
		while (!list_empty(&nid->dat_rcv_queue)) {
			struct req_iterator iter;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
			struct bio_vec bvec;
#else
			struct bio_vec *bvec;
#endif

			req = list_entry(nid->dat_rcv_queue.next, struct request, queuelist);
			list_del_init(&req->queuelist);
			nid->dat_rcv_len--;
			spin_unlock(&nid->queue_lock);

			BUG_ON(nid_cmd(req) != NID_REQ_READ);

			rq_for_each_segment(bvec, req, iter) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
				result = __sock_recv_bio(nid, &bvec);
#else
				result = __sock_recv_bio(nid, bvec);
#endif
				if (result <= 0) {
					nid_log_error("%s: Receive data failed (result %d) on %s",
						log_header, result, disk->disk_name);

					spin_lock(&nid->queue_lock);
					list_add(&req->queuelist, &nid->dat_rcv_queue);
					nid->dat_rcv_len++;
					nid->dat_rcv_len_total++;
					spin_unlock(&nid->queue_lock);

					/* Need recovery, wait to get stopped. */
					__sock_shutdown(nid);
					wait_event_interruptible(nid->dat_rcv_wq, kthread_should_stop());
					goto error_out;
				}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
				nid_log_debug("%s: request %p: got %d bytes data on %s\n",
					log_header, req, bvec.bv_len, disk->disk_name);
#else
				nid_log_debug("%s: request %p: got %d bytes data on %s\n",
					log_header, req, bvec->bv_len, disk->disk_name);
#endif
			}
			__nid_end_io(req);

			spin_lock(&nid->queue_lock);
		}
		spin_unlock(&nid->queue_lock);
	}

error_out:
	nid_log_notice("%s: leave %s", log_header, disk->disk_name);

	priv_p->p_dat_receiver_done = 1;
	return 0;
}

static int
__nid_send_req_data(struct nid_device *nid, struct request *req)
{
	int result, flags;
	int dsize = 0;
	u32 size = 0;

	nid->curr_req = req;
	/*
	 * For read requests, can not trust nid_cmd(ireq->handle). because read request could
	 * already been finished immediatelly after it's header send out.
	 */
	if (nid_cmd(req) == NID_REQ_WRITE) {
		struct req_iterator iter;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
		struct bio_vec bvec;
#else
		struct bio_vec *bvec;
#endif
		size = blk_rq_bytes(req);

		/* Use MSG_MORE */
		rq_for_each_segment(bvec, req, iter) {
			flags = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
			if (!rq_iter_last(bvec, iter))
				flags = MSG_MORE;
			nid_log(LOG_TX, "%s: request %p: sending %d bytes data\n",
					nid->disk->disk_name, req, bvec.bv_len);
			dsize += bvec.bv_len;
			BUG_ON(dsize > size);
			result = __sock_send_bio(nid, &bvec, flags);
#else
			if (!rq_iter_last(req, iter))
				flags = MSG_MORE;
			nid_log(LOG_TX, "%s: request %p: sending %d bytes data\n",
					nid->disk->disk_name, req, bvec->bv_len);
			dsize += bvec->bv_len;
			BUG_ON(dsize > size);
			result = __sock_send_bio(nid, bvec, flags);
#endif
			if (result <= 0) {
				dev_err(disk_to_dev(nid->disk),
					"Send data failed (result %d)\n",
					result);
				nid->dat_snd_err_total++;
				goto error_out;
			}
		}
		BUG_ON(dsize != size);
		nid->dat_snd_done_total += size;
	} else {
		BUG();
	}

	nid->curr_req = NULL;
	wake_up_all(&nid->curr_wq);
	return size;

error_out:
	nid->curr_req = NULL;
	wake_up_all(&nid->curr_wq);
	return -EIO;
}

// nid driver send message to nid server
/* Always call with the nid_mutex held */
static int
__nid_send_req(struct nid_device *nid, struct request *req)
{
	char *log_header = "__nid_send_req";
	int result;
	struct nid_request request;
	u32 size = blk_rq_bytes(req);

	request.magic = htons(NID_REQUEST_MAGIC);
	request.cmd = nid_cmd(req);

	if (nid_cmd(req) == NID_REQ_FLUSH) {
		/* Other values are reserved for FLUSH requests.  */
		request.len = 0;
		nid_log(LOG_IOCTL, "%s: not support NID_REQ_FLUSH yet", log_header);
		BUG();
	} else if (nid_cmd(req) == NID_REQ_KEEPALIVE) {
		request.offset = 0;
		request.dseq = 0;
	} else {
		// rd/wr/trim need set correct offset/len parameters
		request.offset = cpu_to_be64((u64)blk_rq_pos(req) << 9);
		request.len = htonl(size);
		if (nid_cmd(req) == NID_REQ_WRITE) {
			request.dseq = cpu_to_be64(nid->sequence);
		}
	}
	memcpy(request.handle, &req, sizeof(req));

	nid_log(LOG_TX, "%s: request %p: sending control (%s@%llu,%uB)\n",
			nid->disk->disk_name, req,
			__nidcmd_to_str(nid_cmd(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req));

	result = __sock_rw(nid, 1, 1, &request, sizeof(request), 0);
	if (result <= 0) {
		dev_err(disk_to_dev(nid->disk),
			"Send control failed (result %d)\n", result);
		goto error_out;
	}

	return 0;

error_out:
	return -EIO;
}

static void
__nid_handle_req_list(struct nid_device *nid, struct list_head *working_queue, int nr_reqs)
{
	struct request *req, *next_req;
	int result;
	int nr_send = 0, nr_wr = 0, nr_rd = 0, nr_trim = 0;
	struct nid_request *req_buf = nid->req_buf;
	int hsize = 0;

	LIST_HEAD(wr_queue);
	LIST_HEAD(trim_queue);

	list_for_each_entry_safe(req, next_req, working_queue, queuelist) {
		result = __nid_fill_req_header(nid, req, &req_buf[nr_send]);
		if (result != 0) {
			/* Drop bad requests directly. */
			nid_log(LOG_IOCTL, "%s: drop bad requests %p.\n", nid->disk->disk_name, req);
			nid->nr_bad_req_total++;
			list_del_init(&req->queuelist);
			req->errors++;
			__nid_end_io(req);
			continue;
		}
		switch(nid_cmd(req)) {
		case NID_REQ_WRITE:
			hsize += blk_rq_bytes(req);
			nr_wr++;
			/* WRITE Order is critical. */
			list_move_tail(&req->queuelist, &wr_queue);
			nid->wcounter++;
			break;
		case NID_REQ_READ:
			nr_rd++;
			nid->rcounter++;
			break;
		case NID_REQ_TRIM:
			nr_trim++;
			list_move_tail(&req->queuelist, &trim_queue);
			nid->tcounter++;
			break;
		}

		nr_send++;
	}

	if (nr_send == 0) {
		nid_log(LOG_IOCTL, "%s: zero good requests: nr_wr: %d, nr_rd: %d, nr_trim: %d.\n",
				nid->disk->disk_name, nr_wr, nr_rd, nr_trim);
		return;
	}

	/* Move requests to ack queue. */
	spin_lock(&nid->queue_lock);

	list_splice_tail_init(working_queue, &nid->ctl_rcv_queue);
	nid->ctl_rcv_len += nr_rd;
	nid->ctl_rcv_len_total += nr_rd;

	list_splice_tail_init(&wr_queue, &nid->dat_snd_queue);
	nid->dat_snd_len += nr_wr;
	nid->dat_snd_len_total += nr_wr;

	list_splice_tail_init(&trim_queue, &nid->ctl_rcv_queue);
	nid->ctl_snd_len += nr_trim;
	nid->ctl_snd_len_total += nr_trim;

	spin_unlock(&nid->queue_lock);

	mutex_lock(nid->nid_mutex);

	if (unlikely(!nid->data_sock)) {
		mutex_unlock(nid->nid_mutex);
		dev_err(disk_to_dev(nid->disk),
			"Attempted send on closed socket, req equeued\n");
		goto error_out;
	}

	/* NOTICE: The 'request' data structure of a read command can be
	 * finished and get reused immediatelly after it's header send out.
	 */
	result = __sock_rw(nid, 1, 1, req_buf, sizeof(struct nid_request) * nr_send, 0);
	if (result <= 0) {
		mutex_unlock(nid->nid_mutex);
		dev_err(disk_to_dev(nid->disk),
			"Send control failed (result %d)\n", result);
		goto error_out;
	}

	wake_up(&nid->dat_snd_wq);

	mutex_unlock(nid->nid_mutex);

	return;

error_out:
	/* Leave requests at ctl_rcv_queue / dat_snd_queue. */
	/* They will be moved back to when recover finished. */

	return;
}

static int
_nid_dat_sender_thread(void *data)
{
	char *log_header = "_nid_dat_sender_thread";
	struct dev_interface *dev_p = (struct dev_interface *)data;
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	struct gendisk *disk = priv_p->p_disk;
	struct request *req;
	int result;

	nid_log_notice("%s: start (%s) ... \n", log_header, disk->disk_name);
	BUG_ON(priv_p->p_dat_sender_done);

	set_user_nice(current, -19);
	while (!kthread_should_stop()) {
		int rc = 0;

		/* wait for something to do, timeout in 10 secs */
		rc = wait_event_interruptible_timeout(nid->dat_snd_wq,
			kthread_should_stop() || !list_empty(&nid->dat_snd_queue),
			NID_REQUEST_WAIT);

		if (kthread_should_stop()) {
			nid_log_notice("%s: stop %s ...", log_header, disk->disk_name);
			break;
		}

		if (!nid->data_sock) {
			continue;
		}

		/* extract request */
		spin_lock(&nid->queue_lock);
		while(!list_empty(&nid->dat_snd_queue)) {
			req = list_entry(nid->dat_snd_queue.next, struct request, queuelist);
			list_move_tail(&req->queuelist, &nid->ctl_rcv_queue);
			nid->dat_snd_len--;
			nid->ctl_rcv_len++;
			nid->ctl_rcv_len_total++;
			spin_unlock(&nid->queue_lock);

			result = __nid_send_req_data(nid, req);

			if (result < 0) {
				/* Socket error. Need retry. */
				__sock_shutdown(nid);
				nid_log_notice("%s: Send data error %s ...", log_header, disk->disk_name);
				spin_lock(&nid->queue_lock);
				break;
			}
			spin_lock(&nid->queue_lock);
		}
		spin_unlock(&nid->queue_lock);
	}

	/*
	for(i = 0; i < nr_send; i++) {
		ireq = req_buf + i;
		result = __nid_send_req_data(nid, ireq);
		if (result < 0) {
			mutex_unlock(&nid->nid_mutex);
			goto error_out;
		}
		if (ireq->cmd == NID_REQ_WRITE) {
			dsize += result;
		}
	}
	if (hsize != dsize) {
		nid_log(LOG_IOCTL, "nr_send: %d, hsize: %d, dsize: %d\n", nr_send, hsize, dsize);
		BUG_ON(hsize != dsize);
	}
	*/
	nid_log_notice("%s: leave %s ... \n", log_header, disk->disk_name);

	priv_p->p_dat_sender_done = 1;
	return 0;
}

static int
_nid_ctl_sender_thread(void *data)
{
	char *log_header = "_nid_ctl_sender_thread";
	int nr_reqs;
	struct dev_interface *dev_p = (struct dev_interface *)data;
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	struct gendisk *disk = priv_p->p_disk;
	struct request_queue *queue = disk->queue;
	LIST_HEAD(working_queue);

	nid_log_notice("%s: enter %s. rq_timeout:%u, nr_requests:%lu\n",
		log_header, disk->disk_name, queue->rq_timeout, queue->nr_requests);

	BUG_ON(priv_p->p_sender_done);
	set_user_nice(current, -20);
	while (!kthread_should_stop()) {
		int rc = 0;

		/* wait for something to do, timeout in 10 secs */
		rc = wait_event_interruptible_timeout(nid->ctl_snd_wq,
			kthread_should_stop() || !list_empty(&nid->ctl_snd_queue),
			NID_REQUEST_WAIT);

		if (kthread_should_stop()) {
			nid_log_notice("%s: stop %s ...", log_header, disk->disk_name);
			break;
		}

		if (list_empty(&nid->ctl_snd_queue) || !nid->data_sock) {
			continue;
		}

		/* Move all pending requests to the private working queue. */
		spin_lock_irq(&nid->queue_lock);
		list_splice_init(&nid->ctl_snd_queue, &working_queue);
		nr_reqs = nid->ctl_snd_len;
		nid->ctl_snd_len = 0;
		spin_unlock_irq(&nid->queue_lock);

		__nid_handle_req_list(nid, &working_queue, nr_reqs);
	}
	/* The nid_purge_que() after kthread_stop() will cleanup any pending reqs. */
	nid_log_notice("%s: leave %s", log_header, disk->disk_name);
	priv_p->p_sender_done = 1;
	return 0;
}

static int
__nid_start_threads(struct dev_interface *dev_p)
{
	char *log_header = "__nid_start_threads";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	int error = 0;

	nid->nid_ctl_snd_thread = kthread_create(_nid_ctl_sender_thread, dev_p, "%s", priv_p->p_disk->disk_name);
	if (IS_ERR(nid->nid_ctl_snd_thread)) {
		struct task_struct *thread = nid->nid_ctl_snd_thread;
		nid_log(LOG_IOCTL, "%s: cannot create ctl_sender for %s\n", log_header, priv_p->p_disk->disk_name);
		nid->nid_ctl_snd_thread = NULL;
		error = PTR_ERR(thread);
		goto error_out;
	}
	nid_log_info("%s: wake_up ctl_sender for %s...", log_header, nid->disk->disk_name);
	wake_up_process(nid->nid_ctl_snd_thread);

	nid->nid_dat_snd_thread = kthread_create(_nid_dat_sender_thread, dev_p, "%sds", priv_p->p_disk->disk_name);
	if (IS_ERR(nid->nid_dat_snd_thread)) {
		struct task_struct *thread = nid->nid_dat_snd_thread;
		nid_log(LOG_IOCTL, "%s: nid_start: cannot create dat_sender\n", priv_p->p_disk->disk_name);
		nid->nid_dat_snd_thread = NULL;
		error = PTR_ERR(thread);
		goto error_out;
	}
	nid_log_info("nid_start(%s): wake_up dat_sender ...", priv_p->p_disk->disk_name);
	wake_up_process(nid->nid_dat_snd_thread);

	nid->nid_ctl_rcv_thread = kthread_create(_nid_ctl_receiver_thread, dev_p, "%scr", priv_p->p_disk->disk_name);
	if (IS_ERR(nid->nid_ctl_rcv_thread)) {
		struct task_struct *thread = nid->nid_ctl_rcv_thread;
		nid_log(LOG_IOCTL, "%s: nid_start: cannot create ctl_receiver\n", priv_p->p_disk->disk_name);
		nid->nid_ctl_rcv_thread = NULL;
		error = PTR_ERR(thread);
		goto error_out;
	}
	nid_log_info("nid_start(%s): wake_up ctl_receiver...", priv_p->p_disk->disk_name);
	wake_up_process(nid->nid_ctl_rcv_thread);

	nid->nid_dat_rcv_thread = kthread_create(_nid_dat_receiver_thread, dev_p, "%sdr", priv_p->p_disk->disk_name);
	if (IS_ERR(nid->nid_dat_rcv_thread)) {
		struct task_struct *thread = nid->nid_dat_rcv_thread;
		nid_log(LOG_IOCTL, "%s: nid_start: cannot create dat_receiver\n", priv_p->p_disk->disk_name);
		nid->nid_dat_rcv_thread = NULL;
		error = PTR_ERR(thread);
		goto error_out;
	}
	nid_log_info("nid_start(%s): wake_up dat_receiver...", priv_p->p_disk->disk_name);
	wake_up_process(nid->nid_dat_rcv_thread);

	return 0;

error_out:
	if (nid->nid_ctl_snd_thread != NULL) {
		kthread_stop(nid->nid_ctl_snd_thread);
		nid->nid_ctl_snd_thread = NULL;
	}
	if (nid->nid_dat_snd_thread != NULL) {
		kthread_stop(nid->nid_dat_snd_thread);
		nid->nid_dat_snd_thread = NULL;
	}
	if (nid->nid_ctl_rcv_thread != NULL) {
		kthread_stop(nid->nid_ctl_rcv_thread);
		nid->nid_ctl_rcv_thread = NULL;
	}
	if (nid->nid_dat_rcv_thread != NULL) {
		kthread_stop(nid->nid_dat_rcv_thread);
		nid->nid_dat_rcv_thread = NULL;
	}
	return error;
}

static void
__nid_async_reg(struct work_struct *work)
{
	char *log_header = "__nid_async_reg";
	struct nid_device *nid = container_of(work, struct nid_device, work);

	nid_log_notice("%s: start ...", log_header);
	/*
	 * add_disk() will generate IO on the device.
	 * Must be called WITHOUT nid_mutex to avoid deadlock with nid_thread.
	 */
	nid_log_notice("%s: disk:0x%p", log_header, nid->disk);
	add_disk(nid->disk);
	nid_log_notice("%s: disk:0x%p: succeed!", log_header, nid->disk);
	nid->reg_done = 1;
	return;
}

/* Start worker threads when sockets are ready. */
/* Always call with nid_mutex locked. */
static int
__nid_start(struct dev_interface *dev_p, struct block_device *bdev)
{
	char *log_header = "__nid_start";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	struct gendisk *disk = priv_p->p_disk;
	int error = 0;

	nid_log_notice("%s: start (%s) ...", log_header, disk->disk_name);
	if (nid->inited) {
		/* this must be persistent mode, there could be some requests waiting to retry */
		nid_log(LOG_IOCTL, "%s: re-hang %s\n", log_header, disk->disk_name);
		nid->error_now = 0;
		priv_p->p_receiver_done = 0;
		priv_p->p_dat_receiver_done = 0;
		priv_p->p_sender_done = 0;
		priv_p->p_dat_sender_done = 0;
		error = __nid_start_threads(dev_p);
		return error;
	}

	/*
	 * Only do these setup at the first time
	 */
	if (nid->flags & NID_FLAG_READ_ONLY)
		set_device_ro(bdev, true);

	if (nid->flags & NID_FLAG_SEND_TRIM)
		queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, nid->disk->queue);
	if (nid->flags & NID_FLAG_SEND_FLUSH)
		blk_queue_flush(nid->disk->queue, REQ_FLUSH);
	else
		blk_queue_flush(nid->disk->queue, 0);

	blk_queue_flush(priv_p->p_disk->queue, 0);

	error = __nid_start_threads(dev_p);
	if (error != 0) {
		goto error_out;
	}

	nid->reg_done = 0;
	nid->cntl_req_done = 0;
	nid->cntl_req = NULL;
	nid->nr_req = 0;
	INIT_WORK(&nid->work, __nid_async_reg);
	schedule_work(&nid->work);
	nid->inited = 1;

	return 0;

error_out:
	return error;
}

static int
__nid_stop_threads(struct nid_device *nid)
{
	/* _nid_ctl_receiver_thread stop all other threads but itself. */
	if (nid->nid_ctl_snd_thread) {
		kthread_stop(nid->nid_ctl_snd_thread);
		nid->nid_ctl_snd_thread = NULL;
	}
	if (nid->nid_dat_snd_thread) {
		kthread_stop(nid->nid_dat_snd_thread);
		nid->nid_dat_snd_thread = NULL;
	}
	if (nid->nid_dat_rcv_thread) {
		kthread_stop(nid->nid_dat_rcv_thread);
		nid->nid_dat_rcv_thread = NULL;
	}
	return 0;
}

static void
__sock_cleanup(struct nid_device *nid)
{

	struct file *file;
	if (nid->ctl_sock) {
		nid->ctl_sock = NULL;
	}
	if (nid->data_sock) {
		nid->data_sock = NULL;
	}

	file = nid->ctl_file;
	nid->ctl_file = NULL;
	if (file)
		fput(file);
	file = nid->data_file;
	nid->data_file = NULL;
	if (file)
		fput(file);

}

static void
__nid_stat_cleanup(struct nid_device *nid)
{
	nid->rcounter 	= 0;
	nid->r_rcounter = 0;
	nid->wcounter 	= 0;
	nid->r_wcounter = 0;
	nid->kcounter 	= 0;
	nid->r_kcounter = 0;
	nid->dat_snd_sequence = 0;
	nid->dat_rcv_sequence = 0;
}

static int __nid_stop(struct nid_device *nid)
{
	__sock_shutdown(nid);

	__nid_stop_threads(nid);

	mutex_lock(nid->nid_mutex);
	__sock_cleanup(nid);
	__nid_stat_cleanup(nid);
	mutex_unlock(nid->nid_mutex);

	return 0;
}

static int
__nid_purge_que(struct nid_device *nid)
{
	char *log_header = "__nid_purge_que";
	struct request *req;

	BUG_ON(nid->magic != NID_MAGIC);

	//BUG_ON(nid->data_sock);
	//BUG_ON(nid->curr_req);
	if (nid->data_sock || nid->curr_req) {
		nid_log_error("%s: EBUSY", log_header);
		return -EBUSY;
	}

	/* We already set nid->error_now to 1 under the queue_lock.
	 * There should be no more modification to the lists.
	 */

	while (!list_empty(&nid->dat_rcv_queue)) {
		spin_lock(&nid->queue_lock);
		req = list_entry(nid->dat_rcv_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nid->dat_rcv_len--;
		spin_unlock(&nid->queue_lock);
		req->errors++;
		__nid_end_io(req);
	}

	while (!list_empty(&nid->ctl_rcv_queue)) {
		spin_lock(&nid->queue_lock);
		req = list_entry(nid->ctl_rcv_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nid->ctl_rcv_len--;
		spin_unlock(&nid->queue_lock);
		req->errors++;
		__nid_end_io(req);
	}

	while (!list_empty(&nid->dat_snd_queue)) {
		spin_lock(&nid->queue_lock);
		req = list_entry(nid->dat_snd_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nid->dat_snd_len--;
		spin_unlock(&nid->queue_lock);
		req->errors++;
		__nid_end_io(req);
	}

	while (!list_empty(&nid->ctl_snd_queue)) {
		spin_lock(&nid->queue_lock);
		req = list_entry(nid->ctl_snd_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nid->ctl_snd_len--;
		spin_unlock(&nid->queue_lock);
		req->errors++;
		__nid_end_io(req);
	}

	return 0;
}

static void
__nid_remove(struct dev_private *priv_p)
{
	char *log_header = "__nid_remove";
	struct nid_device *nid = priv_p->p_nid;

	nid_log_notice("%s: start (minor:%d) ...", log_header, priv_p->p_minor);
	if (priv_p->p_disk == NULL){
		nid_log_info("%s: disk is NULL for minor:%d!", log_header, priv_p->p_minor);
		return;
	}
	if (priv_p->p_disk->flags & GENHD_FL_UP){
		nid_log_info("%s: del_gendisk: 0x%p", log_header, priv_p->p_disk);
		del_gendisk(priv_p->p_disk);
	}
	if (priv_p->p_disk->queue){
		nid_log_info("%s: cleanup_queue: 0x%p", log_header, priv_p->p_disk->queue);
		blk_cleanup_queue(priv_p->p_disk->queue);
	}
	put_disk(priv_p->p_disk);
	priv_p->p_disk = NULL;
	nid->disk = NULL;
}

static int
__nid_recover(struct dev_interface *dev_p, struct block_device *bdev)
{
	char *log_header = "__nid_recover";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct ddg_interface *ddg_p = (struct ddg_interface *)priv_p->p_ddg;
	struct nid_device *nid = priv_p->p_nid;
	int minor = priv_p->p_minor;
	int error = 0;

	nid_log_notice("%s: minor:%d", log_header, priv_p->p_minor);

	__nid_stop(nid);

	mutex_lock(nid->nid_mutex);
	spin_lock(&nid->queue_lock);
	nid->error_now = 0;
	spin_unlock(&nid->queue_lock);

	/*
	 * splice all pending requests to the waiting send queue to resend
	 * these requests to avoid I/O errors since we are in persistent mode
	 */
	spin_lock(&nid->queue_lock);

	if (!list_empty(&nid->dat_rcv_queue)) {
		list_splice_init(&nid->dat_rcv_queue, nid->ctl_snd_queue.next);
		nid->ctl_snd_len += nid->dat_rcv_len;
		nid->dat_rcv_len = 0;
	}

	if (!list_empty(&nid->ctl_rcv_queue)) {
		list_splice_init(&nid->ctl_rcv_queue, nid->ctl_snd_queue.next);
		nid->ctl_snd_len += nid->ctl_rcv_len;
		nid->ctl_rcv_len = 0;
	}

	if (!list_empty(&nid->dat_snd_queue)) {
		list_splice_init(&nid->dat_snd_queue, nid->ctl_snd_queue.next);
		nid->ctl_snd_len += nid->dat_snd_len;
		nid->dat_snd_len = 0;
	}

	spin_unlock(&nid->queue_lock);

	mutex_unlock(nid->nid_mutex);
	ddg_p->dg_op->dg_recover_channel_done(ddg_p, minor);
	error = nid->harderror;
	return error;
}

static int
__nid_disconnect(struct dev_interface *dev_p, struct block_device *bdev)
{
	char *log_header = "__nid_disconnect";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct ddg_interface *ddg_p = (struct ddg_interface *)priv_p->p_ddg;
	struct nid_device *nid = priv_p->p_nid;
	int minor = priv_p->p_minor;
#if 0
	if (!nid->reg_done) {
		nid->harderror = -EBADR;
		nid_log(LOG_IOCTL, "__nid_ioctl: case NID_STOP. __nid_async_reg not finished yet even after nid_start\n");
	}

	if (nid->disconnect || nid->harderror == -EBADR) {
		need_exit = 1;
	}
#endif

	__nid_stop(nid);

	mutex_lock(nid->nid_mutex);

	if (nid->inited == 0) {
		mutex_unlock(nid->nid_mutex);
		nid_log_debug("%s: nid already disconnected, skip.", log_header);
		return 1;
	}

	nid->inited = 0;

	/*
	 * Just do NID_CLEAR_SOCK's job here.
	 * nid-client should not call NID_CLEAR_SOCK ioctl again.
	 */

	spin_lock(&nid->queue_lock);
	nid->error_now = 1;
	spin_unlock(&nid->queue_lock);
	/* error_now set, no more new requests comming in. */
	__nid_purge_que(nid);

	// kill_bdev(bdev);
	queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, nid->disk->queue);
	set_device_ro(bdev, false);
	nid->flags = 0;
	nid->bytesize = 0;
	bdev->bd_inode->i_size = 0;
	set_capacity(nid->disk, 0);

	nid_log_notice("%s: calling cancel_work_sync", log_header);
	cancel_work_sync(&nid->work);	// in case the job not finished yet
	nid_log_notice("%s: after cancel_work_sync", log_header);
	__nid_remove(priv_p);

	if (NID_MAX_PART > 0)
		ioctl_by_bdev(bdev, BLKRRPART, 0);

	mutex_unlock(nid->nid_mutex);
	ddg_p->dg_op->dg_delete_device_done(ddg_p, minor);
	return 0;
}

static int
__nid_device_stop(struct dev_interface *dev_p)
{
	char *log_header = "__nid_device_stop";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	struct block_device *bdev = NULL;
	int need_exit = 0;

	nid_log_notice("%s: start(minor:%d)...", log_header, priv_p->p_minor);
	if (!nid->disk) {
		nid_log_notice("%s: minor:%d has null disk...", log_header, priv_p->p_minor);
		need_exit = 1;
		goto out;
	}

	nid_log_notice("%s: minor:%d", log_header, priv_p->p_minor);
	bdev = __nid_bdget(priv_p->p_minor);
	if (!bdev) {
		nid_log_debug("%s: minor:%d has null bdev...", log_header, priv_p->p_minor);
		need_exit = 1;
		goto out;
	}

	if (nid->disconnect) {
		nid_log_debug("%s: disconnecting minor:%d ...", log_header, priv_p->p_minor);
		__nid_disconnect(dev_p, bdev);
		need_exit = 1;
	} else {
		nid_log_debug("%s: recovering minor:%d ...", log_header, priv_p->p_minor);
		__nid_recover(dev_p, bdev);
		need_exit = 0;
	}

out:
	if (bdev != NULL) {
		bdput(bdev);
	}
	return need_exit;
}

static int
__nid_device_set_sock(struct dev_interface *dev_p, struct block_device *bdev)
{
	char *log_header = "__nid_device_set_sock";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	int rsfd = priv_p->p_rsfd;
	struct gendisk *disk = nid->disk;
	struct file *file;
	int rc = 0;

	nid_log_notice("%s: start (%s)", log_header, disk->disk_name);
	if (nid->ctl_file) {
		nid_log_error("nid_device_set_sock [%s]: busy",
			disk->disk_name);
		rc = -EBUSY;
		goto out;
	}

	file = fget(rsfd);
	if (file) {
		struct inode *inode = file_inode(file);
		if (S_ISSOCK(inode->i_mode)) {
			nid->sequence = 0;
			nid->ctl_file = file;
			nid->ctl_sock = SOCKET_I(inode);
			bdev->bd_invalidated = 1;
			nid->disconnect = 0; /* we're connected now */
		} else {
			fput(file);
			rc = -EINVAL;
		}
	} else {
		rc = -EINVAL;
	}

out:
	return rc;
}

static int
__nid_device_set_dsock(struct dev_interface *dev_p, struct block_device *bdev)
{
	char *log_header = "__nid_device_set_dsock";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	int dsfd = priv_p->p_dsfd;
	struct gendisk *disk = nid->disk;
	struct file *data_file;
	int rc = 0;

	nid_log_notice("%s: start (%s)", log_header, disk->disk_name);
	if (nid->data_file) {
		nid_log_error("nid_device_set_dsock [%s]: busy",
			disk->disk_name);
		rc = -EBUSY;
		goto out;
	}
	if (!nid->ctl_file) {
		nid_log_info("nid_device_set_dsock [%s]: dsock should be set after sock",
			disk->disk_name);
		rc = -EINVAL;
		goto out;
	}

	data_file = fget(dsfd);
	if (data_file) {
		struct inode *inode = file_inode(data_file);
		if (S_ISSOCK(inode->i_mode)) {
			nid->data_file = data_file;
			nid->data_sock = SOCKET_I(inode);
			bdev->bd_invalidated = 1;
		} else {
			fput(data_file);
			rc = -EINVAL;
		}
	} else {
		rc = -EINVAL;
	}

out:
	return rc;
}

static void __nid_request_handler(struct request_queue *q)
		__releases(q->queue_lock) __acquires(q->queue_lock)
{
	char *log_header = "__nid_request_handler";
	struct request *req;

	while ((req = blk_fetch_request(q)) != NULL) {
		struct dev_interface *dev_p;
		struct dev_private *priv_p;
		struct nid_device *nid;

		dev_p = (struct dev_interface *)req->rq_disk->private_data;
		priv_p = (struct dev_private *)dev_p->dv_private;
		nid = priv_p->p_nid;
		nid->nr_req++;
		nid->nr_req_total++;
		spin_unlock_irq(q->queue_lock);

		nid_log_debug("%s: %s: request %p: dequeued (flags=%x)\n",
			log_header, req->rq_disk->disk_name, req, req->cmd_type);
		BUG_ON(nid->magic != NID_MAGIC);

		spin_lock_irq(&nid->queue_lock);
		if (unlikely(nid->error_now)) {
			/* error_now, Drop all incomming requests. */
			spin_unlock_irq(&nid->queue_lock);
			req->errors++;
			nid_log_warning("%s: minor:%d is error_now", log_header, priv_p->p_minor);
			__nid_end_io(req);
			spin_lock_irq(q->queue_lock);
			continue;
		}
		list_add_tail(&req->queuelist, &nid->ctl_snd_queue);
		nid->ctl_snd_len++;
		nid->ctl_snd_len_total++;
		spin_unlock_irq(&nid->queue_lock);

		wake_up(&nid->ctl_snd_wq);
		spin_lock_irq(q->queue_lock);
		continue;
	}
}

static int
dev_init_device(struct dev_interface *dev_p, uint8_t chan_type, uint64_t size, uint32_t blksize, char *bundle_id)
{
	char *log_header = "dev_init_device";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct ddg_interface *ddg_p = (struct ddg_interface *)priv_p->p_ddg;
	int minor = priv_p->p_minor;
	struct block_device *bdev;
	struct nid_device *nid = priv_p->p_nid;
	uint8_t rc = 0;

	nid_log_notice("%s: (minor:%d, size:%llu, blksize:%u): start ...",
		log_header, minor, size, blksize);
	bdev = __nid_bdget(minor);
	if (!bdev) {
		rc = 1;
		nid_log_error("%s: (minor:%d): got null bdev", log_header, minor);
		goto out;
	}
	if (!bdev->bd_disk) {
		rc = 2;
		nid_log_error("%s: (minor:%d): got null bdev->bd_disk", log_header, minor);
		goto out;
	}
	if (!nid) {
		rc = 3;
		nid_log_error("%s: (minor:%d): got null nid", log_header, minor);
		goto out;
	}

	nid_log_error("%s: (minor:%d): step 1", log_header, minor);
	nid->blksize = blksize;
	nid->bytesize = size;
	bdev->bd_inode->i_size = nid->bytesize;
	set_blocksize(bdev, nid->blksize);
	set_capacity(priv_p->p_disk, nid->bytesize >> 9);
	nid->error_now = 0;
out:
	if (bdev) {
		bdput(bdev);
	}
	ddg_p->dg_op->dg_init_device_done(ddg_p, minor, chan_type, rc);
	return 0;
}

static int
dev_start_channel(struct dev_interface *dev_p, int rsfd, int dsfd, int do_purge)
{
	char *log_header = "dev_start_channel";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct ddg_interface *ddg_p = (struct ddg_interface *)priv_p->p_ddg;
	int minor = priv_p->p_minor;
	struct block_device *bdev;
	struct nid_device *nid = priv_p->p_nid;
	int error;

	nid_log_notice("%s: (minor:%d, rsfd:%d, dsfd:%d): start...",
		log_header, minor, rsfd, dsfd);
	bdev = __nid_bdget(minor);
	if (!bdev) {
		nid_log_error("%s: (minor:%d): got null bdev", log_header, minor);
		//bdput(bdev);
		return -EINVAL;
	}
	if (!bdev->bd_disk) {
		nid_log_error("%s: (minor:%d): got null bdev->bd_disk", log_header, minor);
		bdput(bdev);
		return -EINVAL;
	}

	if (!nid) {
		nid_log_error("%s: (minor:%d): got null nid", log_header, minor);
		bdput(bdev);
		return -EINVAL;
	}

	priv_p->p_rsfd = rsfd;
	priv_p->p_dsfd = dsfd;
	// kill_bdev(bdev);
	if (do_purge) {
		int do_ioerror = 0;
		spin_lock(&nid->queue_lock);
		if (!nid->error_now) {
			do_ioerror = 1;
			nid->error_now = 1;
		}
		spin_unlock(&nid->queue_lock);
		__nid_purge_que(nid);
		if (do_ioerror) {
			spin_lock(&nid->queue_lock);
			nid->error_now = 0;
			spin_unlock(&nid->queue_lock);
		}
	}

	mutex_lock(nid->nid_mutex);
	__nid_device_set_sock(dev_p, bdev);
	__nid_device_set_dsock(dev_p, bdev);
	nid->flags |= NID_FLAG_SEND_TRIM;
	error = __nid_start(dev_p, bdev);
	mutex_unlock(nid->nid_mutex);
	ddg_p->dg_op->dg_start_channel_done(ddg_p, minor);
#if 0
nid_log_notice("%s: ioctl (BLKRRPART)%d", log_header, BLKRRPART);
ioctl_by_bdev(bdev, BLKRRPART, 0);
#endif
	nid_log_notice("%s: (minor:%d) done", log_header, minor);
	bdput(bdev);
	return 0;
}

void
dev_purge(struct dev_interface *dev_p)
{
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	int do_ioerror = 0;

	spin_lock(&nid->queue_lock);
	if (!nid->error_now) {
		do_ioerror = 1;
		nid->error_now = 1;
	}
	spin_unlock(&nid->queue_lock);
	__nid_purge_que(nid);
	if (do_ioerror) {
		spin_lock(&nid->queue_lock);
		nid->error_now = 0;
		spin_unlock(&nid->queue_lock);
	}
}

/*
 * Got recover request from the agent
 * Algorithm:
 * 	shutdown sockets to trigger recovery
 */
static void
dev_recover_channel(struct dev_interface *dev_p)
{
	char *log_header = "dev_recover_channel";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	int minor = priv_p->p_minor;
	struct block_device *bdev;
	struct nid_device *nid = priv_p->p_nid;

	nid_log_debug("%s: minor:%d", log_header, minor);
	bdev = __nid_bdget(minor);
	if (!bdev || !bdev->bd_disk || !nid) {
		nid_log_debug("%s: wrong device, minor:%d", log_header, minor);
		bdput(bdev);
		return;
	}
	__sock_shutdown(nid);
	bdput(bdev);
	return;
}

static void
dev_nid_to_delete(struct dev_interface *dev_p)
{
	char *log_header = "dev_nid_to_delete";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct ddg_interface *ddg_p = (struct ddg_interface *)priv_p->p_ddg;
	int minor = priv_p->p_minor;
	struct block_device *bdev;
	struct nid_device *nid = priv_p->p_nid;
	struct gendisk *disk;
	struct request sreq;

	nid_log_notice("%s: start (minor:%d) ...", log_header, minor);
	bdev = __nid_bdget(minor);
	if (!bdev || !bdev->bd_disk || !nid) {
		nid_log_notice("%s: try to delete non-existed device (minor:%d)", log_header, minor);
		ddg_p->dg_op->dg_delete_device_done(ddg_p, minor);
		bdput(bdev);
		return;
	}
	disk = priv_p->p_disk;

	if (!nid->data_sock) {
		/*TODO: cleanup all threads. */
		nid_log_error("%s: [minor:%d]: deleting disconnected nid.", log_header, minor);
		nid->disconnect = 1;
		/* _nid_ctl_receiver_thread is not there, we must cleanup by ourself. */
		__nid_device_stop(dev_p);
		bdput(bdev);
		return;
	}
	fsync_bdev(bdev);

	blk_rq_init(NULL, &sreq);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
	sreq.cmd_type = REQ_TYPE_DRV_PRIV;
#else
	sreq.cmd_type = REQ_TYPE_SPECIAL;
#endif
	nid_cmd(&sreq) = NID_REQ_DELETE_DEVICE;
	mutex_lock(nid->nid_mutex);
	if (!nid->data_sock) {
		nid_log_notice("%s: try to delete disconnected device (minor:%d)", log_header, minor);
		nid->disconnect = 1;
		mutex_unlock(nid->nid_mutex);
		bdput(bdev);
		return;
	}
	nid->disconnect = 1;
	__nid_send_req(nid, &sreq);
	mutex_unlock(nid->nid_mutex);
	bdput(bdev);
	nid_log_notice("%s: waiting receiver_done (minor:%d)", log_header, minor);
	while (!priv_p->p_receiver_done ||
	       !priv_p->p_dat_receiver_done ||
	       !priv_p->p_sender_done ||
	       !priv_p->p_dat_sender_done)
		msleep(100);
	return;
}

static int
dev_upgrade_channel(struct dev_interface *dev_p, char force)
{
	char *log_header = "dev_upgrade_channel";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	int minor = priv_p->p_minor;
	struct block_device *bdev;
	struct nid_device *nid = priv_p->p_nid;
	struct request req;

	nid_log_debug("%s: [minor:%d]: ...", log_header, minor);
	bdev = __nid_bdget(minor);
	if (!bdev) {
		nid_log_error("%s: (minor:%d): got null bdev", log_header, minor);
		bdput(bdev);
		return -EINVAL;
	}
	if (!bdev->bd_disk) {
		nid_log_error("%s: [minor:%d]: got null bdev->bd_disk", log_header, minor);
		bdput(bdev);
		return -EINVAL;
	}
	if (!nid) {
		nid_log_error("%s: [minor:%d]: got null nid", log_header, minor);
		bdput(bdev);
		return -EINVAL;
	}

	blk_rq_init(NULL, &req);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
	req.cmd_type = REQ_TYPE_DRV_PRIV;
#else
	req.cmd_type = REQ_TYPE_SPECIAL;
#endif
	if (force)
		nid_cmd(&req) = NID_REQ_FUPGRADE;
	else
		nid_cmd(&req) = NID_REQ_UPGRADE;

	mutex_lock(nid->nid_mutex);
	__nid_send_req(nid, &req);
	mutex_unlock(nid->nid_mutex);
	nid_log_debug("%s: [minor:%d]: done", log_header, minor);
	bdput(bdev);
	return 0;
}

static void
dev_nid_set_ioerror(struct dev_interface *dev_p)
{
	char *log_header = "dev_nid_set_ioerror";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct ddg_interface *ddg_p = (struct ddg_interface *)priv_p->p_ddg;
	int minor = priv_p->p_minor;
	struct block_device *bdev;
	struct nid_device *nid = priv_p->p_nid;
	struct gendisk *disk;
	int rc = 0;

	bdev = __nid_bdget(minor);
	if (!bdev) {
		nid_log_error("%s: no bdev, minor:%d", log_header, minor);
		bdput(bdev);
		rc = -1;
		goto send_msg;
	}

	if (!nid) {
		nid_log_error("%s: no nid, minor:%d", log_header, minor);
		bdput(bdev);
		rc = -1;
		goto send_msg;
	}
	disk = priv_p->p_disk;

	spin_lock(&nid->queue_lock);
	nid->error_now = 1;
	spin_unlock(&nid->queue_lock);
	/* error_now set, no more new requests comming in. */
	__nid_purge_que(nid);

	nid_log_debug("%s: done, minor:%d", log_header, minor);

send_msg:
	ddg_p->dg_op->dg_set_ioerror_device_done(ddg_p, minor, rc);
	bdput(bdev);
	return;
}

static int
dev_send_keepalive_channel(struct dev_interface *dev_p)
{
	char *log_header = "dev_send_keepalive_channel";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	int minor = priv_p->p_minor;
	struct block_device *bdev;
	struct nid_device *nid = priv_p->p_nid;
	struct request keepalive_req;

	nid_log_debug("%s: [minor:%d]: ...", log_header, minor);
	bdev = __nid_bdget(minor);
	if (!bdev) {
		nid_log_error("%s: (minor:%d): got null bdev", log_header, minor);
		bdput(bdev);
		return -EINVAL;
	}
	if (!bdev->bd_disk) {
		nid_log_error("%s: [minor:%d]: got null bdev->bd_disk", log_header, minor);
		bdput(bdev);
		return -EINVAL;
	}
	if (!nid) {
		nid_log_error("%s: [minor:%d]: got null nid", log_header, minor);
		bdput(bdev);
		return -EINVAL;
	}

	blk_rq_init(NULL, &keepalive_req);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
	keepalive_req.cmd_type = REQ_TYPE_DRV_PRIV;
#else
	keepalive_req.cmd_type = REQ_TYPE_SPECIAL;
#endif
	nid_cmd(&keepalive_req) = NID_REQ_KEEPALIVE;
	mutex_lock(nid->nid_mutex);
	__nid_send_req(nid, &keepalive_req);
	mutex_unlock(nid->nid_mutex);
	nid->kcounter++;
	nid_log_debug("%s: [minor:%d]: done", log_header, minor);
	bdput(bdev);
	return 0;
}

static struct kobject*
dev_nid_probe(struct dev_interface *dev_p)
{
	char *log_header = "dev_nid_probe";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;
	struct gendisk *disk;
	struct kobject *kobj;

	nid_log_info("%s: start (rcounter:%llu, r_rcounter:%llu, wcounter:%llu, r_wcounter:%llu)...",
		log_header, nid->rcounter, nid->r_rcounter, nid->wcounter, nid->r_wcounter);
	BUG_ON(priv_p->p_disk != nid->disk);
	if (priv_p->p_disk) {
		nid_log_info("%s: disk(minor:%d) existed", log_header, priv_p->p_minor);
		return get_disk(priv_p->p_disk);
	}

	disk = alloc_disk(1 << priv_p->p_part_shift);
	if (!disk) {
		nid_log_error("%s: [minor:%d]: failed to alloc_disk", log_header, priv_p->p_minor);
		return NULL;
	}
	priv_p->p_disk = (void *)disk;
	nid->disk = disk;
	nid_log_notice("%s: alloc_disk: 0x%p, minor:%d\n", log_header, disk, priv_p->p_minor);

	disk->queue = blk_init_queue(__nid_request_handler, &nid_queue_lock);
	if (!disk->queue) {
		put_disk(disk);
		priv_p->p_disk = NULL;
		nid->disk = NULL;
		return NULL;
	}
	nid_log_debug("%s: blk_init_queue: 0x%p\n", log_header, disk->queue);

	nid->nid_queue = disk->queue;

	/* Block device initialize. */
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, disk->queue);
	disk->queue->limits.discard_granularity = 512;
	disk->queue->limits.max_discard_sectors = UINT_MAX;
	disk->queue->limits.discard_zeroes_data = 0;
	blk_queue_max_hw_sectors(disk->queue, 65536);
	disk->queue->limits.max_sectors = 256;

	//nid->dev = dev;
	nid->ctl_file = NULL;
	nid->ctl_sock = NULL;
	nid->data_file = NULL;
	nid->data_sock = NULL;
	nid->magic = NID_MAGIC;
	nid->flags = 0;
	spin_lock_init(&nid->queue_lock);
	INIT_LIST_HEAD(&nid->ctl_snd_queue);
	INIT_LIST_HEAD(&nid->dat_snd_queue);
	INIT_LIST_HEAD(&nid->ctl_rcv_queue);
	INIT_LIST_HEAD(&nid->dat_rcv_queue);
	init_waitqueue_head(&nid->curr_wq);
	init_waitqueue_head(&nid->ctl_snd_wq);
	init_waitqueue_head(&nid->dat_snd_wq);
	init_waitqueue_head(&nid->ctl_rcv_wq);
	init_waitqueue_head(&nid->dat_rcv_wq);
	//nid->blksize = 1024;
	//nid->xmit_timeout = 5 * HZ;
	nid->blksize = 4096;
	nid->bytesize = 0;
	disk->major = NID_MAJOR;
	disk->first_minor = priv_p->p_minor;
	disk->fops = &nid_fops;
	disk->private_data = dev_p;
	sprintf(disk->disk_name, "nid%d", priv_p->p_idx);
	set_capacity(disk, 0);
	kobj = get_disk(disk);
	return kobj;
}

static struct kobject *
dev_get_kobj(struct dev_interface *dev_p)
{
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	BUG_ON(priv_p->p_disk == NULL);
	return get_disk(priv_p->p_disk);
}

static int
dev_get_minor(struct dev_interface *dev_p)
{
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	return priv_p->p_minor;
}

static int
dev_get_stat(struct dev_interface *dev_p, struct nid_message *msg)
{
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct nid_device *nid = priv_p->p_nid;

	memset(msg, 0, sizeof(*msg));
	msg->m_req = NID_REQ_STAT_SAC;

	msg->m_rcounter = nid->rcounter;
	msg->m_has_rcounter = 1;
	msg->m_r_rcounter = nid->r_rcounter;
	msg->m_has_r_rcounter = 1;

	msg->m_wcounter = nid->wcounter;
	msg->m_has_wcounter = 1;
	msg->m_r_wcounter = nid->r_wcounter;
	msg->m_has_r_wcounter = 1;

	msg->m_kcounter = nid->kcounter;
	msg->m_has_kcounter = 1;
	msg->m_r_kcounter = nid->r_kcounter;
	msg->m_has_r_kcounter = 1;

	//total receive message length
	msg->m_recv_sequence = nid->dat_rcv_sequence;
	msg->m_has_recv_sequence = 1;
	//total send message length
	msg->m_wait_sequence = nid->dat_snd_sequence;
	msg->m_has_wait_sequence = 1;

	return 0;
}

static void
dev_cleanup(struct dev_interface *dev_p)
{
	char *log_header = "dev_cleanup";
	struct dev_private *priv_p = (struct dev_private *)dev_p->dv_private;
	struct gendisk *disk = priv_p->p_disk;

	nid_log_notice("%s: start (minor:%d)...", log_header, priv_p->p_minor);
	if (disk) {
		if (disk->flags & GENHD_FL_UP)
			del_gendisk(disk);
		if (disk->queue)
			blk_cleanup_queue(disk->queue);
		put_disk(disk);
	}
	kfree(priv_p->p_nid->req_buf);
	kfree(priv_p->p_nid);
	kfree(priv_p);
	dev_p->dv_private = NULL;
}

static void
dev_drop_request(struct dev_interface *dev_p, struct request *req)
{
	req->errors++;
	__nid_end_io(req);
}

static void
dev_reply_request(struct dev_interface *dev_p, struct request *req)
{
	__nid_end_io(req);
}

static struct dev_operations dev_op = {
	.dv_init_device = dev_init_device,
	.dv_start_channel = dev_start_channel,
	.dv_purge = dev_purge,
	.dv_nid_to_delete = dev_nid_to_delete,
	.dv_nid_set_ioerror = dev_nid_set_ioerror,
	.dv_upgrade_channel = dev_upgrade_channel,
	.dv_recover_channel = dev_recover_channel,
	.dv_send_keepalive_channel = dev_send_keepalive_channel,
	.dv_get_kobj = dev_get_kobj,
	.dv_get_minor = dev_get_minor,
	.dv_get_stat = dev_get_stat,
	.dv_nid_probe = dev_nid_probe,
	.dv_cleanup = dev_cleanup,
	.dv_drop_request = dev_drop_request,
	.dv_reply_request = dev_reply_request,
};

int
dev_initialization(struct dev_interface *dev_p, struct dev_setup *setup)
{
	char *log_header = "dev_initialization";
	struct dev_private *priv_p;
	struct nid_request *req_buf;

	nid_log_notice("%s: start(minor:%d) ...", log_header, setup->minor);
	dev_p->dv_op = &dev_op;
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
	dev_p->dv_private = priv_p;
	priv_p->p_idx = setup->idx;
	priv_p->p_minor = setup->minor;
	priv_p->p_ddg = setup->ddg;
	priv_p->p_part_shift = setup->part_shift;
	priv_p->p_nid = kcalloc(1, sizeof(*priv_p->p_nid), GFP_KERNEL);
	req_buf = kcalloc(max_reqs, sizeof(*req_buf), GFP_KERNEL);
	priv_p->p_nid->req_buf = req_buf;
	priv_p->p_nid->nid_mutex = setup->mutex;
	priv_p->p_nid->disconnect = 1;
	init_waitqueue_head(&priv_p->p_rex_wq);
	return 0;
}
