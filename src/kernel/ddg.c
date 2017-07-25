/*
 * ddg.c
 * 	Implementation of Driver Device Guardian Module
 */
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "nid.h"
#include "nid_log.h"
#include "nid_shared.h"
#include "mpk_if.h"
#include "drv_if.h"
#include "dev_if.h"
#include "ddg_if.h"

#define __minor_to_idx(minor, part_shift) ((minor) >> (part_shift))

struct ddg_private {
	struct drv_interface	*p_drv;
	struct dev_interface	**p_devices;
	uint16_t		p_part_shift;
	uint16_t		p_max_devices;
	struct mutex		*p_nid_mutexes;
	struct rw_semaphore	*p_rwsems;
};

static void inline
__ddg_rlock_device(struct ddg_private *priv_p, int idx)
{
	struct rw_semaphore *rwsem_p = &priv_p->p_rwsems[idx];
	down_read(rwsem_p);
};

static void inline
__ddg_runlock_device(struct ddg_private *priv_p, int idx)
{
	struct rw_semaphore *rwsem_p = &priv_p->p_rwsems[idx];
	up_read(rwsem_p);
};

static void inline
__ddg_wlock_device(struct ddg_private *priv_p, int idx)
{
	struct rw_semaphore *rwsem_p = &priv_p->p_rwsems[idx];
	down_write(rwsem_p);
};

static void inline
__ddg_wunlock_device(struct ddg_private *priv_p, int idx)
{
	struct rw_semaphore *rwsem_p = &priv_p->p_rwsems[idx];
	up_write(rwsem_p);
};

static struct kobject *
ddg_probe_device(struct ddg_interface *ddg_p, int minor)
{
	char *log_header = "ddg_probe_device";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_setup dev_setup;
	struct dev_interface *dev_p;
	int idx;
	struct kobject *kobj;

	nid_log_info("%s: start (minor:%d) ...", log_header, minor);
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	mutex_lock(&priv_p->p_nid_mutexes[idx]);
	__ddg_wlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	if (dev_p) {
		nid_log_info("%s: probing device (minor:%d) existed", log_header, minor);
		kobj = dev_p->dv_op->dv_get_kobj(dev_p);
		goto out;
	}

	dev_setup.idx = idx;
	dev_setup.minor = minor;
	dev_setup.part_shift = priv_p->p_part_shift;
	dev_setup.ddg = ddg_p;
	dev_setup.mutex = &priv_p->p_nid_mutexes[idx];
	dev_p = kcalloc(1, sizeof(*dev_p), GFP_KERNEL);
	dev_initialization(dev_p, &dev_setup);
	priv_p->p_devices[idx] = dev_p;
	kobj = dev_p->dv_op->dv_nid_probe(dev_p);

out:
	__ddg_wunlock_device(priv_p, idx);
	mutex_unlock(&priv_p->p_nid_mutexes[idx]);
	return kobj;
}

static int
ddg_add_device(struct ddg_interface *ddg_p, struct dev_interface *dev_p)
{
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	int idx, minor;
	minor = dev_p->dv_op->dv_get_minor(dev_p);
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	priv_p->p_devices[idx] = dev_p;
	return 0;
}


/*
 * Algorithm:
 * 	start device should be executed in ioctl thread. We just notify
 * 	an ioctl thread to do the job and be waiting for the job finished
 */
static void
ddg_start_channel(struct ddg_interface *ddg_p)
{
	char *log_header = "ddg_start_channel";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct drv_interface *drv_p = priv_p->p_drv;

	nid_log_notice("%s: start ...", log_header);
	drv_p->dr_op->dr_wakeup_ioctl_cmd(drv_p);
	drv_p->dr_op->dr_wait_ioctl_cmd_done(drv_p);
}

/*
 * Note:
 * 	This will be call in ioctl to do start channel
 */
