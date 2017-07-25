/*
 * sacg.c
 * 	Implementation of sacshot Guardian Module
 */

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "list.h"
#include "nid_shared.h"
#include "nid_log.h"
#include "server.h"
#include "ini_if.h"
#include "umpk_sac_if.h"
#include "sac_if.h"
#include "nid.h"
#include "scg_if.h"
#include "sdsg_if.h"
#include "cdsg_if.h"
#include "crcg_if.h"
#include "rwg_if.h"
#include "smc_if.h"
#include "sacg_if.h"
#include "wc_if.h"
#include "bwc_if.h"
#include "rw_if.h"
#include "ds_if.h"
#include "rc_if.h"
#include "lstn_if.h"
#include "tp_if.h"
#include "tpg_if.h"
#include "wcg_if.h"

struct sacg_job {
	struct tp_jobheader	j_header;
	struct sacg_interface	*j_sacg;
};

struct sacg_chain {
	struct list_head	r_list;
	struct list_head	r_head;
	struct tp_interface	*r_tp;
};

struct sacg_private {
	pthread_mutex_t		p_rs_mlck;	// resource lock
	pthread_cond_t		p_rs_cond;
	pthread_mutex_t		p_pickup_lck;
	pthread_cond_t		p_pickup_cond;
	struct list_head	p_chain_head;
	struct list_head	p_free_chain_head;
	struct sacg_chain	p_chains[NID_MAX_RESOURCE];
	struct list_head	p_sac_head;
	struct scg_interface	*p_scg;
	struct sdsg_interface	*p_sdsg;
	struct cdsg_interface	*p_cdsg;
	struct wcg_interface	*p_wcg;
	struct crcg_interface 	*p_crcg;
	struct rwg_interface	*p_rwg;
	struct tp_interface	*p_gtp;
	struct tpg_interface	*p_tpg;
	struct tp_interface	*p_wtp;
	struct ini_interface	**p_ini;
	struct io_interface	**p_all_io;
//	struct ini_key_desc	*p_server_keys;
	struct list_head	p_server_keys;
	struct ini_section_desc	*p_service_sections;
	struct sac_info		p_sac_info[NID_MAX_CHANNELS];
	struct mqtt_interface	*p_mqtt;
	int			p_counter;
	char			p_pickup_new;
};

struct sacg_channel {
	struct list_head	c_list;
	char			s_type;
	void			*c_data;
	struct sacg_interface	*c_super;
};

/*
 * the caller should hold p_rs_mlck
 */
static struct sacg_chain*
__find_chain_by_uuid(struct sacg_private *priv_p, char *target_uuid)
{
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;

	list_for_each_entry(chain_p, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		chan_p = list_first_entry(&chain_p->r_head, struct sacg_channel, c_list);
		sac_p = (struct sac_interface *)chan_p->c_data;
		if (!strcmp(target_uuid, sac_p->sa_op->sa_get_uuid(sac_p))) {
			return chain_p;
		}
	}
	return NULL;
}

static void *
sacg_accept_new_channel(struct sacg_interface *sacg_p, int sfd, char *cip)
{
	char *log_header = "sacg_accept_new_channel";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_interface *sac_p;
	struct sac_setup sac_setup;
	struct sacg_channel *chan_p = NULL;
	int rc;

	nid_log_warning("%s: a new resource channel", log_header);
	sac_setup.sacg = sacg_p;
	sac_setup.sdsg = priv_p->p_sdsg;
	sac_setup.cdsg = priv_p->p_cdsg;
	sac_setup.wcg = priv_p->p_wcg;
	sac_setup.crcg = priv_p->p_crcg;
	sac_setup.rwg = priv_p->p_rwg;
	sac_setup.tpg = priv_p->p_tpg;
	sac_setup.rsfd = sfd;
	sac_setup.all_io = priv_p->p_all_io;
	sac_setup.mqtt = priv_p->p_mqtt;
	sac_p = x_calloc(1, sizeof(*sac_p));
	rc = sac_initialization(sac_p, &sac_setup);
	if (rc) {
		nid_log_error("%s: cannot init sac", log_header);
		goto out;
	}

	/*
	 * setup connection for the new channel
	 */
	rc = sac_p->sa_op->sa_accept_new_channel(sac_p, sfd, cip);
	if (rc) {
		nid_log_error("%s: cannot establish the new channel, s_cleanup ...", log_header);
		sac_p->sa_op->sa_cleanup(sac_p);
		goto out;
	}

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = sac_p;
	chan_p->c_super = sacg_p;

out:
	return chan_p;
}

