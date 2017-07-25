/*
 * driver.c
 */

#include "nid.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "nl_if.h"
#include "mq_if.h"
#include "dat_if.h"
#include "ddg_if.h"
//#include "dev_if.h"
#include "drv_if.h"
#include "version.h"

#define NID_MAX_PART	15

static spinlock_t driver_ioctl_lck;

extern struct drv_interface *nid_drv_p;

#define __minor_to_idx(minor, part_shift) ((minor) >> (part_shift))

struct drv_private {
	int			p_size;
	struct ddg_interface	p_ddg;
	struct mpk_interface	p_mpk;
	struct mq_interface	p_imq;
	struct mq_interface	p_omq;
	struct nl_interface	p_nl;
	struct dat_interface	p_dat;
	struct nid_message	*p_ioctl_msg;
	spinlock_t		p_ioctl_lck;
	wait_queue_head_t	p_ioctl_wq;
	wait_queue_head_t	p_ioctl_done_wq;
	uint8_t			p_started;
	uint8_t			p_msg_exit;
	uint8_t			p_msg_to_exit;
};

static void
__nid_nl_receive(struct sk_buff *skb)
{
	char *log_header = "__nid_nl_receive";
	struct drv_interface *drv_p = nid_drv_p;
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct dat_interface *dat_p = &priv_p->p_dat;
	nid_log_debug("%s: start ...", log_header);
	dat_p->d_op->d_receive(dat_p, skb);
}

