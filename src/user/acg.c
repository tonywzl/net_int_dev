/*
 * acg.c
 * 	Implementation of Agent Channel Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "agent.h"
#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "agent.h"
#include "mpk_if.h"
#include "tp_if.h"
#include "ini_if.h"
#include "asc_if.h"
#include "amc_if.h"
#include "ash_if.h"
#include "drv_if.h"
#include "ascg_if.h"
#include "acg_if.h"
#include "pp2_if.h"
#include "util_nw.h"
#include "umpka_if.h"
#include "umpk_agent_if.h"
#include "umpk_driver_if.h"
#include "version.h"

struct acg_private {
	struct ascg_interface	*p_ascg;
	struct drv_interface	*p_drv;
	struct mpk_interface	*p_mpk;
	struct tp_interface	*p_tp;
	struct ini_interface	*p_ini;
	struct list_head	p_asc_head;
	struct list_head	p_delete_asc_head;
	struct list_head	p_waiting_chan_head;
	struct list_head	p_agent_keys;
	struct pp2_interface	*p_pp2;
	struct pp2_interface	*p_dyn_pp2;
	struct pp2_interface	*p_dyn2_pp2;
	struct umpka_interface	*p_umpka;
	int			p_update_counter;
	uint8_t			p_keepalive_timer;
	char			p_stop;
	char			p_dr_done;
	uint32_t		p_hook_flag; //1: enable, 0: disable
	pthread_mutex_t		p_amc_lck;
};

struct acg_channel {
	struct list_head	a_list;
	uint16_t		a_type;		// channel type
	void			*a_chan;	// data for this communication channel
	char			*a_uuid;
	int			a_chan_id;
	int			a_delay;
	int			a_delay_save;
	char			a_established;
};

struct acg_housekeeping_job {
	struct tp_jobheader	j_header;
	struct acg_interface	*j_acg;
	int			j_delay;
};

static void*
acg_accept_new_channel(struct acg_interface *acg_p, int sfd, char *cip)
{
	char *log_header = "acg_accept_new_channel";
	struct acg_private *priv_p = (struct acg_private *)acg_p->ag_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct acg_channel *chan_p = NULL;
	struct amc_setup amc_setup;
	struct amc_interface *amc_p;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct pp2_interface *dyn2_pp2_p = priv_p->p_dyn2_pp2;
//	char chan_type;
	char chan_type_buf[2], magic_buf[NID_SIZE_CONN_MAGIC + 1];
	char version_buf[NID_SIZE_VERSION];
	char magic[] = NID_CONN_MAGIC;
	uint16_t chan_type, proto_ver, my_proto_ver = NID_PROTOCOL_VER;
	int rc, nread;

	nid_log_debug("acg_accept_new_channel: estblish channel from %s", cip);
	assert(priv_p);
	if(priv_p->p_stop) {
		nid_log_error("%s: agent in stop progress, reject request", log_header);
		goto out;
	}
	/*
	 * first of all, figure out the channel type
	 */
	nread = util_nw_read_n(sfd, &chan_type_buf[0], 1);
       	if (nread != 1) {
		nid_log_error("%s: read failed, nread:%d, errno:%d",
			log_header, nread, errno);
		goto out;
	}
	if (chan_type_buf[0] & 0x80) {
		nread = util_nw_read_n(sfd, &chan_type_buf[1], 1);
		if (nread != 1) {
			nid_log_error("%s: read failed, nread:%d, errno:%d",
				log_header, nread, errno);
			goto out;
		}
		chan_type = (uint8_t)chan_type_buf[1] + (chan_type_buf[0] << 8);

		if (chan_type_buf[1] == 'I'){
			magic_buf[0] = chan_type_buf[1];
			nread = util_nw_read_n(sfd, &magic_buf[1], NID_SIZE_CONN_MAGIC - 1);
			if (nread != NID_SIZE_CONN_MAGIC - 1) {
				nid_log_error("%s: read failed, nread:%d, errno:%d",
					log_header, nread, errno);
				goto out;
			}
			magic_buf[NID_SIZE_CONN_MAGIC] = '\0';
			/* not valid NID message, close the socket*/
			if (strcmp(magic_buf, magic)) {
				close(sfd);
				goto out;
			}

			nread = util_nw_read_n(sfd, &proto_ver, NID_SIZE_PROTOCOL_VER);
			if (nread != NID_SIZE_PROTOCOL_VER) {
				nid_log_error("%s: read failed, nread:%d, errno:%d",
					log_header, nread, errno);
				goto out;
			}
			/* will send magic and protocol version back, no matter this message is supported or not, so send it now */
			util_nw_write_n(sfd, &magic, NID_SIZE_CONN_MAGIC);
			util_nw_write_n(sfd, &my_proto_ver, NID_SIZE_PROTOCOL_VER);

			if (proto_ver > NID_PROTOCOL_RANGE_HIGH || proto_ver < NID_PROTOCOL_RANGE_LOW) {
				nid_log_warning("%s: the NID_PROTOCOL_VER (%d) is not supported by server (%s)",
					log_header, proto_ver, NID_VERSION);
				close(sfd);
				goto out;
			}

			nread = util_nw_read_n(sfd, &chan_type, 2);
			if (nread != NID_SIZE_PROTOCOL_VER) {
				nid_log_error("%s: read failed, nread:%d, errno:%d",
					log_header, nread, errno);
				goto out;
			}
		} else {
			chan_type = (uint8_t)chan_type_buf[1] + ((chan_type_buf[0] & 0x7f) << 8);
			nid_log_warning("%s: get 3.5.0 style command", log_header);
		}

		/* get version information */
		nread = util_nw_read_n(sfd, version_buf, NID_SIZE_VERSION);
		if (nread != NID_SIZE_VERSION) {
			nid_log_error("%s: read failed, nread:%d, errno:%d",
				log_header, nread, errno);
			goto out;
		}
		nid_log_warning("%s, version: %d.%d.%d", log_header, (uint8_t)version_buf[0], (uint8_t)version_buf[1], *(uint16_t *)&version_buf[2]);
	} else {
		chan_type = chan_type_buf[0];
		nid_log_warning("%s: get legacy command", log_header);
	}

	chan_p = (struct acg_channel *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*chan_p));
	chan_p->a_type = chan_type;
	switch (chan_type) {
	case NID_CTYPE_ADM:
		nid_log_debug("acg_accept_new_channel: a new admin channel");
		amc_p = (struct amc_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*amc_p));
		amc_setup.mpk = mpk_p;
		amc_setup.ascg = ascg_p;
		amc_setup.dyn2_pp2 = dyn2_pp2_p;
		rc = amc_initialization(amc_p, &amc_setup);
		if (rc) {
			pp2_p->pp2_op->pp2_put(pp2_p, (void *)amc_p);
			pp2_p->pp2_op->pp2_put(pp2_p, (void *)chan_p);
			chan_p = NULL;
			break;
		}
		chan_p->a_chan = amc_p;
		rc = amc_p->a_op->a_accept_new_channel(amc_p, sfd);
		if (rc) {
			amc_p->a_op->a_cleanup(amc_p);
			pp2_p->pp2_op->pp2_put(pp2_p, (void *)amc_p);
			pp2_p->pp2_op->pp2_put(pp2_p, (void *)chan_p);
			chan_p = NULL;
		}
		break;

	case NID_CTYPE_AGENT: {
		nid_log_debug("%s: doing NID_CTYPE_AGENT", log_header);
		char msg_buf[64], *p = msg_buf, ver_buf[128];
		struct umessage_agent_version nid_msg;
		struct umessage_agent_version_resp resp_msg;
		struct umpka_interface *umpka_p = priv_p->p_umpka;
		uint32_t len;
		char nothing_back;

		util_nw_read_n(sfd, msg_buf, UMSG_AGENT_HEADER_LEN);
		nid_msg.um_header.um_req = *p++;
		nid_msg.um_header.um_req_code = *p++;
		nid_msg.um_header.um_len = *(uint32_t *)p;
		rc = umpka_p->um_op->um_decode(umpka_p, msg_buf,nid_msg.um_header.um_len, NID_CTYPE_AGENT, &nid_msg);
		if (rc)
			goto out;
		assert(nid_msg.um_header.um_req == UMSG_AGENT_CMD_VERSION);

		memset(&resp_msg, 0, sizeof(resp_msg));
		resp_msg.um_header.um_req = UMSG_AGENT_CMD_VERSION;
		resp_msg.um_header.um_req_code = UMSG_AGENT_CODE_RESP_VERSION;
		strcpy(ver_buf, NID_VERSION);
#ifdef HAS_MQTT
		strcat(ver_buf, "\nMQTT Enabled");
#endif
		resp_msg.um_version_len = strlen(ver_buf);
		memcpy(resp_msg.um_version, ver_buf, resp_msg.um_version_len);
		rc = umpka_p->um_op->um_encode(umpka_p, msg_buf, &len, NID_CTYPE_AGENT, &resp_msg);
		if (rc)
			goto out;
		write(sfd, msg_buf, len);
		read(sfd, &nothing_back, 1);

		pp2_p->pp2_op->pp2_put(pp2_p, (void *)chan_p);
		return NULL;

		break;
	}

	case NID_CTYPE_DRIVER: {
		nid_log_debug("%s: doing NID_CTYPE_DRIVER", log_header);
		char msg_buf[64], *p = msg_buf;
		struct umessage_agent_version nid_msg;
		struct umpka_interface *umpka_p = priv_p->p_umpka;

		util_nw_read_n(sfd, msg_buf, UMSG_DRIVER_HEADER_LEN);
		nid_msg.um_header.um_req = *p++;
		nid_msg.um_header.um_req_code = *p++;
		nid_msg.um_header.um_len = *(uint32_t *)p;
		rc = umpka_p->um_op->um_decode(umpka_p, msg_buf,nid_msg.um_header.um_len, NID_CTYPE_DRIVER, &nid_msg);
		if (rc)
			goto out;
		assert(nid_msg.um_header.um_req == UMSG_DRIVER_CMD_VERSION);
		ascg_p->ag_op->ag_check_driver(ascg_p, sfd);

		pp2_p->pp2_op->pp2_put(pp2_p, (void *)chan_p);
		return NULL;
		break;
	}

	default:
		nid_log_error("acg_accept_new_channel: got unknown channel type:%d",
			chan_type);
        	pp2_p->pp2_op->pp2_put(pp2_p, (void *)chan_p);
        	chan_p = NULL;
		break;
	}

