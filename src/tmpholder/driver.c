/* 
 * driver.c
 */

#include "nid.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "nl_if.h"
#include "mq_if.h"
#include "dat_if.h"
#include "drv_if.h"

#define NID_MAX_PART	15

struct mpk_setup mpk_setup;
struct mpk_interface nid_mpk;
static struct mq_setup imq_setup;
static struct mq_interface nid_imq;
static struct mq_setup omq_setup;
static struct mq_interface nid_omq;
static struct nl_setup nl_setup;
static struct nl_interface nid_nl;
static struct dat_setup dat_setup;
static struct dat_interface nid_dat;
static int driver_started = 0;
static int nid_msg_exit = 0;
static int nid_msg_to_exit = 0;
static struct task_struct *msg_thread;
static spinlock_t driver_ioctl_lck;
static wait_queue_head_t driver_ioctl_wq;
static wait_queue_head_t driver_ioctl_done_wq;
static struct nid_message *driver_ioctl_msg = NULL; 

extern int nid_send_req(struct nid_device *nid, struct request *req);
extern int nid_start(struct nid_device *nid, struct block_device *bdev);


static void
_nid_nl_receive(struct sk_buff *skb)
{
	nid_dat.d_op->d_receive(&nid_dat, skb);
}

static void
_nid_msg_process_func(char *msg, int len)
{
	struct mpk_interface *mpk_p = &nid_mpk;
	char *p = msg, *msg_buf;
	int left = len, len2;
	struct nid_message nid_msg, nid_msg2;
	int rc;

	while (left) {
		memset(&nid_msg, 0, sizeof(nid_msg));
		rc = mpk_p->m_op->m_decode(mpk_p, p, &left, &nid_msg);
		if (rc) {
			nid_log_error("_nid_msg_process_func: decode error");
			return;
		}

		switch (nid_msg.m_req) {
		case NID_REQ_KEEPALIVE:
			nid_log_debug("_nid_dat_msg_func: got NID_REQ_KEEPALIVE for minor:%d, left:%d",
				nid_msg.m_devminor, left);
			nid_device_send_keepalive(nid_msg.m_devminor);
			break;

		case NID_REQ_INIT_DEVICE:
			nid_log_warning("_nid_dat_msg_func: got NID_REQ_INIT_DEVICE for minor:%d, left:%d",
				nid_msg.m_devminor, left);
			nid_device_init(nid_msg.m_devminor, nid_msg.m_size, nid_msg.m_blksize);
			break;

		case NID_REQ_START_DEVICE:
			nid_log_warning("_nid_dat_msg_func: got NID_REQ_START_DEVICE for minor:%d, left:%d",
				nid_msg.m_devminor, left);
			spin_lock_irq(&driver_ioctl_lck);
			driver_ioctl_msg = &nid_msg;
			spin_unlock_irq(&driver_ioctl_lck);
			wake_up(&driver_ioctl_wq);
			while (driver_ioctl_msg != NULL)
				wait_event_interruptible(driver_ioctl_done_wq, driver_ioctl_msg == NULL); 
			break;

		case NID_REQ_IOERROR_DEVICE:
			nid_log_warning("_nid_dat_msg_func: got NID_REQ_IOERROR_DRIVER for minor:%d, left:%d",
				nid_msg.m_devminor, left);
			nid_device_set_ioerror(nid_msg.m_devminor);
			break;

		case NID_REQ_DELETE_DEVICE:
			nid_log_warning("_nid_dat_msg_func: got NID_REQ_DELETE_DEVICE for minor:%d, left:%d",
				nid_msg.m_devminor, left);
			if (nid_msg.m_has_devminor == 0) {
				nid_log_error("_nid_dat_msg_func: NID_REQ_DELETE_DEVICE w/o m_has_devminor");
			}
			nid_device_to_delete(nid_msg.m_devminor);
			break;

		case NID_REQ_RECOVER:
			nid_log_warning("_nid_dat_msg_func: got NID_REQ_RECOVER for minor:%d, left:%d",
				nid_msg.m_devminor, left);
			nid_device_to_recover(nid_msg.m_devminor);
			break;

		case NID_REQ_AGENT_STOP:
			nid_log_warning("_nid_dat_msg_func: got NID_REQ_AGENT_STOP");
			msg_buf = kmalloc(128, GFP_KERNEL);
			len2 = 128;
			memset(&nid_msg2, 0, sizeof(nid_msg2));
			nid_msg2.m_req = NID_REQ_AGENT_STOP;
			rc = nid_mpk.m_op->m_encode(&nid_mpk, msg_buf, &len2, &nid_msg2);
			if (!rc)
				nid_dat.d_op->d_send(&nid_dat, msg_buf, len2);
			else
				nid_log_error("_nid_dat_msg_func: NID_REQ_AGENT_STOP encode error rc:%d", rc);
			nid_msg_to_exit = 1;
			break;

		case NID_REQ_UPGRADE:
			nid_log_warning("_nid_dat_msg_func: got NID_REQ_UPGRADE for minor:%d, force: %d, left:%d",
				nid_msg.m_devminor, nid_msg.m_upgrade_force, left);
			nid_device_send_upgrade(nid_msg.m_devminor, nid_msg.m_upgrade_force);
			break;

		default:
			nid_log_error("_nid_dat_msg_func: got unknown request %d", nid_msg.m_req);
			return;
		}
		p = msg + len - left;
	}
}