static void*
_sacg_do_channel(void *data) {

	char *log_header = "_sacg_do_channel";
	struct sacg_channel *chan_p = (struct sacg_channel *)data;
	struct sac_interface *the_sac, *sac_p = (struct sac_interface *)chan_p->c_data;
	struct sacg_private *priv_p = (struct sacg_private *)chan_p->c_super->sag_private;
	struct sacg_channel *the_chan;
	struct sacg_chain *chain_p, *the_chain;
	struct tp_interface *tp_p;
	char *uuid_p, *the_uuid;

	nid_log_warning("%s: start ...", log_header);
	uuid_p = sac_p->sa_op->sa_get_uuid(sac_p);
	chain_p = NULL;
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(the_chain, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		the_chan = list_first_entry(&the_chain->r_head, struct sacg_channel, c_list);
		the_sac = (struct sac_interface *)the_chan->c_data;
		the_uuid = the_sac->sa_op->sa_get_uuid(the_sac);
		if (!strcmp(the_uuid, uuid_p)) {
			nid_log_notice("%s: duplicated rs (uuid:%s) exists", log_header, uuid_p);
			chain_p = the_chain;
			break;
		}
	}

	if (!chain_p) {
		if (list_empty(&priv_p->p_free_chain_head)) {
			nid_log_error("%s: too many channels, no new channel allowed", log_header);
			pthread_mutex_unlock(&priv_p->p_rs_mlck);
			nid_log_warning("%s: before s_do_channel, s_cleanup ...", log_header);
			sac_p->sa_op->sa_cleanup(sac_p);
			goto out;
		}
		chain_p = list_first_entry(&priv_p->p_free_chain_head, struct sacg_chain, r_list);
		INIT_LIST_HEAD(&chain_p->r_head);
		/* the the sac must already have had the tp set when sac_accept_new_channel() */
		tp_p = sac_p->sa_op->sa_get_tp(sac_p);
		assert(tp_p);
		chain_p->r_tp = tp_p;
		list_del(&chain_p->r_list);
		list_add_tail(&chain_p->r_list, &priv_p->p_chain_head);
	}

	list_add_tail(&chan_p->c_list, &chain_p->r_head);
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	sac_p->sa_op->sa_do_channel(sac_p);

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_del(&chan_p->c_list);
	free(chan_p);
	if (list_empty(&chain_p->r_head)) {
		list_del(&chain_p->r_list);
		list_add_tail(&chain_p->r_list, &priv_p->p_free_chain_head);
	}
	pthread_cond_signal(&priv_p->p_rs_cond);
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	sac_p->sa_op->sa_cleanup(sac_p);
	nid_log_warning("%s: after s_do_channel, s_cleanup ...", log_header);

out:
	return NULL;
}

/*
 * Algorithm:
 * 	Call sac do_channel
 * 	Find out the channel(sac) from chain list (p_chain_head) and insert the channel (sac).
 * 	If there is no chain available for this channel, create one chain to contain this channel
 *
 */
static void
sacg_do_channel(struct sacg_interface *sacg_p, void *data)
{
	pthread_t thread_id;
	pthread_attr_t attr;
	int counter = 0;
	struct sacg_channel *chan_p = (struct sacg_channel *)data;

	chan_p->c_super = sacg_p;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	while ((pthread_create(&thread_id, &attr, _sacg_do_channel, data)) < 0) {
		if (++counter > 5)
			assert(0);
		nid_log_error("_tp_extend: pthread_create error %d\n", errno);
		sleep(1);
	}

}

static int
sacg_job_dispatcher(struct sacg_interface *sacg_p)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct tp_interface *tp_p;
	struct sacg_chain *the_chain;
	struct sacg_channel *the_chan;
	struct sac_interface *sac_p;
	struct request_node *rn_p;
	int do_it = 0;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(the_chain, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		tp_p = the_chain->r_tp;
		list_for_each_entry(the_chan, struct sacg_channel, &the_chain->r_head, c_list) {
			sac_p = (struct sac_interface *)the_chan->c_data;
			rn_p = sac_p->sa_op->sa_pickup_request(sac_p);
			if (rn_p) {
				rn_p->r_header.jh_do = sac_p->sa_op->sa_do_request;
				rn_p->r_header.jh_free = NULL;
				tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)rn_p);
				do_it = 1;
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	return do_it;
}