out:
	if (!chan_p)
		close(sfd);
	return (void *)chan_p;
}

static void
acg_do_channel(struct acg_interface *acg_p, void *data)
{
	struct acg_private *priv_p = (struct acg_private *)acg_p->ag_private;
	struct acg_channel *chan_p = (struct acg_channel *)data;
	uint16_t chan_type = chan_p->a_type;
	struct amc_interface *amc_p;
	struct pp2_interface *pp2_p = priv_p->p_pp2;

	nid_log_debug("acg_do_channel start (%p)", data);
	assert(priv_p);
	switch (chan_type) {
	case NID_CTYPE_ADM:
		pthread_mutex_lock(&priv_p->p_amc_lck);
		if(priv_p->p_stop) {
			pthread_mutex_unlock(&priv_p->p_amc_lck);
			nid_log_error("acg_do_channel: acg in stop state");
			goto out;
		}
		nid_log_debug("acg_do_channel: do admin channel");
		amc_p = (struct amc_interface *)chan_p->a_chan;
		amc_p->a_op->a_do_channel(amc_p);
		amc_p->a_op->a_cleanup(amc_p);
		pthread_mutex_unlock(&priv_p->p_amc_lck);
		break;

	default:
		nid_log_error("acg_do_channel: got unknown channel type:%d",
			chan_type);
		break;
	}

out:
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)chan_p->a_chan);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)chan_p);
}