static int 
_msg_process_thread(void *data)
{
	struct mq_interface *mq_p = (struct mq_interface *)data;
	struct mq_message_node *mn_p;

next_node:
	if (nid_msg_to_exit)
		goto out;

	mn_p = mq_p->m_op->m_dequeue(mq_p);
	if (!mn_p) {
		msleep(100);
		goto next_node;
	}

	_nid_msg_process_func(mn_p->m_data, mn_p->m_len);

	if (mn_p->m_free) {
		mn_p->m_free(mn_p->m_container);
		mn_p->m_container = NULL;
	}
	mq_p->m_op->m_put_free_mnode(mq_p, mn_p);	
	goto next_node;

out:
	nid_log_debug("_msg_process_thread: exiting...");
	nid_msg_exit = 1;
	return 0;
}

static void
_driver_start(void)
{
	mpk_initialization(&nid_mpk, &mpk_setup);
	imq_setup.size = 1024;
	mq_initialization(&nid_imq, &imq_setup);
	omq_setup.size = 1024;
	mq_initialization(&nid_omq, &omq_setup);
	nl_setup.receive = _nid_nl_receive;
	nl_initialization(&nid_nl, &nl_setup);
	dat_setup.nl = &nid_nl;
	dat_setup.imq = &nid_imq;
	dat_setup.omq = &nid_omq;
	dat_initialization(&nid_dat, &dat_setup);
	spin_lock_init(&driver_ioctl_lck);
	init_waitqueue_head(&driver_ioctl_wq);
	init_waitqueue_head(&driver_ioctl_done_wq);
	driver_started = 1;
}

void
nid_driver_stop(void)
{
	if (!driver_started)
		return;
	//kthread_stop(msg_thread);
	nid_log_debug("nid_driver_stop: waiting nid_msg_exit ...");
	while (!nid_msg_exit)
		msleep(100);

	nid_log_debug("nid_driver_stop: nid_msg_exit done");
	nid_dat.d_op->d_exit(&nid_dat);
	nid_nl.n_op->n_exit(&nid_nl);
	nid_imq.m_op->m_exit(&nid_imq);
	nid_omq.m_op->m_exit(&nid_omq);
}

void
nid_driver_send_msg(char *msg_buf, int len)
{
	nid_dat.d_op->d_send(&nid_dat, msg_buf, len);
}

