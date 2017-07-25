/* 
 * device.c
 */

#include "nid.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "dat_if.h"

#define NID_MAX_PART	15

extern int nid_send_req(struct nid_device *nid, struct request *req);
extern int nid_start(struct nid_device *nid, struct block_device *bdev);
extern int nid_stop(struct nid_device *nid);
extern void _sock_shutdown(struct nid_device *nid);
extern struct mpk_interface nid_mpk;

static struct block_device*
_nid_bdget(int minor)
{
	dev_t dev;
	dev = MKDEV(NID_MAJOR, minor);
	return bdget(dev);
}

static void
_nid_end_io(struct request *req)
{
	int error = req->errors ? -EIO : 0;
	struct request_queue *q = req->q;
	unsigned long flags;
	struct nid_device *nid;

	nid = (struct nid_device *)req->rq_disk->private_data;
	if (error) {
		nid_log_warning("_nid_end_io: %s request %p failed", req->rq_disk->disk_name, req);
	} else {
		nid_log_debug("_nid_end_io: %s request %p done", req->rq_disk->disk_name, req);
	}

	spin_lock_irqsave(q->queue_lock, flags);
	__blk_end_request_all(req, error);
	nid->nr_req--;
	spin_unlock_irqrestore(q->queue_lock, flags);
}