static void
__nid_msg_process_func(struct drv_interface *drv_p, char *msg, int len)
{
	char *log_header = "__nid_msg_process_func";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	//struct dev_interface *dev_p;
	struct mpk_interface *mpk_p = &priv_p->p_mpk;
	struct dat_interface *dat_p = &priv_p->p_dat;
	char *p = msg, *msg_buf;
	int left = len, len2;
	struct nid_message nid_msg, nid_msg2;
	int rc;

	while (left) {
		memset(&nid_msg, 0, sizeof(nid_msg));
		rc = mpk_p->m_op->m_decode(mpk_p, p, &left, (struct nid_message_hdr *)&nid_msg);
		if (rc) {
			nid_log_error("%s: decode error (rc:%d)", log_header, rc);
			return;
		}

		switch (nid_msg.m_req) {
		case NID_REQ_INIT_DEVICE:
			nid_log_warning("%s: got NID_REQ_INIT_DEVICE for minor:%d, left:%d",
				log_header, nid_msg.m_devminor, left);
			ddg_p->dg_op->dg_init_device(ddg_p, nid_msg.m_devminor, nid_msg.m_chan_type,
				nid_msg.m_size, nid_msg.m_blksize, nid_msg.m_bundle_id);
			break;

		case NID_REQ_IOERROR_DEVICE:
			nid_log_warning("_nid_dat_msg_func: got NID_REQ_IOERROR_DRIVER for minor:%d, left:%d",
				nid_msg.m_devminor, left);
			ddg_p->dg_op->dg_set_ioerror_device( ddg_p, nid_msg.m_devminor);
			break;

		case NID_REQ_DELETE_DEVICE:
			nid_log_warning("%s: got NID_REQ_DELETE_DEVICE for minor:%d, left:%d",
				log_header, nid_msg.m_devminor, left);
			if (nid_msg.m_has_devminor == 0) {
				nid_log_error("%s: NID_REQ_DELETE_DEVICE w/o m_has_devminor",
				log_header);
			}
			ddg_p->dg_op->dg_delete_device(ddg_p, nid_msg.m_devminor);
			break;

		case NID_REQ_AGENT_STOP:
			nid_log_warning("%s: got NID_REQ_AGENT_STOP", log_header);
			msg_buf = kmalloc(128, GFP_KERNEL);
			len2 = 128;
			memset(&nid_msg2, 0, sizeof(nid_msg2));
			nid_msg2.m_req = NID_REQ_AGENT_STOP;
			rc = mpk_p->m_op->m_encode(mpk_p, msg_buf, &len2, (struct nid_message_hdr *)&nid_msg2);
			if (!rc)
				dat_p->d_op->d_send(dat_p, msg_buf, len2);
			else
				nid_log_error("_nid_dat_msg_func: NID_REQ_AGENT_STOP encode error rc:%d", rc);
			priv_p->p_msg_to_exit = 1;
			break;

		case NID_REQ_BOC_DRIVER:
			nid_log_notice("%s: add a channel(minor:%d)", log_header, nid_msg.m_devminor);
			break;

		case NID_REQ_CHANNEL:
			nid_log_notice("%s: channel req (code:%d)", log_header, nid_msg.m_req_code);

			if (nid_msg.m_req_code == NID_CODE_CHANNEL_KEEPALIVE) {
				nid_log_debug("%s: channel keepalive chan_type:%d, chan_id:%d",
					log_header, nid_msg.m_chan_type, nid_msg.m_chan_id);

				if (nid_msg.m_chan_type == NID_CHANNEL_TYPE_ASC_CODE) {
					ddg_p->dg_op->dg_send_keepalive_channel(ddg_p, nid_msg.m_chan_id);
				} else {
					nid_log_error("%s: NID_REQ_CHANNEL got unknown type (%d)",
						log_header, nid_msg.m_chan_type);
				}

			} else if (nid_msg.m_req_code == NID_CODE_CHANNEL_RECOVER) {
				nid_log_debug("%s: channel recover chan_type:%d, chan_id:%d",
					log_header, nid_msg.m_chan_type, nid_msg.m_chan_id);
				if (nid_msg.m_chan_type == NID_CHANNEL_TYPE_ASC_CODE) {
					ddg_p->dg_op->dg_recover_channel(ddg_p, nid_msg.m_chan_id);
				} else if (nid_msg.m_chan_type == NID_CHANNEL_TYPE_CHAN_CODE) {
					nid_log_error("%s: NID_CODE_CHANNEL_RECOVER (chan_type:%d) not support yet",
						log_header, nid_msg.m_chan_type);
				} else {
					nid_log_error("%s: NID_REQ_CHANNEL got unknown type (%d)",
						log_header, nid_msg.m_chan_type);
				}

			} else if (nid_msg.m_req_code == NID_CODE_CHANNEL_START) {
				BUG_ON(priv_p->p_ioctl_msg);
				priv_p->p_ioctl_msg = &nid_msg;
				if (nid_msg.m_chan_type == NID_CHANNEL_TYPE_ASC_CODE) {
					ddg_p->dg_op->dg_start_channel(ddg_p);
				} else {
					nid_log_info("%s: NID_CODE_CHANNEL_START chan_id:%d, got wrong chan_type %d",
						log_header, nid_msg.m_chan_id, nid_msg.m_chan_type);
				}

			} else if (nid_msg.m_req_code == NID_CODE_CHANNEL_UPGRADE) {
				if (nid_msg.m_chan_type == NID_CHANNEL_TYPE_ASC_CODE) {
					ddg_p->dg_op->dg_upgrade_channel(ddg_p, nid_msg.m_chan_id,
						(int)nid_msg.m_upgrade_force);
				} else if (nid_msg.m_has_chan_type == NID_CHANNEL_TYPE_CHAN_CODE) {
					nid_log_error("%s: NID_CODE_CHANNEL_UPGRADE (chan_type:%d) not support yet",
						log_header, nid_msg.m_chan_type);
				} else {
					nid_log_error("%s: NID_CODE_CHANNEL_UPGRADE chan_id:%d, got wrong chan_type %d",
						log_header, nid_msg.m_chan_id, nid_msg.m_chan_type);
				}

			} else {
				nid_log_error("%s: NID_REQ_CHANNEL got unknown code (%d)",
					log_header, nid_msg.m_req_code);
			}
			break;

		case NID_REQ_CHECK_DRIVER:
			nid_log_warning("%s: got NID_REQ_CHECK_DRIVER", log_header);
			msg_buf = kmalloc(128, GFP_KERNEL);
			len2 = 128;
			memset(&nid_msg2, 0, sizeof(nid_msg2));
			nid_msg2.m_req = NID_REQ_CHECK_DRIVER;
			nid_msg2.m_versionlen = strlen(NID_VERSION);
			nid_msg2.m_version = NID_VERSION;
			nid_msg2.m_has_version = 1;
			nid_msg2.m_sfd = nid_msg.m_sfd;
			nid_msg2.m_has_sfd = 1;
			rc = mpk_p->m_op->m_encode(mpk_p, msg_buf, &len2, (struct nid_message_hdr *)&nid_msg2);
			if (!rc)
				dat_p->d_op->d_send(dat_p, msg_buf, len2);
			else
				nid_log_error("_nid_dat_msg_func: NID_REQ_CHECK_DRIVER encode error rc:%d", rc);
			break;

		default:
			nid_log_error("_nid_dat_msg_func: got unknown request %d", nid_msg.m_req);
			return;
		}
		p = msg + len - left;
	}
}

