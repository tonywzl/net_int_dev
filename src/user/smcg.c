/*
 * smcg.c
 * 	Implementation of Server Manager Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "scg_if.h"
#include "smc_if.h"
#include "smcg_if.h"
#include "allocator_if.h"
#include "ini_if.h"

struct smcg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct mpk_interface;
struct smcg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_smc_head;
	struct scg_interface	*p_scg;
	struct mpk_interface	*p_mpk;
	struct allocator_interface	*p_alloc;
	struct ini_interface	*p_ini;
	struct mqtt_interface	*p_mqtt;
};

static void *
smcg_accept_new_channel(struct smcg_interface *smcg_p, int sfd, char *cip)
{
	char *log_header = "smcg_accept_new_channel";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct smc_interface *smc_p;
	struct smc_setup smc_setup;
	struct smcg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	smc_p = x_calloc(1, sizeof(*smc_p));
	smc_setup.smcg = smcg_p;
	smc_setup.mpk = mpk_p;
	smc_setup.ini = priv_p->p_ini;
	smc_setup.mqtt = priv_p->p_mqtt;
	smc_initialization(smc_p, &smc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = smc_p;
	smc_p->s_op->s_accept_new_channel(smc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
smcg_do_channel(struct smcg_interface *smcg_p, void *data)
{
	char *log_header = "smcg_do_channel";
	struct smcg_channel *chan_p = (struct smcg_channel *)data;
	struct smc_interface *smc_p = (struct smc_interface *)chan_p->c_data;
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_smc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	smc_p->s_op->s_do_channel(smc_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	smc_p->s_op->s_cleanup(smc_p);
}

static void
smcg_get_stat(struct smcg_interface *smcg_p, struct list_head *out_head)
{
	char *log_header = "smcg_get_stat";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_debug("%s: start ...", log_header);
	scg_p->sg_op->sg_get_stat(scg_p, out_head);
}

static void
smcg_reset_stat(struct smcg_interface *smcg_p)
{
	char *log_header = "smcg_reset_stat";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_debug("%s: start ...", log_header);
	scg_p->sg_op->sg_reset_stat(scg_p);
}

static void
smcg_update(struct smcg_interface *smcg_p)
{
	char *log_header = "smcg_update";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_warning("%s: start ...", log_header);
	scg_p->sg_op->sg_update(scg_p);
}

static int
smcg_check_conn(struct smcg_interface *smcg_p, char *uuid)
{
	char *log_header = "smcg_update";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_warning("%s: start ...", log_header);
	return scg_p->sg_op->sg_check_conn(scg_p, uuid);
}

static void
smcg_stop(struct smcg_interface *smcg_p)
{
	char *log_header = "smcg_stop";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_warning("%s: start ...", log_header);
	scg_p->sg_op->sg_stop(scg_p);
}

static void
smcg_bio_fast_flush(struct smcg_interface *smcg_p, char *resource)
{
	char *log_header = "smcg_bio_fast_flush";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_warning("%s: start ...", log_header);
	scg_p->sg_op->sg_bio_fast_flush(scg_p, resource);
}

static void
smcg_mem_size(struct smcg_interface *smcg_p)
{
	char *log_header = "smcg_mem_size";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct allocator_interface *alloc_p = priv_p->p_alloc;
	nid_log_warning("%s: start ...", log_header);
	alloc_p->a_op->a_get_size(alloc_p);
}

static void
smcg_bio_stop_fast_flush(struct smcg_interface *smcg_p, char *resource)
{
	char *log_header = "smcg_bio_stop_fast_flush";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_warning("%s: start ...", log_header);
	scg_p->sg_op->sg_bio_stop_fast_flush(scg_p, resource);
}

static void
smcg_bio_vec_start(struct smcg_interface *smcg_p)
{
	char *log_header = "smcg_bio_vec_start";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_warning("%s: start ...", log_header);
	scg_p->sg_op->sg_bio_vec_start(scg_p);
}

static void
smcg_bio_vec_stop(struct smcg_interface *smcg_p)
{
	char *log_header = "smcg_bio_vec_stop";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_warning("%s: start ...", log_header);
	scg_p->sg_op->sg_bio_vec_stop(scg_p);
}

static void
smcg_bio_vec_stat(struct smcg_interface *smcg_p, struct list_head *out_head)
{
	char *log_header = "smcg_bio_vec_stat";
	struct smcg_private *priv_p = (struct smcg_private *)smcg_p->smg_private;
	struct scg_interface *scg_p = priv_p->p_scg;
	nid_log_debug("%s: start ...", log_header);
	scg_p->sg_op->sg_bio_vec_stat(scg_p, out_head);
}

struct smcg_operations smcg_op = {
	.smg_accept_new_channel = smcg_accept_new_channel,
	.smg_do_channel = smcg_do_channel,
	.smg_get_stat = smcg_get_stat,
	.smg_reset_stat = smcg_reset_stat,
	.smg_update = smcg_update,
	.smg_check_conn = smcg_check_conn,
	.smg_stop = smcg_stop,
	.smg_bio_fast_flush = smcg_bio_fast_flush,
	.smg_bio_stop_fast_flush = smcg_bio_stop_fast_flush,
	.smg_bio_vec_start = smcg_bio_vec_start,
	.smg_bio_vec_stop= smcg_bio_vec_stop,
	.smg_bio_vec_stat = smcg_bio_vec_stat,
	.smg_mem_size = smcg_mem_size,
};

int
smcg_initialization(struct smcg_interface *smcg_p, struct smcg_setup *setup)
{
	char *log_header = "smcg_initialization";
	struct smcg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	smcg_p->smg_op = &smcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	smcg_p->smg_private = priv_p;

	priv_p->p_scg = setup->scg;
	priv_p->p_mpk = setup->mpk;
	priv_p->p_alloc = setup->alloc;
	priv_p->p_ini = setup->ini;
	priv_p->p_mqtt = setup->mqtt;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_smc_head);
	return 0;
}