static int
ddg_do_start_channel(struct ddg_interface *ddg_p, int minor, int rsfd, int dsfd, int do_purge)
{
	char *log_header = "ddg_do_start_channel";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int idx;
	nid_log_notice("%s: minor:%d rsfd:%d dsfd:%d", log_header, minor, rsfd, dsfd);
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_rlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	dev_p->dv_op->dv_start_channel(dev_p, rsfd, dsfd, do_purge);
	__ddg_runlock_device(priv_p, idx);
	return 0;
}

static void
ddg_purge(struct ddg_interface *ddg_p, int minor)
{
	char *log_header = "ddg_purge";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int idx;
	nid_log_notice("%s: minor:%d", log_header, minor);
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_rlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	dev_p->dv_op->dv_purge(dev_p);
	__ddg_runlock_device(priv_p, idx);
}

static void
ddg_send_keepalive_channel(struct ddg_interface *ddg_p, int minor)
{
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int idx;
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_rlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	dev_p->dv_op->dv_send_keepalive_channel(dev_p);
	__ddg_runlock_device(priv_p, idx);
}

static void
ddg_got_keepalive_channel(struct ddg_interface *ddg_p, int chan_id, struct nid_request *reply)
{
	char *log_header = "ddg_got_keepalive_channel";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	uint32_t len;

	nid_log_debug("%s: start (chand_type:asc, chan_id:%d)", log_header, chan_id);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHANNEL;
	nid_msg.m_req_code = NID_CODE_CHANNEL_KEEPALIVE;
	nid_msg.m_chan_type = NID_CHANNEL_TYPE_ASC_CODE;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_chan_id = chan_id;
	nid_msg.m_has_chan_id = 1;
	nid_msg.m_recv_sequence = be64_to_cpu(reply->dseq);
	nid_msg.m_has_recv_sequence = 1;
	nid_msg.m_wait_sequence = be64_to_cpu(reply->offset);
	nid_msg.m_has_wait_sequence = 1;
	nid_msg.m_srv_status = be32_to_cpu(reply->len);
	nid_msg.m_has_srv_status = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ddg_init_device(struct ddg_interface *ddg_p, int minor, uint8_t chan_type,
	uint64_t size, uint32_t blksize, char *bundle_id)
{
	char *log_header = "ddg_init_device";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int idx;
	nid_log_notice("%s: minor:%d", log_header, minor);
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_rlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	BUG_ON(!dev_p);
	dev_p->dv_op->dv_init_device(dev_p, chan_type, size, blksize, bundle_id);
	__ddg_runlock_device(priv_p, idx);
}

static void
ddg_set_ioerror_device(struct ddg_interface *ddg_p, int minor)
{
	char *log_header = "ddg_set_ioerror_device";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int idx;

	nid_log_warning("%s: start minor:%d", log_header, minor);
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_rlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	dev_p->dv_op->dv_nid_set_ioerror(dev_p);
	__ddg_runlock_device(priv_p, idx);
}

static void
ddg_delete_device(struct ddg_interface *ddg_p, int minor)
{
	char *log_header = "ddg_delete_device";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int idx;

	nid_log_notice("%s: start (minor:%d) ...", log_header, minor);
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_wlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	if (dev_p) {
		dev_p->dv_op->dv_nid_to_delete(dev_p);
		nid_log_notice("%s: minor:%d cleanup", log_header, minor);
		dev_p->dv_op->dv_cleanup(dev_p);
		kfree(dev_p);
		priv_p->p_devices[idx] = NULL;
	} else {
		nid_log_notice("%s: trying to delete (minor:%d) which does not exist", log_header, minor);
		ddg_p->dg_op->dg_delete_device_done(ddg_p, minor);
	}
	__ddg_wunlock_device(priv_p, idx);
}

static void
ddg_recover_channel(struct ddg_interface *ddg_p, int minor)
{
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int idx;
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_rlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	dev_p->dv_op->dv_recover_channel(dev_p);
	__ddg_runlock_device(priv_p, idx);
}

static void
ddg_upgrade_channel(struct ddg_interface *ddg_p, int minor, int force)
{
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int idx;
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_rlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	dev_p->dv_op->dv_upgrade_channel(dev_p, force);
	__ddg_runlock_device(priv_p, idx);
}

static int
ddg_get_stat_device(struct ddg_interface *ddg_p, int minor, struct nid_message *msg)
{
	char *log_header = "ddg_get_stat_device";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int rc, idx;

	nid_log_debug("%s: start (minor:%d) ...", log_header, minor);
	idx = __minor_to_idx(minor, priv_p->p_part_shift);
	__ddg_rlock_device(priv_p, idx);
	dev_p = priv_p->p_devices[idx];
	if (!dev_p) {
		nid_log_error("%s: minor:%d does not exist", log_header, minor);
		rc = -1;
		goto out;
	}
	rc = dev_p->dv_op->dv_get_stat(dev_p, msg);

out:
	__ddg_runlock_device(priv_p, idx);
	return rc;
}

static void
ddg_cleanup(struct ddg_interface *ddg_p)
{
	char *log_header = "ddg_cleanup";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p;
	int i;

	nid_log_notice("%s: start ...", log_header);
	for (i = 0; i < priv_p->p_max_devices; i++) {
		__ddg_wlock_device(priv_p, i);
		dev_p = priv_p->p_devices[i];
		if (dev_p) {
			dev_p->dv_op->dv_cleanup(dev_p);
			kfree(dev_p);
			priv_p->p_devices[i] = NULL;
		}
		__ddg_wunlock_device(priv_p, i);
	}
}

static void
ddg_init_device_done(struct ddg_interface *ddg_p, int minor, uint8_t chan_type, uint8_t ret_code)
{
	char *log_header = "ddg_init_device_done";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	uint32_t len = 0;

	nid_log_debug("%s: minor:%d", log_header, minor);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_INIT_DEVICE;
	nid_msg.m_devminor = minor;
	nid_msg.m_has_devminor = 1;
	nid_msg.m_chan_type = chan_type;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_has_code = 1;
	nid_msg.m_code = ret_code;
	len = 256;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ddg_delete_device_done(struct ddg_interface *ddg_p, int minor)
{
	char *log_header = "ddg_delete_device_done";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	uint32_t len = 0;

	nid_log_debug("%s: minor:%d", log_header, minor);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_DELETE_DEVICE;
	nid_msg.m_devminor = minor;
	nid_msg.m_has_devminor = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ddg_recover_channel_done(struct ddg_interface *ddg_p, int chan_id)
{
	char *log_header = "ddg_recover_channel_done";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	uint32_t len;

	nid_log_debug("%s: chan_id:%d", log_header, chan_id);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHANNEL;
	nid_msg.m_req_code = NID_CODE_CHANNEL_RECOVER;
	nid_msg.m_chan_type = NID_CHANNEL_TYPE_ASC_CODE;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_chan_id = chan_id;
	nid_msg.m_has_chan_id = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ddg_start_channel_done(struct ddg_interface *ddg_p, int minor)
{
	char *log_header = "ddg_start_channel_done";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	uint32_t len;

	nid_log_debug("%s: minor:%d", log_header, minor);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHANNEL;
	nid_msg.m_req_code = NID_CODE_CHANNEL_START;
	nid_msg.m_chan_type = NID_CHANNEL_TYPE_ASC_CODE;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_chan_id = minor;	// use minor as chan_id
	nid_msg.m_has_chan_id = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ddg_upgrade_channel_done(struct ddg_interface *ddg_p, int minor, char code)
{
	char *log_header = "ddg_upgrade_channel_done";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	uint32_t len;

	nid_log_debug("%s: minor:%d", log_header, minor);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHANNEL;
	nid_msg.m_req_code = NID_CODE_CHANNEL_UPGRADE;
	nid_msg.m_chan_type = NID_CHANNEL_TYPE_ASC_CODE;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_chan_id = minor;
	nid_msg.m_has_chan_id = 1;
	nid_msg.m_code = code;
	nid_msg.m_has_code = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ddg_set_ioerror_device_done(struct ddg_interface *ddg_p, int minor, int code)
{
	char *log_header = "ddg_set_ioerror_device_done";
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	uint32_t len;

	nid_log_warning("%s: minor:%d", log_header, minor);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_IOERROR_DEVICE;
	nid_msg.m_devminor = minor;
	nid_msg.m_has_devminor = 1;
	nid_msg.m_code = code;
	nid_msg.m_has_code = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ddg_drop_request(struct ddg_interface *ddg_p, struct request *req, int dev_id)
{
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p = priv_p->p_devices[dev_id];
	dev_p->dv_op->dv_drop_request(dev_p, req);
}

static void
ddg_reply_request(struct ddg_interface *ddg_p, struct request *req, int dev_id)
{
	struct ddg_private *priv_p = (struct ddg_private *)ddg_p->dg_private;
	struct dev_interface *dev_p = priv_p->p_devices[dev_id];
	dev_p->dv_op->dv_reply_request(dev_p, req);
}

static struct ddg_operations ddg_op = {
	.dg_probe_device = ddg_probe_device,
	.dg_add_device = ddg_add_device,
	.dg_start_channel = ddg_start_channel,
	.dg_do_start_channel = ddg_do_start_channel,
	.dg_purge = ddg_purge,
	.dg_send_keepalive_channel = ddg_send_keepalive_channel,
	.dg_got_keepalive_channel = ddg_got_keepalive_channel,
	.dg_init_device = ddg_init_device,
	.dg_set_ioerror_device = ddg_set_ioerror_device,
	.dg_delete_device = ddg_delete_device,
	.dg_recover_channel = ddg_recover_channel,
	.dg_upgrade_channel = ddg_upgrade_channel,
	.dg_get_stat_device = ddg_get_stat_device,
	.dg_cleanup = ddg_cleanup,

	.dg_init_device_done = ddg_init_device_done,
	.dg_start_channel_done = ddg_start_channel_done,
	.dg_delete_device_done = ddg_delete_device_done,
	.dg_recover_channel_done = ddg_recover_channel_done,
	.dg_upgrade_channel_done = ddg_upgrade_channel_done,
	.dg_set_ioerror_device_done = ddg_set_ioerror_device_done,

	.dg_drop_request = ddg_drop_request,
	.dg_reply_request = ddg_reply_request,
};

int
ddg_initialization(struct ddg_interface *ddg_p, struct ddg_setup *setup)
{
	char *log_header = "ddg_initialization";
	struct ddg_private *priv_p;
	int i;

	nid_log_info("%s: start ...", log_header);
	ddg_p->dg_op = &ddg_op;
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
	ddg_p->dg_private = priv_p;
	priv_p->p_drv = setup->drv;
	priv_p->p_part_shift = setup->part_shift;
	priv_p->p_max_devices = setup->max_devices;
	priv_p->p_devices = kcalloc(priv_p->p_max_devices, sizeof(*priv_p->p_devices), GFP_KERNEL);
	priv_p->p_nid_mutexes = kcalloc(priv_p->p_max_devices, sizeof(*priv_p->p_nid_mutexes), GFP_KERNEL);
	priv_p->p_rwsems = kcalloc(priv_p->p_max_devices, sizeof(*priv_p->p_rwsems), GFP_KERNEL);
	for (i = 0; i < priv_p->p_max_devices; i++) {
		mutex_init(&priv_p->p_nid_mutexes[i]);
		__init_rwsem(&priv_p->p_rwsems[i], NULL, NULL);
	}
	return 0;
}
