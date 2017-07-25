/*
 * amc.c
 * 	Implemantation of Agent Manager Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include "list.h"
#include "util_nw.h"
#include "nid_log.h"
#include "ini_if.h"
#include "mpk_if.h"
#include "ascg_if.h"
#include "asc_if.h"
#include "amc_if.h"
#include "nid_shared.h"
#include "agent.h"
#include "pp2_if.h"

struct amc_private {
	int			p_rsfd;
	struct ini_interface	*p_ini;
	struct mpk_interface	*p_mpk;
	struct ascg_interface	*p_ascg;
	struct pp2_interface	*p_dyn2_pp2;
};

typedef char* (*stat_process_func_t)(struct mpk_interface *, struct amc_stat_record *, int *, struct amc_interface *);
stat_process_func_t stat_func[NID_REQ_MAX];

static int
set_driver_log_level(int level)
{
	int fd;
	char *devname = AGENT_RESERVED_DEV;

	fd = open(devname, O_RDWR|O_CLOEXEC);
	if (fd < 0) {
		nid_log_error("nidagent failed to open %s: (%d)%s",
			devname, errno, strerror(errno));
		return -1;
	}

	ioctl(fd, NID_SET_KERN_LOG_LEVEL, level);

	close(fd);
	return 0;
}

static char*
_stat_process_asc(struct mpk_interface *mpk_p, struct amc_stat_record *sr_p, int *out_len, struct amc_interface * amc_p)
{
	struct amc_private *priv_p = (struct amc_private *)amc_p->a_private;
	struct pp2_interface *dyn2_pp2_p = priv_p->p_dyn2_pp2;
	struct asc_stat *stat_p = (struct asc_stat *)sr_p->r_data;
	char *msg_buf;
	struct nid_message nid_msg;
	int len = 0;

	msg_buf = (char *)dyn2_pp2_p->pp2_op->pp2_get(dyn2_pp2_p, 4096);
	len = 4096;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_STAT;
	nid_msg.m_uuid = stat_p->as_uuid;
	nid_msg.m_uuidlen = strlen(stat_p->as_uuid);
	nid_msg.m_has_uuid = 1;
	nid_msg.m_ip = stat_p->as_sip;
	nid_msg.m_iplen = strlen(stat_p->as_sip);
	nid_msg.m_has_ip = 1;
	nid_msg.m_devname = stat_p->as_devname;
	nid_msg.m_devnamelen = strlen(stat_p->as_devname);
	nid_msg.m_has_devname = 1;
	nid_msg.m_state = stat_p->as_state;
	nid_msg.m_has_state = 1;
	nid_msg.m_alevel = stat_p->as_alevel;
	nid_msg.m_has_alevel = 1;

	nid_msg.m_rcounter = stat_p->as_read_counter;
	nid_msg.m_has_rcounter = 1;
	nid_msg.m_r_rcounter = stat_p->as_read_resp_counter;
	nid_msg.m_has_r_rcounter = 1;

	nid_msg.m_wcounter = stat_p->as_write_counter;
	nid_msg.m_has_wcounter = 1;
	nid_msg.m_r_wcounter = stat_p->as_write_resp_counter;
	nid_msg.m_has_r_wcounter = 1;

	nid_msg.m_kcounter = stat_p->as_keepalive_counter;
	nid_msg.m_has_kcounter = 1;
	nid_msg.m_r_kcounter = stat_p->as_keepalive_resp_counter;
	nid_msg.m_has_r_kcounter = 1;

	nid_msg.m_recv_sequence = stat_p->as_recv_sequence;
	nid_msg.m_has_recv_sequence = 1;
	nid_msg.m_wait_sequence = stat_p->as_wait_sequence;
	nid_msg.m_has_wait_sequence = 1;

	nid_msg.m_live_time = stat_p->as_ts;
	nid_msg.m_has_live_time = 1;

	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	*out_len = len;
	return msg_buf;
}

static int
amc_accept_new_channel(struct amc_interface *amc_p, int sfd)
{
	struct amc_private *priv_p = amc_p->a_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
amc_do_channel(struct amc_interface *amc_p)
{
	struct amc_private *priv_p = (struct amc_private *)amc_p->a_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int sfd = priv_p->p_rsfd;
	struct amc_stat_record *sr_p, *sr_p2;
	struct list_head stat_head;
	char cmd, *p, *msg_buf, devname[NID_MAX_DEVNAME];
	struct nid_message nid_msg;
	int len, out_len, nwrite;
	int rc, level;
	char nothing;
	char ver_buf[16];
	struct pp2_interface *dyn2_pp2_p = priv_p->p_dyn2_pp2;

	nid_log_debug("amc_do_channel: start");

	memset(&nid_msg, 0, sizeof(nid_msg));

	read(sfd, &cmd, 1);
	nid_log_debug("amc_do_channel: got cmd:%d", cmd);
	switch (cmd) {
	case NID_REQ_CHECK_AGENT:
		nid_log_debug("amc_do_channel: got check agent command");
		msg_buf = (char *)dyn2_pp2_p->pp2_op->pp2_get(dyn2_pp2_p, 32);
		p = msg_buf;
		*p++ = cmd;
		util_nw_read_n(sfd, p, 5);
		p++;	// skip m_req_code
		len = be32toh(*(uint32_t *)p);
		p += 4;
		if (len > 6)
			util_nw_read_n(sfd, p, len - 6);
		mpk_p->m_op->m_decode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
		nid_msg.m_req = NID_REQ_CHECK_AGENT;
		mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
		write(sfd, msg_buf, len);
		read(sfd, &nothing, 1);
		close(sfd);
		dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)msg_buf);
		break;

	case NID_REQ_UPDATE:
		nid_log_debug("amc_do_channel: NID_REQ_UPDATE command");
		rc = ascg_p->ag_op->ag_update(ascg_p);
		if (rc <= 0) {
			nid_log_debug("amc_do_channel: NID_REQ_UPDATE, rc:%d",
				rc);
			close(sfd);
		}
		break;

	case NID_REQ_STOP:
		nid_log_debug("amc_do_channel: NID_REQ_STOP command");
		ascg_p->ag_op->ag_stop(ascg_p);
		break;

	case NID_REQ_IOERROR_DEVICE:
		nid_log_debug("amc_do_channel: NID_REQ_IOERROR_DEVICE command");
		msg_buf = (char *)dyn2_pp2_p->pp2_op->pp2_get(dyn2_pp2_p, 512);
		p = msg_buf;
		*p++ = NID_REQ_IOERROR_DEVICE;
		util_nw_read_n(sfd, p, 5);
		p++;	// skip m_req_code
		len = be32toh(*(uint32_t *)p);
		p += 4;
		util_nw_read_n(sfd, p, len - 6);
		mpk_p->m_op->m_decode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
		memcpy(devname, nid_msg.m_devname, nid_msg.m_devnamelen);
		devname[nid_msg.m_devnamelen] = 0;
		nid_log_debug("amc_do_channel: NID_REQ_IOERROR_DEVICE, %s",
			devname);
		rc = ascg_p->ag_op->ag_ioerror_device(ascg_p, devname, sfd);

		nid_log_error("amc_do_channel: NID_REQ_IOERROR_DEVICE %s, rc:%d",
			devname, rc);
//		close(sfd);

		dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)msg_buf);
		break;

	case NID_REQ_DELETE:
		nid_log_debug("amc_do_channel: NID_REQ_DELETE command");
		msg_buf = (char *)dyn2_pp2_p->pp2_op->pp2_get(dyn2_pp2_p, 512);
		p = msg_buf;
		*p++ = NID_REQ_DELETE;
		util_nw_read_n(sfd, p, 5);
		p++;	// skip m_req_code
		len = be32toh(*(uint32_t *)p);
		p += 4;
		util_nw_read_n(sfd, p, len - 6);
		mpk_p->m_op->m_decode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
		memcpy(devname, nid_msg.m_devname, nid_msg.m_devnamelen);
		devname[nid_msg.m_devnamelen] = 0;
		nid_log_debug("amc_do_channel: NID_REQ_DELETE %s",
			devname);
		rc = ascg_p->ag_op->ag_delete_device(ascg_p, devname, sfd, 1);
		if (rc <= 0) {
			nid_log_error("amc_do_channel: NID_REQ_DELETE %s,rc:%d",
				devname, rc);
			close(sfd);
		}
		dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)msg_buf);
		break;

	case NID_REQ_STAT_GET:
		nid_log_debug("amc_do_channel: got get stat command");
		INIT_LIST_HEAD(&stat_head);
		ascg_p->ag_op->ag_get_stat(ascg_p, &stat_head);
		list_for_each_entry_safe(sr_p, sr_p2, struct amc_stat_record, &stat_head, r_list) {
			p = stat_func[sr_p->r_type](mpk_p, sr_p, &out_len, amc_p);
			if (!out_len)
				continue;
			nwrite = write(sfd, p, out_len);
			if (nwrite != out_len) {
				nid_log_error("sasmc_do_channel: cannot write to the nid manager");
			}
			dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)p);
			list_del(&sr_p->r_list);
			free(sr_p->r_data);
			free(sr_p);
		}
		close(sfd);
		break;

	case NID_REQ_STAT_RESET:
		nid_log_debug("amc_do_channel:stat_reset not implemented");
		close(sfd);
		break;

	case NID_REQ_UPGRADE:
		nid_log_debug("amc_do_channel: NID_REQ_UPGRADE command. sfd: %d", sfd);
		msg_buf = (char *)dyn2_pp2_p->pp2_op->pp2_get(dyn2_pp2_p, 512);
		p = msg_buf;
		*p++ = NID_REQ_UPGRADE;
		util_nw_read_n(sfd, p, 5);
		p++;	// skip m_req_code
		len = be32toh(*(uint32_t *)p);
		p += 4;
		util_nw_read_n(sfd, p, len - 6);
		mpk_p->m_op->m_decode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
		memcpy(devname, nid_msg.m_devname, nid_msg.m_devnamelen);
		devname[nid_msg.m_devnamelen] = 0;
		nid_log_debug("amc_do_channel: NID_REQ_UPGRADE %s, force: %d", devname, nid_msg.m_upgrade_force);
		rc = ascg_p->ag_op->ag_upgrade_device(ascg_p, devname, sfd, nid_msg.m_upgrade_force);
		if (rc) {
			nid_log_error("amc_do_channel: NID_REQ_UPGRADE %s, rc:%d",
				devname, rc);
			write(sfd, &rc, 1);
			close(sfd);
		}
		dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)msg_buf);
		break;

	case NID_REQ_SET_LOG_LEVEL:
		util_nw_read_n(sfd, &level, sizeof(level));
		if (level >= LOG_EMERG && level <= LOG_DEBUG) {
			nid_log_debug("amc_do_channel: NID_REQ_SET_LOG_LEVEL: %d", level);
			nid_log_set_level(level);

			set_driver_log_level(level);
		} else {
			printf("NID LOG LEVEL: %d\n", nid_log_level);
		}

		close(sfd);
		break;

	case NID_REQ_GET_VERSION:
		memset(ver_buf, 0, 16);
		sprintf(ver_buf, "%s", __DATE__);
		write(sfd, ver_buf, 16);

		memset(ver_buf, 0, 16);
		sprintf(ver_buf, "%s", __TIME__);
		write(sfd, ver_buf, 16);

		memset(ver_buf, 0, 16);
		sprintf(ver_buf, "%d", nid_log_level);
		write(sfd, ver_buf, 16);

		close(sfd);
		break;

	case NID_REQ_SET_HOOK:
		nid_log_debug("amc_do_channel: NID_REQ_SET_HOOK command. sfd: %d", sfd);
		msg_buf = dyn2_pp2_p->pp2_op->pp2_get(dyn2_pp2_p, 128);
		p = msg_buf;
		*p++ = NID_REQ_SET_HOOK;
		util_nw_read_n(sfd, p, 5);
		p++;	// skip m_req_code
		len = be32toh(*(uint32_t *)p);
		p += 4;
		util_nw_read_n(sfd, p, len - 6);
		mpk_p->m_op->m_decode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
		nid_log_debug("amc_do_channel: NID_REQ_SET_HOOK: %d", nid_msg.m_cmd);

		ascg_p->ag_op->ag_set_hook(ascg_p, nid_msg.m_cmd);
		rc = 0;
		write(sfd, &rc, 1);
		close(sfd);
		dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)msg_buf);
		break;

	default:
		nid_log_debug("amc_do_channel: got unknown command");
		close(sfd);
		break;
	}

	return;
}

static void
amc_cleanup(struct amc_interface *amc_p)
{
	struct amc_private *priv_p = (struct amc_private *)amc_p->a_private;
	struct pp2_interface *dyn2_pp2_p = priv_p->p_dyn2_pp2;

	nid_log_debug("amc_cleanup start, amc_p:%p", amc_p);

	if (amc_p->a_private != NULL) {
		dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)priv_p);
		amc_p->a_private = NULL;
	}
}

struct amc_operations amc_op = {
	.a_accept_new_channel = amc_accept_new_channel,
	.a_do_channel = amc_do_channel,
	.a_cleanup = amc_cleanup,
};

int
amc_initialization(struct amc_interface *amc_p, struct amc_setup *setup)
{
	struct amc_private *priv_p;
	struct pp2_interface *dyn2_pp2_p = setup->dyn2_pp2;

	nid_log_debug("amc_initialization start ...");
	if (!setup) {
		nid_log_error("amc_initialization got null setup");
		return -1;
	}
	amc_p->a_op = &amc_op;
	priv_p = (struct amc_private *)dyn2_pp2_p->pp2_op->pp2_get_zero(dyn2_pp2_p, sizeof(*priv_p));
	amc_p->a_private = priv_p;
	priv_p->p_dyn2_pp2 = dyn2_pp2_p;

	priv_p->p_rsfd = -1;
	priv_p->p_ascg = setup->ascg;
	priv_p->p_mpk = setup->mpk;
	stat_func[NID_REQ_STAT] = _stat_process_asc;

	return 0;
}