int
nid_driver_get_stat(unsigned long arg)
{
	int minor;
	dev_t dev;
	char *msg_buf;
	int msg_len;
	struct block_device* bdev;
	struct nid_device *nid;
	struct nid_message msg;
	int rc = 0;

	if (get_user(minor, (__user int32_t *)arg))
		return -EFAULT;

	dev = MKDEV(NID_MAJOR, minor);
	bdev = bdget(dev);
	if (!bdev) {
		nid_log_error("nid_driver_get_stat (minor:%d): got null bdev", minor);
		return -EINVAL;
	}
	if (!bdev->bd_disk) {
		nid_log_info("nid_driver_get_stat (minor:%d): got null bdev->bd_disk", minor);
		bdput(bdev);
		return -EINVAL;
	}
	nid = bdev->bd_disk->private_data;
	
	memset(&msg, 0, sizeof(msg));
	msg.m_req = NID_REQ_STAT_SAC;

	msg.m_rcounter = nid->rcounter;
	msg.m_has_rcounter = 1;
	msg.m_r_rcounter = nid->r_rcounter;
	msg.m_has_r_rcounter = 1;

	msg.m_wcounter = nid->wcounter;
	msg.m_has_wcounter = 1;
	msg.m_r_wcounter = nid->r_wcounter;
	msg.m_has_r_wcounter = 1;

	msg.m_kcounter = nid->kcounter;
	msg.m_has_kcounter = 1;
	msg.m_r_kcounter = nid->r_kcounter;
	msg.m_has_r_kcounter = 1;

	//total receive message length
	msg.m_recv_sequence = nid->dat_rcv_sequence;
	msg.m_has_recv_sequence = 1;
	//total send message length
	msg.m_wait_sequence = nid->dat_snd_sequence;
	msg.m_has_wait_sequence = 1;

	msg_buf = kcalloc(1, 128, GFP_KERNEL);
	nid_mpk.m_op->m_encode(&nid_mpk, msg_buf, &msg_len, &msg); 
	if (copy_to_user((char *)arg, msg_buf, msg_len))
		rc = -EINVAL;
	kfree(msg_buf);
	bdput(bdev);
	return rc;
}

int
nid_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	struct nid_device *nid = bdev->bd_disk->private_data;
	int rc = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	BUG_ON(nid->magic != NID_MAGIC);

	switch (cmd) {
	case NID_INIT_NETLINK:
		nid_log_info("nid_ioctl: NID_INIT_NETLINK");
		if (!driver_started)
			_driver_start();
		nid_msg_to_exit = 0;
		nid_msg_exit = 0;
		msg_thread = kthread_create(_msg_process_thread, &nid_imq, "%s", "nid_msg");
		if (IS_ERR(msg_thread)) {
			nid_log_error("_driver_start: cannot create msg_thread");
			return -1;
		}
		wake_up_process(msg_thread);
		break;

	case NID_DEVICE_START:
		nid_log_info("nid_ioctl: NID_DEVICE_START");
		while (driver_ioctl_msg == NULL)
			wait_event_interruptible(driver_ioctl_wq, driver_ioctl_msg != NULL); 
		nid_device_start(driver_ioctl_msg->m_devminor, driver_ioctl_msg->m_rsfd, driver_ioctl_msg->m_dsfd);
		driver_ioctl_msg = NULL;
		wake_up(&driver_ioctl_done_wq);
		break;

	case NID_SET_KERN_LOG_LEVEL:
		nid_log_info("nid_ioctl: NID_SET_KERN_LOG_LEVEL: %lu", arg);
		if (arg >= LOG_EMERG && arg <= LOG_DEBUG)
			nid_log_set_level(arg);
		break;

	case NID_GET_STAT:
		nid_log_debug("nid_ioctl: NID_GET_STAT");
		if (arg == 0) {
			nid_log_error("nid_ioctl: NID_GET_STAT: buf is NULL");
			return -1;
		}
		rc = nid_driver_get_stat(arg);
		break;

	default:
		nid_log_error("nid_ioctl: got wrong cmd (%d)", cmd);
		rc = -ENOTTY;
		break;
	}
	return rc;
}

struct drv_private {

};

static struct drv_operations drv_op = {

};

int
drv_initialization(struct drv_interface *drv_p, struct drv_setup *setup)
{
	char *log_header = "drv_initialization";
	struct drv_private *priv_p;

	nid_log_notice("%s: start ...", log_header);
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
	drv_p->dr_private = priv_p;
	drv_p->dr_op = &drv_op;
	return 0;
}
