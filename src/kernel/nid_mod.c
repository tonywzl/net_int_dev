/*
 * nid_mod.c
 */
#include "nid.h"
#include "nid_shared.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "dev_if.h"
#include "drv_if.h"
#include "version.h"
#include <linux/version.h>

static int max_part = 15;
static int part_shift;

unsigned int nids_max = 256;
int nid_log_level;


static struct drv_setup drv_setup;
struct drv_interface *nid_drv_p;

static int
nid_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	char *log_header = "nid_ioctl";
	struct drv_interface *drv_p = nid_drv_p;
	struct task_struct *msg_thread;
	struct nid_message *driver_ioctl_msg;
	int do_purge = 0, rc = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	switch (cmd) {
	case NID_INIT_NETLINK:
		nid_log_info("%s: NID_INIT_NETLINK", log_header);
		drv_p->dr_op->dr_start(drv_p);
		drv_p->dr_op->dr_unset_msg_exit(drv_p);
		msg_thread = kthread_create(drv_p->dr_op->dr_msg_process_thread, drv_p, "%s", "nid_msg");
		if (IS_ERR(msg_thread)) {
			nid_log_error("%s: cannot create msg_thread", log_header);
			return -1;
		}
		wake_up_process(msg_thread);
		break;

	case NID_START_CHANNEL:
		nid_log_info("%s: NID_START_CHANNEL", log_header);
		drv_p->dr_op->dr_wait_ioctl_cmd(drv_p);
		driver_ioctl_msg = drv_p->dr_op->dr_get_ioctl_message(drv_p);
		if (driver_ioctl_msg->m_has_code)
			do_purge = driver_ioctl_msg->m_code;
		drv_p->dr_op->dr_start_channel(drv_p, driver_ioctl_msg->m_chan_type, driver_ioctl_msg->m_chan_id,
			driver_ioctl_msg->m_rsfd, driver_ioctl_msg->m_dsfd, do_purge);
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

static struct kobject *
nid_probe(dev_t dev, int *part, void *data)
{
	char *log_header = "nid_probe";
	struct drv_interface *drv_p = nid_drv_p;
	struct kobject *kobj;
	int minor = MINOR(dev);
	nid_log_notice("%s: start (minor:%d)", log_header, minor);
	kobj = drv_p->dr_op->dr_probe_device(drv_p, minor);
	*part = 0;
	nid_log_notice("%s: (minor:%d), kobj:%p", log_header, minor, kobj);
	return kobj;
}

/* Kernel module interfaces. */

static int nid_init(void)
{
	char *log_header = "nid_init";
	struct drv_interface *drv_p = NULL; 
	int err = -ENOMEM;

	nid_log_level = LOG_NOTICE; 
	nid_log_notice("%s: nid (version:%s)", log_header, NID_VERSION); 

#ifdef DEBUG_LEVEL
	nid_log_level = DEBUG_LEVEL;
#endif

	if (max_part < 0) {
		nid_log_error("%s: invalid max_part, must >= 0\n", log_header);
		return -EINVAL;
	}

	part_shift = 0;
	if (max_part > 0) {
		part_shift = fls(max_part);
		max_part = (1UL << part_shift) - 1;
	}

	if ((1UL << part_shift) > DISK_MAX_PARTS)
		return -EINVAL;

	if (nids_max > 1UL << (MINORBITS - part_shift))
		return -EINVAL;

	if (register_blkdev(NID_MAJOR, "nid")) {
		err = -EIO;
		goto err_out;
	}

        nid_log_notice("%s: registered NID device at major %d\n", log_header, NID_MAJOR);
	/* Real device initialization delayed to register each nid time*/
	blk_register_region(MKDEV(NID_MAJOR, 0), 1UL << MINORBITS,
			THIS_MODULE, nid_probe, NULL, NULL);

	drv_setup.size = nids_max;
	drv_setup.part_shift = part_shift;
	drv_setup.max_devices = nids_max;
	drv_setup.max_channels = nids_max * 4;
	drv_p = kcalloc(1, sizeof(*drv_p), GFP_KERNEL);
	nid_drv_p = drv_p;
	drv_initialization(drv_p, &drv_setup);
	nid_log_notice("%s: part_shift:%d, max_part:%d, nids_max:%d",
		log_header, part_shift, max_part, nids_max);

	return 0;

err_out:
	nid_log_error("%s: error exiting ", log_header);
	if (drv_p) {
		drv_p->dr_op->dr_cleanup(drv_p);	
		kfree(drv_p);
		nid_drv_p = NULL;
	}
	return err;
}

static void nid_exit(void)
{
	char *log_header = "nid_exit";
	struct drv_interface *drv_p = nid_drv_p; 

	nid_log_notice("%s: start ...\n", log_header);
	if (drv_p)
		drv_p->dr_op->dr_cleanup(drv_p);
	blk_unregister_region(MKDEV(NID_MAJOR, 0), nids_max);

	unregister_blkdev(NID_MAJOR, "nid");
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
