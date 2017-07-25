/*
 * repscg.c
 *	Implementation of Replication Source Channel Guaridan Module
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "repsc_if.h"
#include "repscg_if.h"

struct repscg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct repscg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_repsc_head;
	struct repsg_interface	*p_repsg;
	struct umpk_interface	*p_umpk;
};

static void *
repscg_accept_new_channel(struct repscg_interface *repscg_p, int sfd, char *cip)
{
	char *log_header = "repscg_accept_new_channel";
	struct repscg_private *priv_p = (struct repscg_private *)repscg_p->r_private;
	struct repsc_interface *repsc_p;
	struct repsc_setup repsc_setup;
	struct repscg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	repsc_p = calloc(1, sizeof(*repsc_p));
	repsc_setup.umpk = priv_p->p_umpk;
	repsc_setup.repsg = priv_p->p_repsg;
	repsc_initialization(repsc_p, &repsc_setup);
	
	chan_p = calloc(1, sizeof(chan_p));
	chan_p->c_data = repsc_p;
	repsc_p->r_op->rsc_accept_new_channel(repsc_p, sfd);

	return chan_p;
}

static void
repscg_do_channel(struct repscg_interface *repscg_p, void *data)
{
	char *log_header = "repscg_do_channel";
	struct repscg_channel *chan_p = (struct repscg_channel *)data;
	struct repsc_interface *repsc_p = (struct repsc_interface *)chan_p->c_data;
	struct repscg_private *priv_p = (struct repscg_private *)repscg_p->r_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_repsc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	repsc_p->r_op->rsc_do_channel(repsc_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	repsc_p->r_op->rsc_cleanup(repsc_p);
}

struct repscg_operations repscg_op = {
	.rscg_accept_new_channel = repscg_accept_new_channel,
	.rscg_do_channel = repscg_do_channel,
};

int
repscg_initialization(struct repscg_interface *repscg_p, struct repscg_setup *setup)
{
	char *log_header = "repscg_initialization";
	struct repscg_private *priv_p;

	nid_log_warning("%s: start ...", log_header);

	repscg_p->r_op = &repscg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	repscg_p->r_private = priv_p;

	priv_p->p_umpk = setup->umpk;
	priv_p->p_repsg = setup->repsg;

	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_repsc_head);
	return 0;
}
