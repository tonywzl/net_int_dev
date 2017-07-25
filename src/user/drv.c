/*
 * drv.c
 * 	Implementation of Driver Module
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "agent.h"
#include "mpk_if.h"
#include "umpka_if.h"
#include "mq_if.h"
#include "adt_if.h"
#include "ascg_if.h"
#include "drv_if.h"
#include "pp2_if.h"
#include "umpk_driver_if.h"
#include "version.h"

struct drv_private {
	struct mpk_interface	*p_mpk;
	struct umpka_interface	*p_umpka;
	struct adt_interface	*p_adt;
	struct ascg_interface	*p_ascg;
	struct pp2_interface	*p_pp2;
	struct pp2_interface	*p_dyn2_pp2;
	uint8_t			p_stop;
};

static void
drv_send_message(struct drv_interface *drv_p, struct nid_message *nid_msg, int *len)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct adt_interface *adt_p = priv_p->p_adt;
	char *msg_buf;

	msg_buf = adt_p->a_op->a_get_buffer(adt_p, *len);
	mpk_p->m_op->m_encode(mpk_p, msg_buf, len, (struct nid_message_hdr *)nid_msg);
	adt_p->a_op->a_send(adt_p, msg_buf, *len);
}

static int
drv_is_stop(struct drv_interface *drv_p)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	return priv_p->p_stop ? 1 : 0;
}

static int
drv_get_chan_id(struct drv_interface *drv_p, char *chan_uuid)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	return ascg_p->ag_op->ag_get_chan_id(ascg_p, chan_uuid);
}

static void
drv_set_ascg(struct drv_interface *drv_p, struct ascg_interface *ascg_p)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	assert(!priv_p->p_ascg);
	priv_p->p_ascg = ascg_p;
}

static void
drv_cleanup(struct drv_interface *drv_p)
{
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct adt_interface *adt_p = priv_p->p_adt;

	while (priv_p->p_stop == 0) {
		sleep(1);
	}

	adt_p->a_op->a_stop(adt_p);
	adt_p->a_op->a_cleanup(adt_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)adt_p);

	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
	drv_p->dr_private = NULL;
}

struct drv_operations drv_op = {
	.dr_send_message = drv_send_message,
	.dr_is_stop = drv_is_stop,
	.dr_get_chan_id = drv_get_chan_id,
	.dr_set_ascg = drv_set_ascg,
	.dr_cleanup = drv_cleanup,
};

static void
__drv_process_msg(struct drv_private *priv_p, char *mpk_buf, int len)
{
	char *log_header = "__drv_process_msg";
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int left = len;
	int rc;
	struct nid_message res;
	char the_uuid[NID_MAX_UUID];

	while (left) {
		memset(&res, 0, sizeof(res));
		rc = mpk_p->m_op->m_decode(mpk_p, mpk_buf, &left, (struct nid_message_hdr *)&res);
		if (rc) {
			nid_log_error("%s: cannot decode message (left:%d)", log_header, left);
			break;
		}

		switch (res.m_req) {
		case NID_REQ_INIT_DEVICE:
			nid_log_notice("%s: got NID_REQ_INIT_DEVICE, minor:%d, chan_type:%d",
				log_header, res.m_devminor, res.m_chan_type);
			if (res.m_chan_type == NID_CHANNEL_TYPE_ASC_CODE) {
				ascg_p->ag_op->ag_rdev_init_device(ascg_p, res.m_devminor, res.m_code);
			} else {
				nid_log_error("%s: got NID_REQ_INIT_DEVICE, minor:%d, wrong chan_type:%d",
					log_header, res.m_devminor, res.m_chan_type);
			}
			break;

		case NID_REQ_DELETE_DEVICE:
			ascg_p->ag_op->ag_rdev_delete_device(ascg_p, res.m_devminor);
			break;

		case NID_REQ_IOERROR_DEVICE:
			ascg_p->ag_op->ag_rdev_ioerror_device(ascg_p, res.m_devminor, res.m_code);
			break;

		case NID_REQ_AGENT_STOP:
			agent_stop();
			break;

		case NID_REQ_CHANNEL:
			nid_log_info("%s: got NID_CODE_CHANNEL (code:%d)",
				log_header, res.m_req_code);
			if (res.m_req_code == NID_CODE_CHANNEL_KEEPALIVE) {
				nid_log_info("%s: got NID_CODE_CHANNEL_KEEPALIVE (uuid:%s, chan_id:%d)",
					log_header, the_uuid, res.m_chan_id);
				ascg_p->ag_op->ag_rdev_keepalive(ascg_p, res.m_chan_type, res.m_chan_id,
					res.m_recv_sequence, res.m_wait_sequence, res.m_srv_status);

			} else if (res.m_req_code == NID_CODE_CHANNEL_RECOVER) {
				nid_log_notice("%s: got NID_CODE_CHANNEL_RECOVER(chan_type:%d, chan_id:%d)",
					log_header, res.m_chan_type, res.m_chan_id);
				ascg_p->ag_op->ag_rdev_recover_channel(ascg_p, res.m_chan_type, res.m_chan_id);

			} else if (res.m_req_code == NID_CODE_CHANNEL_START) {
				nid_log_notice("%s: got NID_CODE_CHANNEL_START (chan_type:%d, chan_id:%d)",
					log_header, res.m_chan_type, res.m_chan_id);
				ascg_p->ag_op->ag_rdev_start_channel(ascg_p, res.m_chan_type, res.m_chan_id);

			} else if (res.m_req_code == NID_CODE_CHANNEL_UPGRADE) {
				nid_log_notice("%s: got NID_CODE_CHANNEL_UPGRADE (chan_type:%d, chan_id:%d, code:%d)",
					log_header, res.m_chan_type, res.m_chan_id, res.m_code);
				ascg_p->ag_op->ag_rdev_upgrade(ascg_p, res.m_chan_type, res.m_chan_id, res.m_code);

			} else {
				nid_log_error("%s: NID_CODE_CHANNEL got unknown code (%d)",
					log_header, res.m_req_code);
			}
			break;

		case NID_REQ_CHECK_DRIVER: {
			int sfd = res.m_sfd;
			uint32_t len2;
			struct umessage_driver_version_resp resp_msg;
			struct umpka_interface *umpka_p = priv_p->p_umpka;
			char msg_buf[64], nothing_back;

			memset(&resp_msg, 0, sizeof(resp_msg));
			resp_msg.um_header.um_req = UMSG_DRIVER_CMD_VERSION;
			resp_msg.um_header.um_req_code = UMSG_DRIVER_CODE_RESP_VERSION;
			resp_msg.um_version_len = strlen(NID_VERSION);
			memcpy(resp_msg.um_version, NID_VERSION, resp_msg.um_version_len);
			rc = umpka_p->um_op->um_encode(umpka_p, msg_buf, &len2, NID_CTYPE_DRIVER, &resp_msg);
			if (rc) {
				close(sfd);
				break;
			}
			write(sfd, msg_buf, len2);
			read(sfd, &nothing_back, 1);

			break;
		}

		default:
			nid_log_error("%s: got unknown message %d", log_header, res.m_req);
			break;
		}
	}
}

static void *
_drv_message_thread(void *p)
{
	char *log_header = "_drv_message_thread";
	struct drv_interface *drv_p = (struct drv_interface *)p;
	struct drv_private *priv_p = (struct drv_private *)drv_p->dr_private;
	struct adt_interface *adt_p = priv_p->p_adt;

	nid_log_notice("%s: start ...", log_header);
	while (!priv_p->p_ascg) {
		sleep(1);
	}

	while (!agent_is_stop()) {
		struct mq_message_node *mn_p;
		mn_p = adt_p->a_op->a_receive(adt_p);
		__drv_process_msg(priv_p, mn_p->m_data, mn_p->m_len);
		if (mn_p->m_free) {
			mn_p->m_free(mn_p);
			mn_p->m_container = NULL;
		}
		adt_p->a_op->a_collect_mnode(adt_p, mn_p);
	}
	priv_p->p_stop = 1;
	nid_log_notice("%s: exit ...", log_header);
	return NULL;
}

static int
__start_driver_nl()
{
        int fd;
        char *devname = AGENT_RESERVED_DEV;

        fd = open(devname, O_RDWR|O_CLOEXEC);
        if (fd < 0) {
                nid_log_error("nidagent failed to open %s: (%d)%s",
                        devname, errno, strerror(errno));
                return -1;
        }

        ioctl(fd, NID_INIT_NETLINK, 0);

        close(fd);
        return 0;
}

static struct adt_interface *
__driver_prepare(struct pp2_interface *pp2_p, struct pp2_interface *dyn2_pp2_p)
{
        struct adt_setup adt_setup;
        struct adt_interface *adt_p = NULL;
        int rc;

        rc = __start_driver_nl();
        if (rc) {
                nid_log_error("nidagent: __start_driver_nl failed");
                return NULL;
        }

        adt_setup.pp2 = pp2_p;
        adt_setup.dyn2_pp2 = dyn2_pp2_p;

        adt_p = (struct adt_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*adt_p));
        adt_initialization(adt_p, &adt_setup);

        return adt_p;
}

int
drv_initialization(struct drv_interface *drv_p, struct drv_setup *setup)
{
	char *log_header = "drv_initialization";
	struct drv_private *priv_p;
	struct pp2_interface *pp2_p = setup->pp2, *dyn2_pp2_p = setup->dyn2_pp2;
	pthread_t thread_id;
	pthread_attr_t attr;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	drv_p->dr_op = &drv_op;
	priv_p = (struct drv_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	drv_p->dr_private = priv_p;
	priv_p->p_ascg = setup->ascg;
	priv_p->p_mpk = setup->mpk;
	priv_p->p_umpka = setup->umpka;
	priv_p->p_adt = __driver_prepare(pp2_p, dyn2_pp2_p);
	priv_p->p_pp2 = setup->pp2;
	priv_p->p_dyn2_pp2 = dyn2_pp2_p;

	if(!priv_p->p_adt) {
		pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
		return -1;
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _drv_message_thread, drv_p);

	return 0;
}
