#include "nid.h"
#include "nid_shared.h"
#include "nid_log.h"
#include "version.h"
#include <linux/version.h>

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

extern unsigned int nids_max;
extern struct nid_device *nid_dev;

static void nid_async_reg(struct work_struct *work);
extern void nid_end_io(struct request *req);
void _sock_shutdown(struct nid_device *nid);

static void nid_sock_timeout(unsigned long arg)
{
	struct task_struct *task = (struct task_struct *)arg;

	nid_log(LOG_IOCTL, "nid: killing hung io (%s, pid: %d)\n",
		task->comm, task->pid);
	force_sig(SIGKILL, task);
}

static int sock_rw(struct nid_device *nid, int req, int send, void *buf, int size,
		int msg_flags)
{
	struct socket *sock = req ? nid->ctl_sock : nid->data_sock;
	int result;
	struct msghdr msg;
	struct kvec iov;
	sigset_t blocked, oldset;
	unsigned long pflags = current->flags;

	if (unlikely(!sock)) {
		dev_err(disk_to_dev(nid->disk),
			"Attempted %s on closed socket in sock_rw\n",
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
				ti.function = nid_sock_timeout;
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

static inline int sock_recv_bio(struct nid_device *nid, struct bio_vec *bvec)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	result = sock_rw(nid, 0, 0, kaddr + bvec->bv_offset, bvec->bv_len,
			MSG_WAITALL);
	kunmap(bvec->bv_page);
	return result;
}

static inline int sock_send_bio(struct nid_device *nid, struct bio_vec *bvec,
		int flags)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	result = sock_rw(nid, 0, 1, kaddr + bvec->bv_offset,
			   bvec->bv_len, flags);
	kunmap(bvec->bv_page);
	return result;
}

static const char *nidcmd_to_str(int cmd)
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
static int nid_fill_req_header(struct nid_device *nid, struct request *req, struct nid_request *ireq)
{
	u32 size = blk_rq_bytes(req);

	if (req->cmd_type != REQ_TYPE_FS) {
		return -EINVAL;
	} /* REQ_TYPE_SPECIAL */

	nid_cmd(req) = NID_REQ_READ;
	if (rq_data_dir(req) == WRITE) {
		/*
		if ((req->cmd_flags & REQ_DISCARD)) {
			WARN_ON(!(nid->flags & NID_FLAG_SEND_TRIM));
			nid_cmd(req) = NID_REQ_TRIM;
		} else
			nid_cmd(req) = NID_REQ_WRITE;
		*/
		nid_cmd(req) = NID_REQ_WRITE;

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

	if (nid_cmd(req) == NID_REQ_FLUSH) {
		/* Other values are reserved for FLUSH ireqs.  */
		ireq->len = 0;
		nid_log(LOG_IOCTL, "nid_fill_req_header: not support NID_REQ_FLUSH yet");
		BUG();
	} else {
		ireq->offset = cpu_to_be64((u64)blk_rq_pos(req) << 9);
		ireq->len = htonl(size);
		if (nid_cmd(req) == NID_REQ_WRITE) {
			ireq->dseq = cpu_to_be64(nid->sequence);
			nid->sequence += size; 
		}
	}
	memcpy(ireq->handle, &req, sizeof(req));

	nid_log(LOG_TX, "%s: ireq %p: preparing control (%s@%llu,%uB) %llu\n",
			nid->disk->disk_name, req,
			nidcmd_to_str(nid_cmd(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req),
			be64_to_cpu(ireq->dseq));
	return 0;
}

static void nid_dump_queues(struct nid_device *nid)
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

static struct request *nid_find_request(struct nid_device *nid,
					struct request *xreq)
{
	struct request *req, *tmp;
	int err;

	err = 0;
	while (nid->curr_req == xreq) {
		err = wait_event_interruptible(nid->curr_wq, nid->curr_req != xreq);
	}
	if (unlikely(err)) {
		nid_log(LOG_IOCTL, "%s: nid_find_request wait_event_interruptible err:%d\n",
			nid->disk->disk_name, err);		
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

	nid_log(LOG_IOCTL, "%s:nid_find_request failed to find request(%p) in ctl_rcv_queue, ctl_rcv_len:%d, ctl_snd_len:%d\n",
		nid->disk->disk_name, xreq, nid->ctl_rcv_len, nid->ctl_snd_len);
	nid_dump_queues(nid);
	spin_unlock(&nid->queue_lock);
	err = -ENOENT;

out:
	return ERR_PTR(err);
}

static int nid_read_resp(struct nid_device *nid)
{
	int result;
	struct nid_request reply;
	struct request *req;

next:
	result = sock_rw(nid, 1, 0, &reply, sizeof(reply), MSG_WAITALL);
	if (result <= 0) {
		dev_err(disk_to_dev(nid->disk),
			"Receive control failed (result %d)\n", result);
		goto harderror;
	}

	/* Keepalive handling, Notify user agent. */
	if (reply.cmd == NID_REQ_KEEPALIVE) {
		nid->last_keepalive = jiffies;
		nid_device_recv_keepalive(nid);
		nid->r_kcounter++;
		goto next;
	} else if (reply.cmd == NID_REQ_UPGRADE || reply.cmd == NID_REQ_FUPGRADE) {
		nid_log_debug("nid_read_resp() cmd(7:U, 8:F): %d", reply.cmd);
		nid_device_recv_upgrade(nid, reply.code);
		goto next;
	} else if (reply.cmd == NID_REQ_WRITE) {
		nid->r_wcounter++;
	}

	req = nid_find_request(nid, *(struct request **)reply.handle);
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
		nid_end_io(req);
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
	nid_end_io(req);
	return 0;
harderror:
	nid->harderror = result;
	return 1;
}

static int nid_ctl_receiver(void *data)
{
	struct nid_device *nid = data;
	struct gendisk *disk = nid->disk;
	int rc = 0;

	BUG_ON(nid->magic != NID_MAGIC);
	nid_log(LOG_IOCTL, "nid_dev:%p", nid);
	BUG_ON(nid->data_sock == NULL);

	sk_set_memalloc(nid->data_sock->sk);
	nid->pid = task_pid_nr(current);

	nid_log(LOG_IOCTL, "%s: enter nid_ctl_receiver. \n", disk->disk_name);
	set_user_nice(current, -20);

	while (nid_read_resp(nid) == 0) {
		;
	}

	rc = nid_device_stop(nid);
	if (rc != 0) {
		nid_log_info("nid_ctl_receiver(%s): disconnecting ...", disk->disk_name);
	}

	nid_log(LOG_IOCTL, "%s: leave nid_ctl_receiver. \n", disk->disk_name);

	nid->pid = 0;
	return 0;
}

static int nid_dat_receiver(void *data)
{
	struct request *req;
	struct nid_device *nid = data;
	struct gendisk *disk = nid->disk;
	int result;

	BUG_ON(nid->magic != NID_MAGIC);
	nid_log(LOG_IOCTL, "nid_dev:%p", nid);
	BUG_ON(nid->data_sock == NULL);

	nid_log(LOG_IOCTL, "%s: enter nid_dat_receiver. \n", disk->disk_name);
	set_user_nice(current, -19);

	while (!kthread_should_stop()) {
		int rc = 0;

		/* wait for something to do, timeout in 10 secs */
		rc = wait_event_interruptible_timeout(nid->dat_rcv_wq,
			kthread_should_stop() || !list_empty(&nid->dat_rcv_queue),
			NID_REQUEST_WAIT);

		if (kthread_should_stop()) {
			nid_log_info("nid_dat_receiver(%s): stop ...", disk->disk_name);
			break;
		}
	
		if (!nid->data_sock) {
			nid_log_info("nid_dat_receiver(%s): Error, no data_sock...", disk->disk_name);
			continue;
		}

		/* extract request */
		spin_lock(&nid->queue_lock);
		while(!list_empty(&nid->dat_rcv_queue)) {
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
				result = sock_recv_bio(nid, &bvec);
#else
				result = sock_recv_bio(nid, bvec);
#endif
				if (result <= 0) {
					dev_err(disk_to_dev(nid->disk), "Receive data failed (result %d)\n",
						result);

					spin_lock(&nid->queue_lock);

					list_add(&req->queuelist, &nid->dat_rcv_queue);
					nid->dat_rcv_len++;
					nid->dat_rcv_len_total++;

					spin_unlock(&nid->queue_lock);

					/* Need recovery, wait to get stopped. */
					_sock_shutdown(nid);
					wait_event_interruptible(nid->dat_rcv_wq, kthread_should_stop());
					goto error_out;
				}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
				nid_log(LOG_RX, "%s: request %p: got %d bytes data\n",
					nid->disk->disk_name, req, bvec.bv_len);
#else
				nid_log(LOG_RX, "%s: request %p: got %d bytes data\n",
					nid->disk->disk_name, req, bvec->bv_len);
#endif
			}
			nid_end_io(req);

			spin_lock(&nid->queue_lock);
		}
		spin_unlock(&nid->queue_lock);
	}

error_out:
	nid_log(LOG_IOCTL, "%s: leave nid_dat_receiver. \n", disk->disk_name);

	return 0;
}

static int nid_send_req_data(struct nid_device *nid, struct request *req)
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
			result = sock_send_bio(nid, &bvec, flags);
#else
			if (!rq_iter_last(req, iter))
				flags = MSG_MORE;
			nid_log(LOG_TX, "%s: request %p: sending %d bytes data\n",
					nid->disk->disk_name, req, bvec->bv_len);
			dsize += bvec->bv_len;
			BUG_ON(dsize > size);
			result = sock_send_bio(nid, bvec, flags);
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
int nid_send_req(struct nid_device *nid, struct request *req)
{
	int result;
	struct nid_request request;
	u32 size = blk_rq_bytes(req);

	request.magic = htons(NID_REQUEST_MAGIC);
	request.cmd = nid_cmd(req);

	if (nid_cmd(req) == NID_REQ_FLUSH) {
		/* Other values are reserved for FLUSH requests.  */
		request.len = 0;
		nid_log(LOG_IOCTL, "nid_send_req: not support NID_REQ_FLUSH yet");
		BUG();
	} else {
		request.offset = cpu_to_be64((u64)blk_rq_pos(req) << 9);
		request.len = htonl(size);
		if (nid_cmd(req) == NID_REQ_WRITE) {
			// nid_log(LOG_IOCTL, "nid_send_req: NID_REQ_WRITE sequence:%lu\n", nid->sequence);
			request.dseq = cpu_to_be64(nid->sequence);
		}
	}
	memcpy(request.handle, &req, sizeof(req));

	nid_log(LOG_TX, "%s: request %p: sending control (%s@%llu,%uB)\n",
			nid->disk->disk_name, req,
			nidcmd_to_str(nid_cmd(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req));

	result = sock_rw(nid, 1, 1, &request, sizeof(request), 0);
	if (result <= 0) {
		dev_err(disk_to_dev(nid->disk),
			"Send control failed (result %d)\n", result);
		goto error_out;
	}

	return 0;

error_out:
	return -EIO;
}

static void nid_handle_req_list(struct nid_device *nid, struct list_head *working_queue, int nr_reqs)
{
	struct request *req, *next_req;
	int result;
	int nr_send = 0, nr_wr = 0, nr_rd = 0;
	struct nid_request *req_buf = nid->req_buf;
	LIST_HEAD(wr_queue);
	int hsize = 0;

	list_for_each_entry_safe(req, next_req, working_queue, queuelist) {
		result = nid_fill_req_header(nid, req, &req_buf[nr_send]);
		if (result != 0) {
			/* Drop bad requests directly. */
			nid_log(LOG_IOCTL, "%s: drop bad requests %p.\n", nid->disk->disk_name, req);
			nid->nr_bad_req_total++;
			list_del_init(&req->queuelist);
			req->errors++;
			nid_end_io(req);
			continue;
		}
		if (nid_cmd(req) == NID_REQ_WRITE) {
			hsize += blk_rq_bytes(req);
			nr_wr++;
			/* WRITE Order is critical. */
			list_move_tail(&req->queuelist, &wr_queue);
			nid->wcounter++;
		} else {
			/* READ and CTL requests are counted the same way. */
			nr_rd++;
			if (nid_cmd(req) == NID_REQ_READ)
				nid->rcounter++;
		}
		nr_send++;
	}

	if (nr_send == 0) {
		nid_log(LOG_IOCTL, "%s: zero good requests: nr_wr: %d, nr_rd: %d.\n", nid->disk->disk_name, nr_wr, nr_rd);
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

	spin_unlock(&nid->queue_lock);

	mutex_lock(&nid->nid_mutex);

	if (unlikely(!nid->data_sock)) {
		mutex_unlock(&nid->nid_mutex);
		dev_err(disk_to_dev(nid->disk),
			"Attempted send on closed socket, req equeued\n");
		goto error_out;
	}

	/* NOTICE: The 'request' data structure of a read command can be 
	 * finished and get reused immediatelly after it's header send out.
	 */
	result = sock_rw(nid, 1, 1, req_buf, sizeof(struct nid_request) * nr_send, 0);
	if (result <= 0) {
		mutex_unlock(&nid->nid_mutex);
		dev_err(disk_to_dev(nid->disk),
			"Send control failed (result %d)\n", result);
		goto error_out;
	}

	wake_up(&nid->dat_snd_wq);

	mutex_unlock(&nid->nid_mutex);

	return;

error_out:
	/* Leave requests at ctl_rcv_queue / dat_snd_queue. */
	/* They will be moved back to when recover finished. */

	return;
}

static int nid_dat_sender(void *data)
{
	struct nid_device *nid = data;
	struct gendisk *disk = nid->disk;
	struct request *req;
	int result;

	nid_log(LOG_IOCTL, "%s: enter nid_dat_sender. \n", disk->disk_name);

	set_user_nice(current, -19);
	while (!kthread_should_stop()) {
		int rc = 0;

		/* wait for something to do, timeout in 10 secs */
		rc = wait_event_interruptible_timeout(nid->dat_snd_wq,
			kthread_should_stop() || !list_empty(&nid->dat_snd_queue),
			NID_REQUEST_WAIT);

		if (kthread_should_stop()) {
			nid_log_info("nid_dat_sender(%s): stop ...", disk->disk_name);
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

			result = nid_send_req_data(nid, req);

			if (result < 0) {
				/* Socket error. Need retry. */
				_sock_shutdown(nid);
				nid_log_info("nid_dat_sender(%s): Send data error...", disk->disk_name);
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
		result = nid_send_req_data(nid, ireq);
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
	nid_log(LOG_IOCTL, "%s: leave nid_dat_sender. \n", disk->disk_name);

	return 0;
}

static int nid_ctl_sender(void *data)
{
	int nr_reqs;
	struct nid_device *nid = data;
	struct gendisk *disk = nid->disk;
	struct request_queue *queue = disk->queue;
	LIST_HEAD(working_queue);

	nid_log(LOG_IOCTL, "%s: enter nid_ctl_sender. rq_timeout:%u, nr_requests:%lu\n",
		disk->disk_name, queue->rq_timeout, queue->nr_requests);

	set_user_nice(current, -20);
	while (!kthread_should_stop()) {
		int rc = 0;

		/* wait for something to do, timeout in 10 secs */
		rc = wait_event_interruptible_timeout(nid->ctl_snd_wq,
			kthread_should_stop() || !list_empty(&nid->ctl_snd_queue),
			NID_REQUEST_WAIT);

		if (kthread_should_stop()) {
			nid_log_info("nid_ctl_sender(%s): stop ...", disk->disk_name);
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

		nid_handle_req_list(nid, &working_queue, nr_reqs);
	}
	/* The nid_purge_que() after kthread_stop() will cleanup any pending reqs. */
	nid_log(LOG_IOCTL, "%s: leave nid_ctl_sender. \n", disk->disk_name);
	return 0;
}

int nid_start_threads(struct nid_device *nid)
{
	int error = 0;

	nid->nid_ctl_snd_thread = kthread_create(nid_ctl_sender, nid, "%s", nid->disk->disk_name);
	if (IS_ERR(nid->nid_ctl_snd_thread)) {
		struct task_struct *thread = nid->nid_ctl_snd_thread;
		nid_log(LOG_IOCTL, "%s: nid_start: cannot create ctl_sender\n", nid->disk->disk_name);
		nid->nid_ctl_snd_thread = NULL;
		error = PTR_ERR(thread);
		goto error_out;
	}
	nid_log_info("nid_start(%s): wake_up ctl_sender ...", nid->disk->disk_name);
	wake_up_process(nid->nid_ctl_snd_thread);

	nid->nid_dat_snd_thread = kthread_create(nid_dat_sender, nid, "%sds", nid->disk->disk_name);
	if (IS_ERR(nid->nid_dat_snd_thread)) {
		struct task_struct *thread = nid->nid_dat_snd_thread;
		nid_log(LOG_IOCTL, "%s: nid_start: cannot create dat_sender\n", nid->disk->disk_name);
		nid->nid_dat_snd_thread = NULL;
		error = PTR_ERR(thread);
		goto error_out;
	}
	nid_log_info("nid_start(%s): wake_up dat_sender ...", nid->disk->disk_name);
	wake_up_process(nid->nid_dat_snd_thread);

	nid->nid_ctl_rcv_thread = kthread_create(nid_ctl_receiver, nid, "%scr", nid->disk->disk_name);
	if (IS_ERR(nid->nid_ctl_rcv_thread)) {
		struct task_struct *thread = nid->nid_ctl_rcv_thread;
		nid_log(LOG_IOCTL, "%s: nid_start: cannot create ctl_receiver\n", nid->disk->disk_name);
		nid->nid_ctl_rcv_thread = NULL;
		error = PTR_ERR(thread);
		goto error_out;
	}
	nid_log_info("nid_start(%s): wake_up ctl_receiver...", nid->disk->disk_name);
	wake_up_process(nid->nid_ctl_rcv_thread);

	nid->nid_dat_rcv_thread = kthread_create(nid_dat_receiver, nid, "%sdr", nid->disk->disk_name);
	if (IS_ERR(nid->nid_dat_rcv_thread)) {
		struct task_struct *thread = nid->nid_dat_rcv_thread;
		nid_log(LOG_IOCTL, "%s: nid_start: cannot create dat_receiver\n", nid->disk->disk_name);
		nid->nid_dat_rcv_thread = NULL;
		error = PTR_ERR(thread);
		goto error_out;
	}
	nid_log_info("nid_start(%s): wake_up dat_receiver...", nid->disk->disk_name);
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

/* Start worker threads when sockets are ready. */
/* Always call with nid_mutex locked. */
int nid_start(struct nid_device *nid, struct block_device *bdev)
{
	struct gendisk *disk = nid->disk;
	int error = 0;

	nid_log_error("nid_start(%s): start ...", disk->disk_name);
	if (nid->inited) {
		/* this must be persistent mode, there could be some requests waiting to retry */	
		nid_log(LOG_IOCTL, "%s: nid_start: re-hang\n", disk->disk_name);
		nid->error_now = 0;
		error = nid_start_threads(nid);
		return error;
	}
 
	/*
	 * Only do these setup at the first time
	 */
	if (nid->flags & NID_FLAG_READ_ONLY)
		set_device_ro(bdev, true);
	/*
	if (nid->flags & NID_FLAG_SEND_TRIM)
		queue_flag_set_unlocked(QUEUE_FLAG_DISCARD,
			nid->disk->queue);
	if (nid->flags & NID_FLAG_SEND_FLUSH)
		blk_queue_flush(nid->disk->queue, REQ_FLUSH);
	else
		blk_queue_flush(nid->disk->queue, 0);
	*/
	blk_queue_flush(nid->disk->queue, 0);

	error = nid_start_threads(nid);
	if (error != 0) {
		goto error_out;
	}

	nid->reg_done = 0;
	nid->cntl_req_done = 0;
	nid->cntl_req = NULL;
	nid->nr_req = 0;
	INIT_WORK(&nid->work, nid_async_reg);
	schedule_work(&nid->work);
	nid->inited = 1;

	return 0;

error_out:
	return error;
}

int nid_stop_threads(struct nid_device *nid)
{
	/* nid_ctl_receiver stop all other threads but itself. */
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

void
_sock_shutdown(struct nid_device *nid)
{
	/*
	 * Forcibly shutdown the socket causing all listeners to error
	 * and let the nid_ctl_receiver do cleanup work.
	 */
	if (nid->ctl_sock) {
		nid_log_warning("shutting down ctl socket\n");
		kernel_sock_shutdown(nid->ctl_sock, SHUT_RDWR);
	}
	if (nid->data_sock) {
		nid_log_warning("shutting down data socket\n");
		kernel_sock_shutdown(nid->data_sock, SHUT_RDWR);
	}
}

static void
_sock_cleanup(struct nid_device *nid)
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
nid_stat_cleanup(struct nid_device *nid)
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

int nid_stop(struct nid_device *nid)
{
	mutex_lock(&nid->nid_mutex);
	_sock_shutdown(nid);
	mutex_unlock(&nid->nid_mutex);

	nid_stop_threads(nid);

	mutex_lock(&nid->nid_mutex);
	_sock_cleanup(nid);
	nid_stat_cleanup(nid);
	mutex_unlock(&nid->nid_mutex);

	return 0;
}

static void nid_async_reg(struct work_struct *work)
{
	struct nid_device *nid = container_of(work, struct nid_device, work);

	nid_log(LOG_IOCTL, "Enter nid_async_reg\n");  
	/*
	 * add_disk() will generate IO on the device. 
	 * Must be called WITHOUT nid_mutex to avoid deadlock with nid_thread.
	 */
	nid_log(LOG_IOCTL, "nid_async_reg: add_disk: 0x%p\n", nid->disk);
	add_disk(nid->disk);
	nid_log(LOG_IOCTL, "nid_async_reg succeed!\n");
	nid->reg_done = 1;
	dev_notice(disk_to_dev(nid->disk), "register nid succeed!\n");
	return;
}