static void
sacg_get_stat(struct sacg_interface *sacg_p, struct list_head *out_head)
{
	char *log_header = "sacg_get_stat";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sacg_chain *the_chain;
	struct sacg_channel *the_chan;
	struct sac_interface *sac_p;
	struct sac_stat *stat_p;
	struct smc_stat_record *sr_p;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(the_chain, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		list_for_each_entry(the_chan, struct sacg_channel, &the_chain->r_head, c_list) {
			sac_p = (struct sac_interface *)the_chan->c_data;
			stat_p = x_calloc(1, sizeof(*stat_p));
			sr_p = x_calloc(1, sizeof(*sr_p));
			sr_p->r_type = NID_REQ_STAT_SAC;
			sr_p->r_data = (void *)stat_p;
			list_add_tail(&sr_p->r_list, out_head);
			sac_p->sa_op->sa_get_stat(sac_p, stat_p);

			nid_log_debug("%s: uuid:%s", log_header, stat_p->sa_uuid);
			nid_log_debug("%s: level:%d", log_header, stat_p->sa_alevel);
			nid_log_debug("%s: s_read_counter:%lu", log_header, stat_p->sa_read_counter);
			nid_log_debug("%s: s_read_ready_counter:%lu", log_header, stat_p->sa_read_ready_counter);
			nid_log_debug("%s: s_read_resp_counter:%lu", log_header, stat_p->sa_read_resp_counter);
			nid_log_debug("%s: s_write_counter:%lu", log_header, stat_p->sa_write_counter);
			nid_log_debug("%s: s_write_ready_counter:%lu", log_header, stat_p->sa_write_ready_counter);
			nid_log_debug("%s: s_write_resp_counter:%lu", log_header, stat_p->sa_write_resp_counter);
			nid_log_debug("%s: s_keepalive_counter:%lu", log_header, stat_p->sa_keepalive_counter);
			nid_log_debug("%s: s_keepalive_resp_counter:%lu", log_header, stat_p->sa_keepalive_resp_counter);
			nid_log_debug("%s: s_data_not_ready_counter:%lu", log_header, stat_p->sa_data_not_ready_counter);
			nid_log_debug("%s: s_recv_sequence:%lu", log_header, stat_p->sa_recv_sequence);
			nid_log_debug("%s: s_wait_sequence:%lu", log_header, stat_p->sa_wait_sequence);
			nid_log_debug("%s: start_tv %lu(secs)+%lu(usecs)",
					log_header, stat_p->sa_start_tv.tv_sec, stat_p->sa_start_tv.tv_usec);
			nid_log_debug("%s: ip: %s", log_header, stat_p->sa_ipaddr);

			if (stat_p->sa_io_type == IO_TYPE_BUFFER) {
				nid_log_debug("%s: io: rtree(nseg:%u, segsz:%u nfree:%u, nused:%u)",
					log_header, stat_p->sa_io_stat.s_rtree_nseg, stat_p->sa_io_stat.s_rtree_segsz,
					stat_p->sa_io_stat.s_rtree_nfree, stat_p->sa_io_stat.s_rtree_nused);
				nid_log_debug("%s: io: rtn(nseg:%u, segsz:%u nfree:%u)",
					log_header, stat_p->sa_io_stat.s_btn_nseg, stat_p->sa_io_stat.s_btn_segsz,
					stat_p->sa_io_stat.s_btn_nfree);
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

}

static void
sacg_request_coming(struct sacg_interface *sacg_p)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	pthread_mutex_lock(&priv_p->p_pickup_lck);
	if (!priv_p->p_pickup_new) {
		priv_p->p_pickup_new = 1;
		pthread_cond_signal(&priv_p->p_pickup_cond);
	}
	pthread_mutex_unlock(&priv_p->p_pickup_lck);
}

static void
sacg_reset_stat(struct sacg_interface *sacg_p)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(chain_p, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
			sac_p = (struct sac_interface *)chan_p->c_data;
			sac_p->sa_op->sa_reset_stat(sac_p);
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
}

static void
sacg_update(struct sacg_interface *sacg_p)
{
	char *log_header = "sacg_update";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct ini_interface *new_ini, *old_ini;
	struct ini_setup ini_setup;

	nid_log_debug("%s: start ...", log_header);
	ini_setup.path = NID_CONF_SERVICE;
//	ini_setup.keys_desc = priv_p->p_server_keys;
	ini_setup.key_set = priv_p->p_server_keys;
	new_ini = x_calloc(1, sizeof(*new_ini));
	ini_initialization(new_ini, &ini_setup);

	/*
	 * Just switch the ini to the new one and destroy the old one.
	 * All the changes will be done by specialized nidmanager commands.
	 */
	old_ini = *(priv_p->p_ini);
	*(priv_p->p_ini) = new_ini;
	old_ini->i_op->i_cleanup(old_ini);
	free((void *)old_ini);
}
/*
 * return:
 * 	1: found the channel
 * 	0: otherwise
 */
static int
sacg_check_conn(struct sacg_interface *sacg_p, char *uuid)
{
	char *log_header = "sacg_check_conn";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;
	int got_it = 0;

	nid_log_debug("%s: start (uuid:%s)...", log_header, uuid);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(chain_p, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
			sac_p = (struct sac_interface *)chan_p->c_data;
			nid_log_debug("sacg_check_conn: comparing %s %s", uuid, sac_p->sa_op->sa_get_uuid(sac_p));
			if (!strcmp(uuid, sac_p->sa_op->sa_get_uuid(sac_p))) {
				got_it = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	return got_it;
}

static int
sacg_upgrade_alevel(struct sacg_interface *sacg_p, struct sac_interface *target)
{
	char *log_header = "sacg_upgrade_alevel";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;
	char *target_uuid;
	int got_it = 0, do_it = 1, rc = 0;
	char target_alevel;
	char doing_stop = 0;
	char *target_aip = NULL;
	char *aip = NULL;
	char *uuid = NULL;

	target_uuid = target->sa_op->sa_get_uuid(target);
	nid_log_debug("%s: start (uuid:%s)...", log_header, target_uuid);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	chain_p = __find_chain_by_uuid(priv_p, target_uuid);
	if (!chain_p) {
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
		nid_log_warning("%s: (uuid:%s): cannot find the chain", log_header, target_uuid);
		return -1;
	}

	target_alevel = target->sa_op->sa_get_alevel(target);
	if (target_alevel >= NID_ALEVEL_MAX) {
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
		return 0;
	}

	target_aip = target->sa_op->sa_get_aip(target);
	list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
		if (chan_p->c_data == target) {
			got_it = 1;
			continue;
		}

		sac_p = (struct sac_interface *)chan_p->c_data;
		aip = sac_p->sa_op->sa_get_aip(sac_p);
		uuid = sac_p->sa_op->sa_get_uuid(sac_p);
		if (!strcmp(aip, target_aip) && !strcmp(uuid, target_uuid)) {
			nid_log_warning("scg_upgrade_alevel(uuid:%s): ipaddress: %s == %s, uuid: %s == %s",
				target_uuid, aip, target_aip, uuid, target_uuid);
			do_it = 0;
			doing_stop = 1;
			break;
		}
		if (sac_p->sa_op->sa_get_alevel(sac_p) > target_alevel) {
			nid_log_warning("%s(uuid:%s): alevel: %d > target_level: %d",
				log_header, target_uuid, sac_p->sa_op->sa_get_alevel(sac_p), target_alevel);
			do_it = 0;
			if (sac_p->sa_op->sa_get_stop_state(sac_p))
				doing_stop = 1;

			break;
		}
	}
	nid_log_warning("%s: target_uuid:%s, target_alevel:%d, got_it:%d, do_it:%d",
		log_header, target_uuid, target_alevel, got_it, do_it);

	if (got_it && do_it)
		target->sa_op->sa_upgrade_alevel(target);
	else if(doing_stop)
		rc = -2;
	else
		rc = -1;
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	return rc;
}

static int
sacg_fupgrade_alevel(struct sacg_interface *sacg_p, struct sac_interface *target)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;
	char *target_uuid;
	int got_it = 0, do_it = 1, rc = 0;
	char target_alevel;

	target_uuid = target->sa_op->sa_get_uuid(target);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	chain_p = __find_chain_by_uuid(priv_p, target_uuid);
	if (!chain_p) {
		rc = -1;
		goto out;
	}

	target_alevel = target->sa_op->sa_get_alevel(target);
	if (target_alevel >= NID_ALEVEL_MAX)
		goto out;

checking:
	do_it = 1;
	list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
		if (chan_p->c_data == target) {
			got_it = 1;
			continue;
		}
		if (do_it) {
			sac_p = (struct sac_interface *)chan_p->c_data;
			if (sac_p->sa_op->sa_get_alevel(sac_p) > target_alevel)
				do_it = 0;
		}
	}

	if (!got_it) {
		rc = -1;
		goto out;
	}

	if (!do_it) {
		sac_p->sa_op->sa_stop(sac_p);
		pthread_cond_wait(&priv_p->p_rs_cond, &priv_p->p_rs_mlck);
#if 0
		while (_get_alevel_checking(chain_p, sac_p) > target_alevel) {
			/*
			 * tricky: need stop it again since sac_p could be a new channel
			 */
			sac_p->sa_op->sa_stop(sac_p);
			pthread_cond_wait(&priv_p->p_rs_cond, &priv_p->p_rs_mlck);
		}
#endif

		/*
		 * need checking again since other channel may jump in while we released p_rs_mlck
		 */
		goto checking;
	}

	target->sa_op->sa_upgrade_alevel(target);

out:
	nid_log_warning("sacg_fupgrade_alevel: target_uuid:%s, target_alevel:%d, got_it:%d, do_it:%d",
		target_uuid, target_alevel, got_it, do_it);

	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	return rc;
}

static void
sacg_stop(struct sacg_interface *sacg_p)
{
	char *log_header = "sacg_stop";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;

	nid_log_notice("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(chain_p, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
			sac_p = (struct sac_interface *)chan_p->c_data;
			sac_p->sa_op->sa_stop(sac_p);
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
}

static struct sac_interface *
sacg_get_sac(struct sacg_interface *sacg_p, char *chan_uuid)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p, *sac_ret = NULL;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(chain_p, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
			sac_p = (struct sac_interface *)chan_p->c_data;
			if (!strcmp(sac_p->sa_op->sa_get_uuid(sac_p), chan_uuid)) {
				sac_ret = sac_p;
				break;
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	return sac_ret;
}

static int
sacg_add_sac(struct sacg_interface *sacg_p, char *chan_uuid, char sync, char direct_io, char enable_kill_myself,
		int alignment, char *ds_name, char *dev_name, char *exportname, uint32_t dev_size, char *tp_name)
{
	char *log_header = "sacg_add_sac";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_info *sac_info_p = &priv_p->p_sac_info[0];
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	struct cdsg_interface *cdsg_p = priv_p->p_cdsg;
	struct wcg_interface *wcg_p = priv_p->p_wcg;
	struct crcg_interface *crcg_p = priv_p->p_crcg;
	struct rwg_interface *rwg_p = priv_p->p_rwg;
	struct rw_interface *rw_p = NULL;
	struct rc_interface *rc_p = NULL;
	struct wc_interface *wc_p = NULL;
	struct ds_interface *ds_p = NULL;
	char *wc_uuid, *rc_uuid;
	int io_type, ds_type, rc_type, wc_type;
	void *rc_handle = NULL;
	int new_worker;
	struct wc_channel_info wc_info;
	struct rc_channel_info rc_info;
	int i, got_it = 0, rc = 1, empty_index = -1;

	nid_log_debug("%s: start...", log_header);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	for (i = 0; i < NID_MAX_CHANNELS; i++, sac_info_p++) {
		if (!strcmp(chan_uuid, sac_info_p->uuid)) {
			got_it = 1;
			break;
		}

		if (empty_index == -1 && sac_info_p->uuid[0] == '\0') {
			// fill in the name at the first place as place holder
			strcpy(sac_info_p->uuid, chan_uuid);
			priv_p->p_counter++;
			empty_index = i;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	if (got_it) {
		nid_log_warning("%s: sac already exists (chan_uuid:%s)", log_header, chan_uuid);
		goto out;
	}
	if (empty_index == -1) {
		nid_log_warning("%s: too many sac (chan_uuid:%s)", log_header, chan_uuid);
		goto out;
	}

	/*
	 * Set alignment
	 */
	if (rwg_p)
		rw_p = rwg_p->rwg_op->rwg_search_and_create(rwg_p, dev_name);

	if (!rw_p) {
		nid_log_warning("%s: according rw does not exist (rw_name:%s)", log_header, dev_name);
		goto out;
	}

	ds_type = DS_TYPE_NONE;
	if (sdsg_p)
		ds_p = sdsg_p->dg_op->dg_search_and_create_sds(sdsg_p, ds_name);
	if (ds_p)
		ds_type = DS_TYPE_SPLIT;
	else {
		if (cdsg_p)
			ds_p = cdsg_p->dg_op->dg_search_and_create_cds(cdsg_p, ds_name);
		if (ds_p)
			ds_type = DS_TYPE_CYCLE;
	}
	if (ds_type == DS_TYPE_NONE) {
		nid_log_warning("%s: according ds does not exist (ds_name:%s)", log_header, ds_name);
		goto out;
	}

	rc_uuid = ds_p->d_op->d_get_rc_name(ds_p);
	rc_type = RC_TYPE_NONE;
	if (rc_uuid[0] != '\0') {
		if (crcg_p) {
			rc_p = crcg_p->rg_op->rg_search_and_create_crc(crcg_p, rc_uuid);
			rc_type = RC_TYPE_CAS;
		}
		if (rc_type == RC_TYPE_NONE) {
			nid_log_warning("%s: according rc does not exist (rc_uuid:%s)", log_header, rc_uuid);
			goto out;
		}
	}

	if (rc_p) {
		rc_info.rc_rw = rw_p;
		rc_handle = rc_p->rc_op->rc_create_channel(rc_p, NULL, &rc_info, chan_uuid, &new_worker);
		/*
		 * This is online sac adding. There can be inactive rc channel added already by previous failed command.
		 * For idempotence, the rc create channel request doesn't have to be the first one.
		 */
//		assert(new_worker);
	}

	wc_uuid = ds_p->d_op->d_get_wc_name(ds_p);
	wc_type = WC_TYPE_NONE;
	if (wc_uuid[0] != '\0') {

		if (wcg_p)
			wc_p = wcg_p->wg_op->wg_search_and_create_wc(wcg_p, wc_uuid);

		if (wc_p)
			wc_type = wc_p->wc_op->wc_get_type(wc_p);

		if (wc_type == WC_TYPE_NONE) {
			nid_log_warning("%s: according wc does not exist (wc_uuid:%s)", log_header, wc_uuid);
			goto out;
		}
	}

	if (wc_p) {
		wc_info.wc_rw = rw_p;
		wc_info.wc_rc = rc_p;
		wc_info.wc_rc_handle = rc_handle;
		wc_info.wc_rw_exportname = exportname;
		wc_info.wc_rw_sync = sync;
		wc_info.wc_rw_direct_io = direct_io;
		wc_info.wc_rw_alignment = alignment;
		wc_info.wc_rw_dev_size = dev_size;
		wc_p->wc_op->wc_create_channel(wc_p, NULL, &wc_info, chan_uuid, &new_worker);
		/*
		 * This is online sac adding. There can be inactive wc channel added already by previous failed command.
		 * For idempotence, the wc create channel request doesn't have to be the first one.
		 */
//		assert(new_worker);
	}

	if (!wc_p)
		io_type = IO_TYPE_RESOURCE;
	else
		io_type = IO_TYPE_BUFFER;

	rc = 0;

out:
	if (!rc) {
		sac_info_p = &priv_p->p_sac_info[empty_index];
		sac_info_p->sync = sync;
		sac_info_p->direct_io = direct_io;
		sac_info_p->enable_kill_myself = enable_kill_myself;
		sac_info_p->alignment = alignment;
		strcpy(sac_info_p->ds_name, ds_name);
		strcpy(sac_info_p->dev_name, dev_name);
		strcpy(sac_info_p->wc_uuid, wc_uuid);
		strcpy(sac_info_p->rc_uuid, rc_uuid);
		sac_info_p->io_type = io_type;
		sac_info_p->wc_type = wc_type;
		sac_info_p->rc_type = rc_type;
		sac_info_p->ds_type = ds_type;
		strcpy(sac_info_p->export_name, exportname);
		sac_info_p->dev_size = dev_size;
		strcpy(sac_info_p->tp_name, tp_name);
		sac_info_p->ready = 1;
	} else {
		// clear the name as place holder if failed.
		pthread_mutex_lock(&priv_p->p_rs_mlck);
		sac_info_p->uuid[0] = '\0';
		priv_p->p_counter--;
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
	}

	return rc;
}

static int
sacg_delete_sac(struct sacg_interface *sacg_p, char *chan_uuid)
{
	char *log_header = "sacg_delete_sac";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_info *sac_info_p = &priv_p->p_sac_info[0];
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;
	struct wc_interface *wc_p = NULL;
	int i, got_it = 0;

	nid_log_warning("%s: start (resource uuid:%s) ...", log_header, chan_uuid);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	for (i = 0; i < NID_MAX_CHANNELS; i++, sac_info_p++) {
		if (!strcmp(chan_uuid, sac_info_p->uuid)) {
			sac_info_p->uuid[0] = '\0';
			priv_p->p_counter--;
			got_it = 1;
			break;
		}
	}
	if (!got_it)
		goto out;

	got_it = 0;
	list_for_each_entry(chain_p, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		/* the target chain already found and processed */
		if (got_it) {
			goto out;
		}

		list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
			sac_p = (struct sac_interface *)chan_p->c_data;
			if (!got_it && !strcmp(sac_p->sa_op->sa_get_uuid(sac_p), chan_uuid)) {
				got_it = 1;
				wc_p = sac_p->sa_op->sa_get_wc(sac_p);
			}

			if (got_it)
				sac_p->sa_op->sa_stop(sac_p);
		}
	}
out:
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	if (!got_it)
		nid_log_warning("%s: no uuid matched (uuid:%s)", log_header, chan_uuid);

	/* Even got_it, wc_p is still not guaranteed to be non-empty (e.g. rio). */
	if (wc_p)
		wc_p->wc_op->wc_destroy_chain(wc_p, chan_uuid);

	return 0;
}

static int
sacg_set_keepalive_sac(struct sacg_interface *sacg_p, char *chan_uuid, uint16_t enable_keepalive, uint16_t max_keepalive)
{
        char *log_header = "sacg_set_keepalive_sac";
        struct sac_interface *sac_p;

        sac_p = sacg_get_sac(sacg_p, chan_uuid);
        if (!sac_p) {
                nid_log_warning("%s: no uuid matched (uuid:%s)", log_header, chan_uuid);
                return 1;
        }

        if(enable_keepalive) {
                sac_p->sa_op->sa_enable_int_keep_alive(sac_p, max_keepalive);
        } else {
                sac_p->sa_op->sa_disable_int_keep_alive(sac_p);
        }

        return 0;
}

static int
sacg_switch_sac(struct sacg_interface *sacg_p, char *chan_uuid, char *wc_uuid, int wc_type)
{
	char *log_header = "sacg_switch_wc";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_info *sac_info_p = &priv_p->p_sac_info[0];
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;
	struct wc_interface *wc_p;
	int i, rc = 1, got_it = 0;

	nid_log_warning("%s: start (resource uuid:%s) ...", log_header, chan_uuid);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	for (i = 0; i < NID_MAX_CHANNELS; i++, sac_info_p++) {
		if (!strcmp(chan_uuid, sac_info_p->uuid)) {
			got_it = 1;
			break;
		}
	}
	if (!got_it)
		goto out;

	got_it = 0;
	/*
	 * This sac may haven't accepted any connection yet thus not in the chain.
	 * Consider it as successful for this case.
	 */
	rc = 0;
	list_for_each_entry(chain_p, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		/* the target chain already found and processed */
		if (got_it)
			goto out;

		list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
			sac_p = (struct sac_interface *)chan_p->c_data;
			if (!got_it && !strcmp(sac_p->sa_op->sa_get_uuid(sac_p), chan_uuid))
				got_it = 1;

			if (got_it) {
				wc_p = sac_p->sa_op->sa_get_wc(sac_p);
				/*
				 * The old bwc stuff for this channel specific should have already been cleaned up and removed.
				 * Just check if wc_p is NULL to decide whether to switch the sac to new wc.
				 */
//					if (strcmp(wc_p->wc_op->wc_get_uuid(wc_p), wc_uuid)) {
				if (!wc_p)
					rc = sac_p->sa_op->sa_switch_wc(sac_p, wc_uuid, wc_type);
				else {
					nid_log_warning("%s: sac:%s is still using old wc:%s",
							log_header, chan_uuid, wc_uuid);
					rc = 1;
				}
				break;
			}
		}
	}

	if (!rc)
		strcpy(sac_info_p->wc_uuid, wc_uuid);
out:
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	return rc;
}

struct sac_info *
sacg_get_sac_info(struct sacg_interface *sacg_p, char *chan_uuid)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_info *sac_info_p = &priv_p->p_sac_info[0], *ret_sac_info_p = NULL;
	int i;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	for (i = 0; i < NID_MAX_CHANNELS; i++, sac_info_p++) {
		if (!strcmp(chan_uuid, sac_info_p->uuid)) {
			ret_sac_info_p = sac_info_p;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	return ret_sac_info_p;
}

struct sac_info *
sacg_get_all_sac_info(struct sacg_interface *sacg_p, int *num_sac)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_info *sac_info_p = &priv_p->p_sac_info[0];

	*num_sac = priv_p->p_counter;
	return sac_info_p;
}

char **
sacg_get_working_sac_name(struct sacg_interface *sacg_p, int *num_sac)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sacg_chain *chain_p;
	struct sacg_channel *chan_p;
	struct sac_interface *sac_p;
	char **ret;
	int i = 0;

	ret = x_calloc(priv_p->p_counter, sizeof(char *));
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(chain_p, struct sacg_chain, &priv_p->p_chain_head, r_list) {
		list_for_each_entry(chan_p, struct sacg_channel, &chain_p->r_head, c_list) {
			sac_p = (struct sac_interface *)chan_p->c_data;
			ret[i] = sac_p->sa_op->sa_get_uuid(sac_p);
			i++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	*num_sac = i;

	return ret;
}

static int
sacg_start_fast_release(struct sacg_interface *sacg_p, char *res_uuid)
{
	char *log_header = "sacg_start_fast_release";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_interface *sac_p = NULL;
	struct wc_interface *wc_p;
	int rc = -1;

	nid_log_warning("%s: start ...", log_header);
	sac_p = sacg_get_sac(sacg_p, res_uuid);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	if (sac_p == NULL)
		goto out;
	wc_p = sac_p->sa_op->sa_get_wc(sac_p);
	if (wc_p == NULL)
		goto out;
	rc = wc_p->wc_op->wc_dropcache_start(wc_p, res_uuid, 0);

out:
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	return rc;
}

static int
sacg_stop_fast_release(struct sacg_interface *sacg_p, char *res_uuid)
{
	char *log_header = "sacg_stop_fast_release";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_interface *sac_p = NULL;
	struct wc_interface *wc_p;
	int rc = -1;

	nid_log_warning("%s: start ...", log_header);
	sac_p = sacg_get_sac(sacg_p, res_uuid);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	if (sac_p == NULL)
		goto out;
	wc_p = sac_p->sa_op->sa_get_wc(sac_p);
	if (wc_p == NULL)
		goto out;
	rc = wc_p->wc_op->wc_dropcache_stop(wc_p, res_uuid);

out:
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	return rc;
}

static int
sacg_get_counter(struct sacg_interface *sacg_p, char *res_uuid, uint32_t *bfe_page_counter, uint32_t *bre_page_counter)
{
	char *log_header = "sacg_get_counter";
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	struct sac_interface *sac_p = NULL;
	struct wc_interface *wc_p;
	int rc = -1;

	nid_log_warning("%s: start ...", log_header);
	sac_p = sacg_get_sac(sacg_p, res_uuid);
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	if (sac_p == NULL)
		goto out;
	wc_p = sac_p->sa_op->sa_get_wc(sac_p);
	if (wc_p == NULL)
		goto out;
	*bfe_page_counter = wc_p->wc_op->wc_get_flush_page_counter(wc_p);
	*bre_page_counter = wc_p->wc_op->wc_get_release_page_counter(wc_p);
	rc = 0;
out:
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	return rc;
}

static int
sacg_get_list_stat(struct sacg_interface *sacg_p, char *res_uuid, struct umessage_sac_list_stat_resp *list_stat)
{
	char *log_header = "sacg_get_list_stat";
	struct sac_interface *sac_p = NULL;
	int rc = -1;

	nid_log_warning("%s: start ...", log_header);
	sac_p = sacg_get_sac(sacg_p, res_uuid);
	if (sac_p == NULL)
		return rc;
	sac_p->sa_op->sa_get_list_stat(sac_p, list_stat);
	rc = 0;

	return rc;
}

struct sacg_operations sacg_op = {
	.sag_accept_new_channel = sacg_accept_new_channel,
	.sag_do_channel = sacg_do_channel,
	.sag_job_dispatcher = sacg_job_dispatcher,
	.sag_get_stat = sacg_get_stat,
	.sag_request_coming = sacg_request_coming,
	.sag_reset_stat = sacg_reset_stat,
	.sag_update = sacg_update,
	.sag_check_conn = sacg_check_conn,
	.sag_upgrade_alevel = sacg_upgrade_alevel,
	.sag_fupgrade_alevel = sacg_fupgrade_alevel,
	.sag_stop = sacg_stop,
	.sag_add_sac = sacg_add_sac,
	.sag_delete_sac = sacg_delete_sac,
	.sag_switch_sac = sacg_switch_sac,
	.sag_get_sac_info = sacg_get_sac_info,
	.sag_set_keepalive_sac = sacg_set_keepalive_sac,
	.sag_get_all_sac_info = sacg_get_all_sac_info,
	.sag_get_working_sac_name = sacg_get_working_sac_name,
	.sag_get_sac = sacg_get_sac,
	.sag_start_fast_release = sacg_start_fast_release,
	.sag_stop_fast_release = sacg_stop_fast_release,
	.sag_get_counter = sacg_get_counter,
	.sag_get_list_stat = sacg_get_list_stat,
};

static int
__sacg_add_all_sac(struct sacg_private *priv_p)
{
	char *log_header = "__sacg_add_all_sac";
	struct ini_interface *ini_p = *priv_p->p_ini;
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	struct cdsg_interface *cdsg_p = priv_p->p_cdsg;
	struct wcg_interface *wcg_p = priv_p->p_wcg;
	struct crcg_interface *crcg_p = priv_p->p_crcg;
	struct rwg_interface *rwg_p = priv_p->p_rwg;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct rw_interface *rw_p;
	struct rc_interface *rc_p;
	struct wc_interface *wc_p;
	struct ds_interface *ds_p;
	struct sac_info *sac_info_p;
	int sac_counter = 0;

	nid_log_warning("%s: start...", log_header);
	sac_info_p = &priv_p->p_sac_info[0];
	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		struct ini_key_content *kc_p;
		int io_type, ds_type, rc_type, rw_sync, rw_direct, wc_type, dev_sz = 0, enable_kill_myself;
		char *exportname;
		void *rc_handle = NULL;
		int new_worker;
		char *ds_name, *dev_name, *rc_uuid, *wc_uuid, *tp_name;
		struct wc_channel_info wc_info;
		struct rc_channel_info rc_info;
		char *chan_uuid = sc_p->s_name;
		rc_p = NULL;
		wc_p = NULL;
		ds_p = NULL;
		rw_p = NULL;

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "sac"))
			continue;

		kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "sync");
		rw_sync = *((int *)kc_p->k_value);

		kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "direct_io");
		rw_direct = *((int *)kc_p->k_value);

		kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "enable_kill_myself");
		enable_kill_myself = *((int *)kc_p->k_value);

		kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "tp_name");
		tp_name = (char *)(kc_p->k_value);
		/*
		 * Set alignment
		 */
		kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "alignment");
		int alignment = *(int *)(kc_p->k_value);
		if (((~(alignment - 1)) & alignment) != alignment) {
			nid_log_error("Alignment must be 2^n.");
			return -1;
		}

		kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "dev_name");
		dev_name = (char *)kc_p->k_value;
		if (rwg_p)
			rw_p = rwg_p->rwg_op->rwg_search_and_create(rwg_p, dev_name);
		if (!rw_p) {
			nid_log_warning("%s: according rw does not exist (rw_name:%s)", log_header, dev_name);
			return -1;
		}

		exportname = rw_p->rw_op->rw_get_exportname(rw_p);
		if (exportname[0] == '\0') {
			/* mrw case */
			kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "volume_uuid");
			exportname = (char *)kc_p->k_value;
			kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "dev_size");
			dev_sz = *(uint32_t *)kc_p->k_value;
			nid_log_warning("%s: mrw (rw_name:%s, volume_uuid:%s, dev_size:%u)",
					log_header, dev_name, exportname, dev_sz);
		} else {
			/* drw case */
			nid_log_warning("%s: drw (rw_name:%s, exportname:%s)",
					log_header, dev_name, exportname);
		}

		kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "ds_name");
		ds_name = (char *)kc_p->k_value;
		ds_type = DS_TYPE_NONE;
		if (sdsg_p)
			ds_p = sdsg_p->dg_op->dg_search_and_create_sds(sdsg_p, ds_name);
		if (ds_p)
			ds_type = DS_TYPE_SPLIT;
		else {
			if (cdsg_p)
				ds_p = cdsg_p->dg_op->dg_search_and_create_cds(cdsg_p, ds_name);
			if (ds_p)
				ds_type = DS_TYPE_CYCLE;
		}
		if (ds_type == DS_TYPE_NONE) {
			nid_log_warning("%s: according ds does not exist (ds_name:%s)", log_header, ds_name);
			return -1;
		}

		rc_uuid = ds_p->d_op->d_get_rc_name(ds_p);
		rc_type = RC_TYPE_NONE;
		if (rc_uuid[0] != '\0') {
			if (crcg_p) {
				rc_p = crcg_p->rg_op->rg_search_and_create_crc(crcg_p, rc_uuid);
				rc_type = RC_TYPE_CAS;
			}
			if (rc_type == RC_TYPE_NONE) {
				nid_log_warning("%s: according rc does not exist (rc_uuid:%s)", log_header, rc_uuid);
				return -1;
			}
		}

		if (rc_p) {
			rc_info.rc_rw = rw_p;
			rc_handle = rc_p->rc_op->rc_create_channel(rc_p, NULL, &rc_info, chan_uuid, &new_worker);
			assert(new_worker);
		}

		wc_uuid = ds_p->d_op->d_get_wc_name(ds_p);
		wc_type = WC_TYPE_NONE;
		if (wc_uuid[0] != '\0') {

			if (wcg_p)
				wc_p = wcg_p->wg_op->wg_search_and_create_wc(wcg_p, wc_uuid);

			if (wc_p)
				wc_type = wc_p->wc_op->wc_get_type(wc_p);

			if (wc_type == WC_TYPE_NONE) {
				nid_log_warning("%s: according wc does not exist (wc_uuid:%s)", log_header, wc_uuid);
				return -1;
			}
		}

		if (wc_p) {
			wc_info.wc_rw = rw_p;
			wc_info.wc_rc = rc_p;
			wc_info.wc_rc_handle = rc_handle;
			wc_info.wc_rw_exportname = exportname;
			wc_info.wc_rw_sync = rw_sync;
			wc_info.wc_rw_direct_io = rw_direct;
			wc_info.wc_rw_alignment = alignment;
			wc_info.wc_rw_dev_size = dev_sz;
			wc_p->wc_op->wc_create_channel(wc_p, NULL, &wc_info, chan_uuid, &new_worker);
			assert(new_worker);
		}

		if (!wc_p)
			io_type = IO_TYPE_RESOURCE;
		else
			io_type = IO_TYPE_BUFFER;

		strcpy(sac_info_p->uuid, chan_uuid);
		sac_info_p->sync = (char)rw_sync;
		sac_info_p->direct_io = (char)rw_direct;
		sac_info_p->enable_kill_myself = (char)enable_kill_myself;
		sac_info_p->alignment = alignment;
		strcpy(sac_info_p->ds_name, ds_name);
		strcpy(sac_info_p->dev_name, dev_name);
		strcpy(sac_info_p->wc_uuid, wc_uuid);
		strcpy(sac_info_p->rc_uuid, rc_uuid);
		sac_info_p->io_type = io_type;
		sac_info_p->wc_type = wc_type;
		sac_info_p->rc_type = rc_type;
		sac_info_p->ds_type = ds_type;
		strcpy(sac_info_p->export_name, exportname);
		sac_info_p->dev_size = dev_sz;
		strcpy(sac_info_p->tp_name, tp_name);
		sac_info_p->ready = 1;

		sac_info_p++;
		sac_counter++;
	}
	priv_p->p_counter = sac_counter;

	return 0;
}