static int
acg_is_stop(struct acg_interface *acg_p)
{
	struct acg_private *priv_p = (struct acg_private *)acg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	return drv_p->dr_op->dr_is_stop(drv_p);
}

static void
acg_cleanup(struct acg_interface *acg_p)
{
	struct acg_private *priv_p = (struct acg_private *)acg_p->ag_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct ascg_interface *ascg_p = priv_p->p_ascg;

	mpk_p->m_op->m_cleanup(mpk_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)mpk_p);

	drv_p->dr_op->dr_cleanup(drv_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_drv);

	ascg_p->ag_op->ag_cleanup(ascg_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_ascg);

	pthread_mutex_destroy(&priv_p->p_amc_lck);
   	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
	acg_p->ag_private = NULL;
}

static
void
acg_set_stop(struct acg_interface *acg_p)
{
	struct acg_private *priv_p = (struct acg_private *)acg_p->ag_private;
	priv_p->p_stop = 1;
}

struct acg_operations acg_op = {
	.ag_accept_new_channel = acg_accept_new_channel,
	.ag_do_channel = acg_do_channel,
	.ag_is_stop = acg_is_stop,
	.ag_set_stop = acg_set_stop,
	.ag_cleanup = acg_cleanup,
};

int
acg_initialization(struct acg_interface *acg_p, struct acg_setup *setup)
{
	char *log_header = "acg_initialization";
	struct acg_private *priv_p;
	struct ascg_interface *ascg_p;
	struct ascg_setup ascg_setup;
	struct mpk_setup mpk_setup;
	struct mpk_interface *mpk_p;
	struct drv_setup drv_setup;
	struct drv_interface *drv_p;
	struct pp2_interface *pp2_p = setup->pp2;
	struct pp2_interface *dyn_pp2_p = setup->dyn_pp2;
	struct pp2_interface *dyn2_pp2_p = setup->dyn2_pp2;

	nid_log_notice("%s: start ...", log_header);
	acg_p->ag_op = &acg_op;
	priv_p = (struct acg_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	acg_p->ag_private = priv_p;

	priv_p->p_ini = setup->ini;
	priv_p->p_tp = setup->tp;
	priv_p->p_pp2 = pp2_p;
	priv_p->p_dyn_pp2 = dyn_pp2_p;
	priv_p->p_dyn2_pp2 = dyn2_pp2_p;
	priv_p->p_agent_keys = setup->agent_keys;
	priv_p->p_umpka = setup->umpka;

	mpk_setup.type = 0;
	mpk_setup.pp2 = pp2_p;
	mpk_setup.do_pp2 = 1;
	mpk_p = (struct mpk_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*mpk_p));
	mpk_initialization(mpk_p, &mpk_setup);
	priv_p->p_mpk = mpk_p;

	drv_setup.ascg = NULL;	// not ready yet
	drv_setup.mpk = mpk_p;
	drv_setup.pp2 = pp2_p;
	drv_setup.dyn2_pp2 = dyn2_pp2_p;
	drv_setup.umpka = priv_p->p_umpka;
	drv_p = (struct drv_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*drv_p));
	drv_initialization(drv_p, &drv_setup);
	usleep(10000);
	priv_p->p_drv = drv_p;

	ascg_p = (struct ascg_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*ascg_p));
	ascg_setup.drv = drv_p;
	ascg_setup.mpk = mpk_p;
	ascg_setup.ini = setup->ini;
	ascg_setup.tp = setup->tp;
	ascg_setup.pp2 = pp2_p;
	ascg_setup.dyn_pp2 = dyn_pp2_p;
	ascg_setup.agent_keys = priv_p->p_agent_keys;
	ascg_setup.acg = acg_p;
	ascg_initialization(ascg_p, &ascg_setup);
	priv_p->p_ascg = ascg_p;

	drv_p = ascg_setup.drv;
	drv_p->dr_op->dr_set_ascg(drv_p, ascg_p);

	priv_p->p_hook_flag = 1;
	pthread_mutex_init(&priv_p->p_amc_lck, NULL);

	nid_log_notice("%s: done", log_header);
	return 0;
}