static int
drv_msg_process_thread(void *data)
{
	char *log_header = "drv_msg_process_thread";
	struct drv_interface *drv_p = (struct drv_interface *)data;
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct mq_interface *mq_p = &priv_p->p_imq;
	struct mq_message_node *mn_p;

	nid_log_notice("%s: start ...", log_header);
next_node:
	if (priv_p->p_msg_to_exit)
		goto out;

	mn_p = mq_p->m_op->m_dequeue(mq_p);
	if (!mn_p) {
		msleep(100);
		goto next_node;
	}

	__nid_msg_process_func(drv_p, mn_p->m_data, mn_p->m_len);

	if (mn_p->m_free) {
		mn_p->m_free(mn_p->m_container);
		mn_p->m_container = NULL;
	}
	mq_p->m_op->m_put_free_mnode(mq_p, mn_p);
	goto next_node;

out:
	nid_log_debug("%s: exiting...", log_header);
	priv_p->p_msg_exit = 1;
	return 0;
}

static void
drv_start(struct drv_interface *drv_p)
{
	char *log_header = "drv_start";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct mq_interface *imq_p = &priv_p->p_imq;
	struct mq_interface *omq_p = &priv_p->p_omq;
	struct mpk_setup mpk_setup;
	struct mq_setup imq_setup, omq_setup;
	struct nl_setup nl_setup;
	struct nl_interface *nl_p = &priv_p->p_nl;
	struct dat_setup dat_setup;
	struct dat_interface *dat_p = &priv_p->p_dat;

	nid_log_notice("%s: start ...", log_header);
	if (priv_p->p_started)
		return;

	mpk_setup.type = 0;
	mpk_initialization(&priv_p->p_mpk, &mpk_setup);
	imq_setup.size = 1024;
	mq_initialization(imq_p, &imq_setup);
	omq_setup.size = 1024;
	mq_initialization(omq_p, &omq_setup);
	nl_setup.receive = __nid_nl_receive;
	nl_initialization(nl_p, &nl_setup);
	dat_setup.nl = nl_p;
	dat_setup.imq = imq_p;
	dat_setup.omq = omq_p;
	dat_initialization(dat_p, &dat_setup);
	spin_lock_init(&driver_ioctl_lck);
	priv_p->p_started = 1;
}

