/*
 *
 */
#include "nid.h"
#include "nid_shared.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "dev_if.h"
#include "drv_if.h"
#include "version.h"
#include <linux/version.h>

//static int max_reqs = 1024;
static int max_part = 15;
static int part_shift;

//struct nid_device *nid_dev;
unsigned int nids_max = 256;
int nid_log_level;

//static DEFINE_SPINLOCK(nid_queue_lock);

static struct drv_setup drv_setup;
struct drv_interface *nid_drv_p;

#if 0
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
void nid_end_io(struct request *req)
{
	int error = req->errors ? -EIO : 0;
	struct request_queue *q = req->q;
	unsigned long flags;
	struct nid_device *nid;

	nid = (struct nid_device *)req->rq_disk->private_data;
	if (error) {
		nid_log_warning("nid_end_io: %s request %p failed", req->rq_disk->disk_name, req);
	} else {
		nid_log_debug("nid_end_io: %s request %p done", req->rq_disk->disk_name, req);
	}
	spin_lock_irqsave(q->queue_lock, flags);
	__blk_end_request_all(req, error);
	nid->nr_req--;
	spin_unlock_irqrestore(q->queue_lock, flags);
}

static void nid_request_handler(struct request_queue *q)
		__releases(q->queue_lock) __acquires(q->queue_lock)
{
	char *log_header = "nid_request_handler";
	struct request *req;
	
	while ((req = blk_fetch_request(q)) != NULL) {
		struct nid_device *nid;

		nid = (struct nid_device *)req->rq_disk->private_data;
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
			nid_log_warning("nid_request_handler: %s is error_now", req->rq_disk->disk_name);
			nid_end_io(req);
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
#endif


//extern int nid_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg);
static int
nid_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	char *log_header = "nid_ioctl";
	struct drv_interface *drv_p = nid_drv_p;
	struct nid_device *nid = bdev->bd_disk->private_data;
	struct task_struct *msg_thread;
	struct nid_message *driver_ioctl_msg;
	int rc = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	BUG_ON(nid->magic != NID_MAGIC);

	switch (cmd) {
	case NID_INIT_NETLINK:
		nid_log_info("%s: NID_INIT_NETLINK", log_header);
		drv_p->dr_op->dr_start(drv_p);
		drv_p->dr_op->dr_unset_msg_exit(drv_p);
		msg_thread = kthread_create(drv_p->dr_op->dr_msg_process_thread, nid_drv_p, "%s", "nid_msg");
		if (IS_ERR(msg_thread)) {
			nid_log_error("%s: cannot create msg_thread", log_header);
			return -1;
		}
		wake_up_process(msg_thread);
		break;

	case NID_CHAN_OPEN:
		nid_log_info("%s: NID_CHAN_OPEN", log_header);
		drv_p->dr_op->dr_wait_ioctl_cmd(drv_p);
		driver_ioctl_msg = drv_p->dr_op->dr_get_ioctl_message(drv_p);
		drv_p->dr_op->dr_chan_open(drv_p, driver_ioctl_msg->m_chan_id,
			driver_ioctl_msg->m_rsfd, driver_ioctl_msg->m_dsfd);	
		drv_p->dr_op->dr_done_ioctl_cmd(drv_p);
		break;

	case NID_DEVICE_START:
		nid_log_info("%s: NID_DEVICE_START", log_header);
		drv_p->dr_op->dr_wait_ioctl_cmd(drv_p);
		driver_ioctl_msg = drv_p->dr_op->dr_get_ioctl_message(drv_p);
		drv_p->dr_op->dr_start_device(drv_p, driver_ioctl_msg->m_devminor,
			driver_ioctl_msg->m_rsfd, driver_ioctl_msg->m_dsfd);
		//dev_p = drv_p->dr_op->dr_get_device(drv_p, driver_ioctl_msg->m_devminor);
		//dev_p->dv_op->dv_nid_start(dev_p, driver_ioctl_msg->m_rsfd, driver_ioctl_msg->m_dsfd);
		// nid_device_start(driver_ioctl_msg->m_devminor, driver_ioctl_msg->m_rsfd, driver_ioctl_msg->m_dsfd);
		drv_p->dr_op->dr_done_ioctl_cmd(drv_p);
		break;

	case NID_SET_KERN_LOG_LEVEL:
		nid_log_info("%s: NID_SET_KERN_LOG_LEVEL: %lu", log_header, arg);
		if (arg >= LOG_EMERG && arg <= LOG_DEBUG)
			nid_log_set_level(arg);
		break;

	case NID_GET_STAT:
		nid_log_debug("nid_ioctl: NID_GET_STAT");
		if (arg == 0) {
			nid_log_error("nid_ioctl: NID_GET_STAT: buf is NULL");
			return -1;
		}
		rc = drv_p->dr_op->dr_get_stat(drv_p, arg);
		break;

	default:
		nid_log_info("nid_ioctl: got wrong cmd (%d)", cmd);
		rc = -ENOTTY;
		break;
	}
	return rc;
}

const struct block_device_operations nid_fops = {
	.owner = THIS_MODULE,
	.ioctl = nid_ioctl,
};

#if 0
static struct gendisk *nid_add(dev_t dev, int i)
{
	char *log_header = "nid_add";
	struct gendisk *disk = NULL;
	struct nid_device *nid;
	struct dev_setup dev_setup;
	struct dev_interface *dev_p;

	nid_log_info("%s: [i:%d]: start ...", log_header, i);
	if (i >= nids_max) {
		return NULL;
	}

	nid = &nid_dev[i];

	if (nid->disk) {
		nid_log_info("%s: [i%d]: disk existed", log_header, i);
		return nid->disk;
	}

	disk = alloc_disk(1 << part_shift);
	if (!disk) {
		nid_log_error("%s: [i:%d]: failed to alloc_disk", log_header, i);
		return NULL;
	}
	nid->disk = disk;
	nid_log_notice("%s: alloc_disk: 0x%p\n", log_header, nid->disk);

	disk->queue = blk_init_queue(nid_request_handler, &nid_queue_lock);
	if (!disk->queue) {
		put_disk(disk);
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

	nid->dev = dev;
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
	nid->blksize = 1024;
	nid->bytesize = 0;
	disk->major = NID_MAJOR;
	disk->first_minor = i << part_shift;
	disk->fops = &nid_fops;
	disk->private_data = nid;
	sprintf(disk->disk_name, "nid%d", i);
	set_capacity(disk, 0);

	/*
	 * add_disk() will be called after nid_ctl_sender/receiver() is running.
	 */
	dev_setup.idx = i;
	dev_setup.minor = disk->first_minor;
	dev_setup.drv = nid_drv_p;
	dev_p = kcalloc(1, sizeof(*dev_p), GFP_KERNEL);
	dev_initialization(dev_p, &dev_setup);
	nid_drv_p->dr_op->dr_add_device(nid_drv_p, (void *)dev_p, i);
	return disk;
}
#endif

static struct kobject *nid_probe(dev_t dev, int *part, void *data)
{
	//char *log_header = "nid_probe";
	struct kobject *kobj;
	//struct gendisk *disk;
	//struct nid_device *nid;
	//int i, minor;
	int minor;

	minor = MINOR(dev);
#if 0
	i = minor >> part_shift;
	nid_log_debug("%s: probing nid%d, minor:%d\n", log_header, i, MINOR(dev));

	if (i >= nids_max) {
		nid_log_error("%s: i:%d is too big", log_header, i);
		return NULL;
	}
	nid = &nid_dev[i];
	nid->sequence = 0;

	mutex_lock(&nid->nid_mutex);

	if (nid->disk) {
		disk = nid->disk;
		nid_log_debug("%s: nid using half setup disk: %p.\n", log_header, disk);
	} else {
		nid_log_debug("%s: [i:%d]: adding disk ...", log_header, i);
		disk = nid_add(dev, i);
		nid_log_notice("%s: nid use new disk: %p.\n", log_header,  disk);
	}
	if (disk == NULL) {
		mutex_unlock(&nid->nid_mutex);
		return NULL;
	}
#endif

//	kobj = get_disk(disk);

//	mutex_unlock(&nid->nid_mutex);

	kobj = nid_drv_p->dr_op->dr_probe_device(nid_drv_p, minor);
	*part = 0;

	return kobj;
}
/* Kernel module interfaces. */

static int nid_init(void)
{
	char *log_header = "nid_init";
	//int i = 0;
	int err = -ENOMEM;
	//struct nid_request *req_buf;

	nid_log_level = LOG_NOTICE; 
	nid_log_notice("%s: nid (version:%s)", log_header, NID_VERSION); 

#ifdef DEBUG_LEVEL
	nid_log_level = DEBUG_LEVEL;
#endif

	if (max_part < 0) {
		nid_log_error("%s: invalid max_part, must >= 0\n", log_header);
		return -EINVAL;
	}

#if 0
	nid_dev = kcalloc(nids_max, sizeof(*nid_dev), GFP_KERNEL);
	if (!nid_dev)
		return -ENOMEM;
#endif

	part_shift = 0;
	if (max_part > 0) {
		part_shift = fls(max_part);
		max_part = (1UL << part_shift) - 1;
	}

	if ((1UL << part_shift) > DISK_MAX_PARTS)
		return -EINVAL;

	if (nids_max > 1UL << (MINORBITS - part_shift))
		return -EINVAL;
#if 0
	for (i = 0; i < nids_max; i++) {
		req_buf = kcalloc(max_reqs, sizeof(*req_buf), GFP_KERNEL);
		if (!req_buf) {
			err = -ENOMEM;
			goto out;
		}
		nid_dev[i].req_buf = req_buf;
	}
#endif
	if (register_blkdev(NID_MAJOR, "nid")) {
		err = -EIO;
		goto out;
	}

        nid_log_notice("%s: registered NID device at major %d\n", log_header, NID_MAJOR);
#if 0
	for (i = 0; i < nids_max; i++) {
		mutex_init(&nid_dev[i].nid_mutex);
		nid_dev[i].disconnect = 1;
	}
#endif
	/* Real device initialization delayed to register each nid time*/
	blk_register_region(MKDEV(NID_MAJOR, 0), 1UL << MINORBITS,
			THIS_MODULE, nid_probe, NULL, NULL);

	drv_setup.size = nids_max;
	drv_setup.part_shift = part_shift;
	nid_drv_p = kcalloc(1, sizeof(*nid_drv_p), GFP_KERNEL);
	drv_initialization(nid_drv_p, &drv_setup);
	nid_log_notice("%s: part_shift:%d, max_part:%d, nids_max:%d",
		log_header, part_shift, max_part, nids_max);

	return 0;

out:
#if 0
	for (i = 0; i < nids_max; i++) {
		req_buf = nid_dev[i].req_buf;
		kfree(req_buf);
	}
	kfree(nid_dev);
#endif
	return err;
}

static void nid_exit(void)
{
	char *log_header = "nid_exit";
	//int i;
	//struct nid_request *req_buf;

	nid_log_notice("%s: start ...\n", log_header);
#if 0
	for (i = 0; i < nids_max; i++) {
		struct gendisk *disk = nid_dev[i].disk;
		nid_dev[i].magic = 0;
		/* Cleanup any half setup nid disk.*/
		if (disk) {
			if (disk->flags & GENHD_FL_UP){
				nid_log_notice("%s: del_gendisk: 0x%p\n", log_header, disk);
				del_gendisk(disk);
			}
			if (disk->queue){
				nid_log_notice("%s: cleanup_queue: 0x%p\n", log_header, disk->queue);
				blk_cleanup_queue(disk->queue);
			}
			put_disk(disk);
		}
	}
#endif
	nid_drv_p->dr_op->dr_cleanup(nid_drv_p);
	blk_unregister_region(MKDEV(NID_MAJOR, 0), nids_max);

	unregister_blkdev(NID_MAJOR, "nid");
#if 0
	for (i = 0; i < nids_max; i++) {
		req_buf = nid_dev[i].req_buf;
		kfree(req_buf);
	}
	kfree(nid_dev);
#endif
	nid_log_notice("%s: unregistered NID device at major %d\n", log_header, NID_MAJOR);  

	nid_drv_p->dr_op->dr_stop(nid_drv_p);
}

module_init(nid_init);
module_exit(nid_exit);

MODULE_DESCRIPTION("Atlas Block Device");
MODULE_LICENSE("GPL");

module_param(nids_max, int, 0444);
MODULE_PARM_DESC(nids_max, "number of atlas block devices to initialize (default: 16)");
module_param(max_part, int, 0444);
MODULE_PARM_DESC(max_part, "number of partitions per device (default: 15)");
