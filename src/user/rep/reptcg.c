/*
 * reptcg.c
 *	Implementation of Replication Target Channel Guaridan Module
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "reptc_if.h"
#include "reptcg_if.h"

struct reptcg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct reptcg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_reptc_head;
	struct reptg_interface	*p_reptg;
	struct umpk_interface	*p_umpk;
};

static void *
reptcg_accept_new_channel(struct reptcg_interface *reptcg_p, int sfd, char *cip)
{
	char *log_header = "reptcg_accept_new_channel";
	struct reptcg_private *priv_p = (struct reptcg_private *)reptcg_p->r_private;
	struct reptc_interface *reptc_p;
	struct reptc_setup reptc_setup;
	struct reptcg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	reptc_p = calloc(1, sizeof(*reptc_p));
	reptc_setup.umpk = priv_p->p_umpk;
	reptc_setup.reptg = priv_p->p_reptg;
	reptc_initialization(reptc_p, &reptc_setup);
	
	chan_p = calloc(1, sizeof(chan_p));
	chan_p->c_data = reptc_p;
	reptc_p->r_op->rtc_accept_new_channel(reptc_p, sfd);

	return chan_p;
}

static void
reptcg_do_channel(struct reptcg_interface *reptcg_p, void *data)
{
	char *log_header = "reptcg_do_channel";
	struct reptcg_channel *chan_p = (struct reptcg_channel *)data;
	struct reptc_interface *reptc_p = (struct reptc_interface *)chan_p->c_data;
	struct reptcg_private *priv_p = (struct reptcg_private *)reptcg_p->r_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_reptc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	reptc_p->r_op->rtc_do_channel(reptc_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	reptc_p->r_op->rtc_cleanup(reptc_p);
}

struct reptcg_operations reptcg_op = {
	.rtcg_accept_new_channel = reptcg_accept_new_channel,
	.rtcg_do_channel = reptcg_do_channel,
};

int
reptcg_initialization(struct reptcg_interface *reptcg_p, struct reptcg_setup *setup)
{
	char *log_header = "reptcg_initialization";
	struct reptcg_private *priv_p;

	nid_log_warning("%s: start ...", log_header);

	reptcg_p->r_op = &reptcg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	reptcg_p->r_private = priv_p;

	priv_p->p_umpk = setup->umpk;
	priv_p->p_reptg = setup->reptg;

	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_reptc_head);
	return 0;
}