static void
__sacg_housekeeping(struct tp_jobheader *jh_p)
{
	char *log_header = "sacg_housekeeping";
	struct sacg_job *job = (struct sacg_job *)jh_p;
	struct sacg_interface *sacg_p = job->j_sacg;
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	int do_it, nothing_to_do = 0;

	nid_log_debug("%s: start ...", log_header);
next_round:
	do_it = sacg_p->sag_op->sag_job_dispatcher(sacg_p);

	if (!do_it)
		nothing_to_do++;
	else
		nothing_to_do = 0;

	if (nothing_to_do > 50) {
		nothing_to_do = 0;
		pthread_mutex_lock(&priv_p->p_pickup_lck);
		while (!priv_p->p_pickup_new) {
			pthread_cond_wait(&priv_p->p_pickup_cond, &priv_p->p_pickup_lck);
		}
		priv_p->p_pickup_new = 0;
		pthread_mutex_unlock(&priv_p->p_pickup_lck);
	}

	goto next_round;
}

static void
__free_sacg_job(struct tp_jobheader *jh_p)
{
	free(jh_p);
}

/*
static void
__sacg_request_coming(struct sacg_interface *sacg_p)
{
	struct sacg_private *priv_p = (struct sacg_private *)sacg_p->sag_private;
	pthread_mutex_lock(&priv_p->p_pickup_lck);
	if (!priv_p->p_pickup_new) {
		priv_p->p_pickup_new = 1;
		pthread_cond_signal(&priv_p->p_pickup_cond);
	}
	pthread_mutex_unlock(&priv_p->p_pickup_lck);
}
*/
int
sacg_initialization(struct sacg_interface *sacg_p, struct sacg_setup *setup)
{
	char *log_header = "sacg_initialization";
	struct sacg_private *priv_p;
	struct sacg_chain *chain_p;
	struct tpg_interface *tpg_p;
	struct sacg_job *job_p;
	int i, rc;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	sacg_p->sag_op = &sacg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	sacg_p->sag_private = priv_p;

	priv_p->p_scg = setup->scg;
	priv_p->p_sdsg = setup->sdsg;
	priv_p->p_cdsg = setup->cdsg;
	priv_p->p_wcg = setup->wcg;
	priv_p->p_crcg = setup->crcg;
	priv_p->p_rwg = setup->rwg;
	priv_p->p_wtp = setup->wtp;
	priv_p->p_ini = setup->ini;
	priv_p->p_all_io = setup->all_io;
//	priv_p->p_server_keys = setup->server_keys;
	priv_p->p_server_keys = setup->server_keys;
	priv_p->p_mqtt = setup->mqtt;

	tpg_p = setup->tpg;
	priv_p->p_gtp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, NID_GLOBAL_TP);
	priv_p->p_tpg = tpg_p;

	pthread_mutex_init(&priv_p->p_rs_mlck, NULL);
	pthread_cond_init(&priv_p->p_rs_cond, NULL);
	pthread_mutex_init(&priv_p->p_pickup_lck, NULL);
	pthread_cond_init(&priv_p->p_pickup_cond, NULL);
	INIT_LIST_HEAD(&priv_p->p_chain_head);
	INIT_LIST_HEAD(&priv_p->p_free_chain_head);
	INIT_LIST_HEAD(&priv_p->p_sac_head);

	chain_p = priv_p->p_chains;
	for (i = 0; i < NID_MAX_RESOURCE; i++, chain_p++) {
		list_add_tail(&chain_p->r_list, &priv_p->p_free_chain_head);
	}

	rc = __sacg_add_all_sac(priv_p);
	assert(!rc);

	job_p = x_calloc(1, sizeof(*job_p));
	job_p->j_header.jh_do = __sacg_housekeeping;
	job_p->j_header.jh_free = __free_sacg_job;
	job_p->j_sacg = sacg_p;
	if (priv_p->p_gtp)
		priv_p->p_gtp->t_op->t_job_insert( priv_p->p_gtp, (struct tp_jobheader *)job_p);

	return 0;
}