static void
drv_stop(struct drv_interface *drv_p)
{
	char *log_header = "drv_stop";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct mq_interface *imq_p = &priv_p->p_imq;
	struct mq_interface *omq_p = &priv_p->p_omq;
	struct nl_interface *nl_p = &priv_p->p_nl;
	struct dat_interface *dat_p = &priv_p->p_dat;
	if (!priv_p->p_started)
		return;
	//kthread_stop(msg_thread);
	nid_log_debug("%s: waiting nid_msg_exit ...", log_header);
	while (!priv_p->p_msg_exit)
		msleep(100);

	nid_log_debug("%s: nid_msg_exit done", log_header);
	dat_p->d_op->d_exit(dat_p);
	nl_p->n_op->n_exit(nl_p);
	imq_p->m_op->m_exit(imq_p);
	omq_p->m_op->m_exit(omq_p);
	kfree(priv_p);
	drv_p->dr_private = NULL;
}

static void
drv_dev_purge(struct drv_interface *drv_p, int minor)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	ddg_p->dg_op->dg_purge(ddg_p, minor);
}

static void
drv_start_channel(struct drv_interface *drv_p, int chan_type, int chan_id, int rsfd, int dsfd, int do_purge)
{
	char *log_header = "drv_start_channel";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	nid_log_notice("%s: chan_id:%d rsfd:%d dsfd:%d", log_header, chan_id, rsfd, dsfd);
	if (chan_type == NID_CHANNEL_TYPE_ASC_CODE)
		ddg_p->dg_op->dg_do_start_channel(ddg_p, chan_id, rsfd, dsfd, do_purge);
	else
		nid_log_error("%s: got wrong chan_id:%d", log_header, chan_id);
}

static int
drv_get_stat(struct drv_interface *drv_p, unsigned long arg)
{
	char *log_header = "nid_driver_get_stat";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	struct mpk_interface *mpk_p = &priv_p->p_mpk;
	int minor;
	char *msg_buf;
	int msg_len;
	struct nid_message msg;
	int rc = 0;

	if (get_user(minor, (__user int32_t *)arg))
		return -EFAULT;

	rc = ddg_p->dg_op->dg_get_stat_device(ddg_p, minor, &msg);
	if (rc) {
		nid_log_error("%s: cannot get stat for minor:%d", log_header, minor);
		return -1;
	}

	msg_buf = kcalloc(1, 128, GFP_KERNEL);
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &msg_len, (struct nid_message_hdr *)&msg);
	if (copy_to_user((char *)arg, msg_buf, msg_len))
		rc = -EINVAL;
	kfree(msg_buf);
	return 0;
}

static void
drv_wait_ioctl_cmd(struct drv_interface *drv_p)
{
	char *log_header = "drv_wait_ioctl_cmd";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;

	nid_log_debug("%s: start ...", log_header);
	while (priv_p->p_ioctl_msg == NULL) {
		nid_log_debug("%s: wait_event ...", log_header);
		wait_event_interruptible(priv_p->p_ioctl_wq, priv_p->p_ioctl_msg != NULL);
	}
	nid_log_debug("%s: got ioctl_cmd", log_header);
}

static void
drv_wakeup_ioctl_cmd(struct drv_interface *drv_p)
{
	char *log_header = "drv_wakeup_ioctl_cmd";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	nid_log_notice("%s: start ...", log_header);
	wake_up(&priv_p->p_ioctl_wq);
}

static void
drv_wait_ioctl_cmd_done(struct drv_interface *drv_p)
{
	char *log_header = "drv_wait_ioctl_cmd_done";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	while (priv_p->p_ioctl_msg != NULL)
		wait_event_interruptible(priv_p->p_ioctl_done_wq, priv_p->p_ioctl_msg == NULL);
	nid_log_notice("%s: end", log_header);
}

static void
drv_done_ioctl_cmd(struct drv_interface *drv_p)
{
	char *log_header = "drv_done_ioctl_cmd";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;

	nid_log_debug("%s: start ...", log_header);
	priv_p->p_ioctl_msg = NULL;
	wake_up(&priv_p->p_ioctl_done_wq);
}