static int 
_nid_purge_que(struct nid_device *nid)
{
	struct request *req;

	BUG_ON(nid->magic != NID_MAGIC);

	//BUG_ON(nid->data_sock);
	//BUG_ON(nid->curr_req);
	if (nid->data_sock || nid->curr_req) {
		nid_log_error("_nid_purge_que: EBUSY");
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
		_nid_end_io(req);
	}

	while (!list_empty(&nid->ctl_rcv_queue)) {
		spin_lock(&nid->queue_lock);
		req = list_entry(nid->ctl_rcv_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nid->ctl_rcv_len--;
		spin_unlock(&nid->queue_lock);
		req->errors++;
		_nid_end_io(req);
	}

	while (!list_empty(&nid->dat_snd_queue)) {
		spin_lock(&nid->queue_lock);
		req = list_entry(nid->dat_snd_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nid->dat_snd_len--;
		spin_unlock(&nid->queue_lock);
		req->errors++;
		_nid_end_io(req);
	}

	while (!list_empty(&nid->ctl_snd_queue)) {
		spin_lock(&nid->queue_lock);
		req = list_entry(nid->ctl_snd_queue.next, struct request,
				 queuelist);
		list_del_init(&req->queuelist);
		nid->ctl_snd_len--;
		spin_unlock(&nid->queue_lock);
		req->errors++;
		_nid_end_io(req);
	}

	return 0;
}

static void
_nid_remove(struct nid_device *nid)
{
	nid_log_debug("Enter nid_remove %s", nid->disk->disk_name);  
	if (nid->disk == NULL){
		nid_log_info("nid_remove: disk is NULL for nid %p!\n", nid);
		return;
	}
	if (nid->disk->flags & GENHD_FL_UP){
		nid_log_info("nid_remove: del_gendisk: 0x%p", nid->disk);
		del_gendisk(nid->disk);
	}
	if (nid->disk->queue){
		nid_log_info("nid_remove: cleanup_queue: 0x%p", nid->disk->queue);
		blk_cleanup_queue(nid->disk->queue);
	}
	put_disk(nid->disk);
	nid->disk = NULL;
}

static void
_driver_to_recover(struct nid_device *nid)
{
       	char *msg_buf;
	uint32_t len = 0;
	struct nid_message nid_msg;

	nid_log_debug("_driver_to_recover: %d", MINOR(nid->dev));	
	msg_buf = kmalloc(128, GFP_KERNEL);
	len = 128;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_RECOVER;
	nid_msg.m_devminor = MINOR(nid->dev);
	nid_msg.m_has_devminor = 1;
	nid_mpk.m_op->m_encode(&nid_mpk, msg_buf, &len, &nid_msg); 
	nid_driver_send_msg(msg_buf, len);
}

static void
_driver_established(struct nid_device *nid)
{
       	char *msg_buf;
	uint32_t len = 0;
	struct nid_message nid_msg;

	nid_log_error("_driver_established: %s", nid->disk->disk_name);	
	msg_buf = kmalloc(128, GFP_KERNEL);
	len = 128;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHAN_ESTABLISHED;
	nid_msg.m_devminor = MINOR(nid->dev);
	nid_msg.m_has_devminor = 1;
	nid_mpk.m_op->m_encode(&nid_mpk, msg_buf, &len, &nid_msg); 
	nid_driver_send_msg(msg_buf, len);
}

static void
_device_deleted(int minor)
{
       	char *msg_buf;
	uint32_t len = 0;
	struct nid_message nid_msg;

	nid_log_debug("_device_deleted: minor:%d", minor);	
	msg_buf = kmalloc(128, GFP_KERNEL);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_DELETE_DEVICE;
	nid_msg.m_devminor = MINOR(minor);
	nid_msg.m_has_devminor = 1;
	len = 128;
	nid_mpk.m_op->m_encode(&nid_mpk, msg_buf, &len, &nid_msg); 
	nid_driver_send_msg(msg_buf, len);
}

static int
_nid_recover(struct nid_device *nid, struct block_device *bdev)
{
	int error = 0;

	nid_log_debug("_nid_recover: %s", nid->disk->disk_name);

	nid_stop(nid);

	mutex_lock(&nid->nid_mutex);
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

	mutex_unlock(&nid->nid_mutex);
	_driver_to_recover(nid);
	error = nid->harderror;
	return error;
}

static int
_nid_disconnect(struct nid_device *nid, struct block_device *bdev)
{
	char *log_header = "_nid_disconnect";
#if 0	
	if (!nid->reg_done) {
		nid->harderror = -EBADR;
		nid_log(LOG_IOCTL, "__nid_ioctl: case NID_STOP. nid_async_reg not finished yet even after nid_start\n");
	}

	if (nid->disconnect || nid->harderror == -EBADR) {
		need_exit = 1;
	}
#endif

	nid_stop(nid);

	mutex_lock(&nid->nid_mutex);

	if (nid->inited == 0) {
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
	_nid_purge_que(nid);

	kill_bdev(bdev);
	queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, nid->disk->queue);
	set_device_ro(bdev, false);
	nid->flags = 0;
	nid->bytesize = 0;
	bdev->bd_inode->i_size = 0;
	set_capacity(nid->disk, 0);

	nid_log_notice("%s: calling cancel_work_sync", log_header);
	cancel_work_sync(&nid->work);	// in case the job not finished yet
	nid_log_notice("%s: after cancel_work_sync", log_header);
	_nid_remove(nid);

	if (NID_MAX_PART > 0)
		ioctl_by_bdev(bdev, BLKRRPART, 0);

	mutex_unlock(&nid->nid_mutex);
	_device_deleted(MINOR(nid->dev));
	return 0;
}

int 
nid_device_init(int minor, uint64_t size, uint32_t blksize)
{
	struct block_device *bdev;
	struct nid_device *nid;

	nid_log_error("nid_device_init (minor:%d, size:%llu, blksize:%u): start ...",
		minor, size, blksize);
	bdev = _nid_bdget(minor);
	if (!bdev) {
		nid_log_error("nid_device_init (minor:%d): got null bdev", minor);
		bdput(bdev);
		return -EINVAL;
	}
	if (!bdev->bd_disk) {
		nid_log_error("nid_device_init (minor:%d): got null bdev->bd_disk", minor);
		bdput(bdev);
		return -EINVAL;
	}
	nid = bdev->bd_disk->private_data;
	if (!nid) {
		nid_log_error("nid_device_init (minor:%d): got null nid", minor);
		bdput(bdev);
		return -EINVAL;
	}

	nid_log_error("nid_device_init (minor:%d): step 1", minor);

	nid->blksize = blksize;
	nid->bytesize = size;	
	bdev->bd_inode->i_size = nid->bytesize;
	set_blocksize(bdev, nid->blksize);
	set_capacity(nid->disk, nid->bytesize >> 9);
	nid->error_now = 0;
	bdput(bdev);
	return 0;
}


static int
_nid_device_set_sock(struct nid_device *nid, struct block_device *bdev, int rsfd)
{
	struct gendisk *disk = nid->disk;
	struct file *file;
	int rc = 0;

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
_nid_device_set_dsock(struct nid_device *nid, struct block_device *bdev, int dsfd)
{
	struct gendisk *disk = nid->disk;
	struct file *data_file;
	int rc = 0;

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

int 
nid_device_start(int minor, int rsfd, int dsfd)
{
	struct block_device *bdev;
	struct nid_device *nid;
	int error;

	nid_log_error("nid_device_start (minor:%d, rsfd:%d, dsfd:%d): start...", minor, rsfd, dsfd);
	bdev = _nid_bdget(minor);
	if (!bdev) {
		nid_log_error("nid_device_start (minor:%d): got null bdev", minor);
		//bdput(bdev);
		return -EINVAL;
	}
	if (!bdev->bd_disk) {
		nid_log_error("nid_device_start (minor:%d): got null bdev->bd_disk", minor);
		bdput(bdev);
		return -EINVAL;
	}
	nid = bdev->bd_disk->private_data;
	if (!nid) {
		nid_log_error("nid_device_start (minor:%d): got null nid", minor);
		bdput(bdev);
		return -EINVAL;
	}

	kill_bdev(bdev);
	mutex_lock(&nid->nid_mutex);
	_nid_device_set_sock(nid, bdev, rsfd);
	_nid_device_set_dsock(nid, bdev, dsfd);
	error = nid_start(nid, bdev);
	mutex_unlock(&nid->nid_mutex);
	_driver_established(nid);
	bdput(bdev);
	return 0;
}

void
nid_device_set_ioerror(int minor)
{
	char *log_header = "nid_device_set_ioerror";
	struct block_device *bdev;
	struct nid_device *nid;
	struct gendisk *disk;
       	char *msg_buf;
	uint32_t len = 0;
	struct nid_message nid_msg;

	bdev = _nid_bdget(minor);
	if (!bdev) {
		nid_log_error("%s: no bdev, minor:%d", log_header, minor);
		bdput(bdev);
		goto send_msg;
	}
	nid = bdev->bd_disk->private_data;
	if (!nid) {
		nid_log_error("%s: no nid, minor:%d", log_header, minor);
		bdput(bdev);
		goto send_msg;
	}
	disk = nid->disk;

	spin_lock(&nid->queue_lock);
	nid->error_now = 1;
	spin_unlock(&nid->queue_lock);
	/* error_now set, no more new requests comming in. */
	_nid_purge_que(nid);

	nid_log_debug("%s: done, minor:%d", log_header, minor);

send_msg:
	msg_buf = kmalloc(128, GFP_KERNEL);
	len = 128;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_IOERROR_DEVICE;
	nid_msg.m_devminor = minor;
	nid_msg.m_has_devminor = 1;
	nid_mpk.m_op->m_encode(&nid_mpk, msg_buf, &len, &nid_msg); 
	nid_driver_send_msg(msg_buf, len);
	bdput(bdev);
	return;
}

/*
 * Got recover request from the agent
 */
void
nid_device_to_recover(int minor)
{
	struct block_device *bdev;
	struct nid_device *nid;

	nid_log_debug("nid_device_to_recover: minor:%d", minor);
	bdev = _nid_bdget(minor);
	if (!bdev || !bdev->bd_disk || !(nid = bdev->bd_disk->private_data)) {
		nid_log_debug("nid_device_to_recover: wrong device, minor:%d", minor);
		bdput(bdev);
		return;
	}
	_sock_shutdown(nid);
	bdput(bdev);
	return;
}

void
nid_device_to_delete(int minor)
{
	struct block_device *bdev;
	struct nid_device *nid;
	struct gendisk *disk;
	struct request sreq;

	bdev = _nid_bdget(minor);
	if (!bdev || !bdev->bd_disk || !(nid = bdev->bd_disk->private_data)) {
		_device_deleted(minor);
		bdput(bdev);
		return;
	}
	disk = nid->disk;

	if (!nid->data_sock) {
		/*TODO: cleanup all threads. */
		nid_log_error("nid_device_to_delete[minor:%d]: deleting disconnected nid.", minor);
		nid->disconnect = 1;
		/* nid_ctl_receiver is not there, we must cleanup by ourself. */
		nid_device_stop(nid);
		bdput(bdev);
		return;
	}
	fsync_bdev(bdev);

	blk_rq_init(NULL, &sreq);
	sreq.cmd_type = REQ_TYPE_SPECIAL;
	nid_cmd(&sreq) = NID_REQ_DELETE_DEVICE;
	mutex_lock(&nid->nid_mutex);
	if (!nid->data_sock) {
		nid->disconnect = 1;
		mutex_unlock(&nid->nid_mutex);
		bdput(bdev);
		return;
	}
	nid->disconnect = 1;
	nid_send_req(nid, &sreq);
	mutex_unlock(&nid->nid_mutex);
	bdput(bdev);
	return;
}

int 
nid_device_send_keepalive(int minor)
{
	struct block_device *bdev;
	struct nid_device *nid;
	struct request keepalive_req;

	nid_log_debug("nid_device_send_keepalive[minor:%d]: ...", minor);
	bdev = _nid_bdget(minor);
	if (!bdev) {
		nid_log_error("nid_device_send_keepalive (minor:%d): got null bdev", minor);
		bdput(bdev);
		return -EINVAL;
	}
	if (!bdev->bd_disk) {
		nid_log_error("nid_device_send_keepalive [minor:%d]: got null bdev->bd_disk", minor);
		bdput(bdev);
		return -EINVAL;
	}
	nid = bdev->bd_disk->private_data;
	if (!nid) {
		nid_log_error("nid_device_send_keepalive [minor:%d]: got null nid", minor);
		bdput(bdev);
		return -EINVAL;
	}

	blk_rq_init(NULL, &keepalive_req);
	keepalive_req.cmd_type = REQ_TYPE_SPECIAL;
	nid_cmd(&keepalive_req) = NID_REQ_KEEPALIVE;
	mutex_lock(&nid->nid_mutex);
	nid_send_req(nid, &keepalive_req);
	mutex_unlock(&nid->nid_mutex);
	nid->kcounter++;
	nid_log_debug("nid_device_send_keepalive[minor:%d]: done", minor);
	bdput(bdev);
	return 0;
}

int
nid_device_stop(struct nid_device *nid)
{
	char *log_header = "nid_device_stop";
	struct block_device *bdev = NULL;
	int need_exit = 0;

	if (!nid->disk) {
		nid_log_notice("%s: %s has null disk...", log_header, nid->disk->disk_name);
		need_exit = 1;
		goto out;
	}

	nid_log_notice("%s: %s", log_header, nid->disk->disk_name);
	bdev = _nid_bdget(MINOR(disk_devt(nid->disk)));
	if (!bdev) {
		nid_log_debug("%s: %s has null bdev...", log_header, nid->disk->disk_name);
		need_exit = 1;
		goto out;
	}

	if (nid->disconnect) {
		nid_log_debug("%s: disconnecting %s ...", log_header, nid->disk->disk_name);
		_nid_disconnect(nid, bdev);
		need_exit = 1;
	} else {
		nid_log_debug("%s: recovering %s...", log_header, nid->disk->disk_name);
		_nid_recover(nid, bdev);
		need_exit = 0;
	}

out:
	if (bdev != NULL) {
		bdput(bdev);
	}
	return need_exit;
}

void
nid_device_recv_keepalive(struct nid_device *nid)
{
       	char *msg_buf;
	uint32_t len = 0;
	struct nid_message nid_msg;

	nid_log_debug("driver_keepalive_recv: got keepalive from nidservice for %s", nid->disk->disk_name);	
	msg_buf = kmalloc(128, GFP_KERNEL);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_KEEPALIVE;
	nid_msg.m_devminor = MINOR(nid->dev);
	nid_msg.m_has_devminor = 1;
	len = 128;
	nid_mpk.m_op->m_encode(&nid_mpk, msg_buf, &len, &nid_msg); 
	nid_driver_send_msg(msg_buf, len);
}

int 
nid_device_send_upgrade(int minor, char force)
{
	struct block_device *bdev;
	struct nid_device *nid;
	struct request req;

	nid_log_debug("nid_device_send_upgrade[minor:%d]: ...", minor);
	bdev = _nid_bdget(minor);
	if (!bdev) {
		nid_log_error("nid_device_send_upgrade (minor:%d): got null bdev", minor);
		bdput(bdev);
		return -EINVAL;
	}
	if (!bdev->bd_disk) {
		nid_log_error("nid_device_send_upgrade [minor:%d]: got null bdev->bd_disk", minor);
		bdput(bdev);
		return -EINVAL;
	}
	nid = bdev->bd_disk->private_data;
	if (!nid) {
		nid_log_error("nid_device_send_upgrade [minor:%d]: got null nid", minor);
		bdput(bdev);
		return -EINVAL;
	}

	blk_rq_init(NULL, &req);
	req.cmd_type = REQ_TYPE_SPECIAL;
	if (force)
		nid_cmd(&req) = NID_REQ_FUPGRADE;
	else
		nid_cmd(&req) = NID_REQ_UPGRADE;

	mutex_lock(&nid->nid_mutex);
	nid_send_req(nid, &req);
	mutex_unlock(&nid->nid_mutex);
	nid_log_debug("nid_device_send_upgrade[minor:%d]: done", minor);
	bdput(bdev);
	return 0;
}

void
nid_device_recv_upgrade(struct nid_device *nid, char code)
{
       	char *msg_buf;
	uint32_t len = 0;
	struct nid_message nid_msg;

	nid_log_debug("nid_device_recv_upgrade for %s, code: %d", nid->disk->disk_name, code);
	msg_buf = kmalloc(128, GFP_KERNEL);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_UPGRADE;
	nid_msg.m_devminor = MINOR(nid->dev);
	nid_msg.m_has_devminor = 1;
	nid_msg.m_code = code;
	nid_msg.m_has_code = 1;
	len = 128;
	nid_mpk.m_op->m_encode(&nid_mpk, msg_buf, &len, &nid_msg); 
	nid_driver_send_msg(msg_buf, len);
}