static struct kobject *
drv_probe_device(struct drv_interface *drv_p, int minor)
{
	char *log_header = "drv_probe_device";
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	nid_log_info("%s: minor:%d", log_header, minor);
	return ddg_p->dg_op->dg_probe_device(ddg_p, minor);
}

static void
drv_add_device(struct drv_interface *drv_p, void *dev_p, int idx)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	ddg_p->dg_op->dg_add_device(ddg_p, dev_p);
}

/*
 * Must follow dr_wait_ioctl_cmd
 */
static struct nid_message *
drv_get_ioctl_message(struct drv_interface *drv_p)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	return priv_p->p_ioctl_msg;
}

static void
drv_unset_msg_exit(struct drv_interface *drv_p)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	priv_p->p_msg_to_exit = 0;
	priv_p->p_msg_exit = 0;
}

static void
drv_send_message(struct drv_interface *drv_p, struct nid_message *nid_msg, int *len)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct mpk_interface *mpk_p = &priv_p->p_mpk;
	struct dat_interface *dat_p = &priv_p->p_dat;
	char *msg_buf;

	msg_buf = kmalloc(*len, GFP_KERNEL);
	mpk_p->m_op->m_encode(mpk_p, msg_buf, len, (struct nid_message_hdr *)nid_msg);
	dat_p->d_op->d_send(dat_p, msg_buf, *len);
}

static void
drv_cleanup(struct drv_interface *drv_p)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	ddg_p->dg_op->dg_cleanup(ddg_p);
}

static void
drv_drop_request(struct drv_interface *drv_p, struct request *req, int dev_id)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	ddg_p->dg_op->dg_drop_request(ddg_p, req, dev_id);
}

static void
drv_reply_request(struct drv_interface *drv_p, int dev_id, struct request *req)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ddg_interface *ddg_p = &priv_p->p_ddg;
	ddg_p->dg_op->dg_reply_request(ddg_p, req, dev_id);
}


static struct drv_operations drv_op = {
	.dr_msg_process_thread = drv_msg_process_thread,
	.dr_probe_device = drv_probe_device,
	.dr_add_device = drv_add_device,
	.dr_start = drv_start,
	.dr_dev_purge = drv_dev_purge,
	.dr_start_channel = drv_start_channel,
	.dr_stop = drv_stop,
	.dr_unset_msg_exit = drv_unset_msg_exit,
	.dr_get_stat = drv_get_stat,
	.dr_send_message = drv_send_message,
	.dr_wait_ioctl_cmd = drv_wait_ioctl_cmd,
	.dr_wakeup_ioctl_cmd = drv_wakeup_ioctl_cmd,
	.dr_wait_ioctl_cmd_done	= drv_wait_ioctl_cmd_done,
	.dr_done_ioctl_cmd = drv_done_ioctl_cmd,
	.dr_get_ioctl_message = drv_get_ioctl_message,
	.dr_cleanup = drv_cleanup,
	.dr_drop_request = drv_drop_request,
	.dr_reply_request = drv_reply_request,
};

int
drv_initialization(struct drv_interface *drv_p, struct drv_setup *setup)
{
	char *log_header = "drv_initialization";
	struct drv_private *priv_p;
	struct ddg_setup ddg_setup;
	struct ddg_interface *ddg_p;

	nid_log_notice("%s: start ...", log_header);
	drv_p->dr_op = &drv_op;
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
	drv_p->dr_private = priv_p;
	priv_p->p_size = setup->size;
	spin_lock_init(&priv_p->p_ioctl_lck);
	init_waitqueue_head(&priv_p->p_ioctl_wq);
	init_waitqueue_head(&priv_p->p_ioctl_done_wq);
	ddg_p = &priv_p->p_ddg;
	ddg_setup.drv = drv_p;
	ddg_setup.max_devices = setup->max_devices;
	ddg_setup.part_shift = setup->part_shift;
	ddg_initialization(ddg_p, &ddg_setup);
	return 0;
}
