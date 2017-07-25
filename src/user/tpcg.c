/*
 * tpcg.c
 *	Implementation of Thread Pool Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "tpc_if.h"
#include "tpcg_if.h"

struct tpcg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct tpcg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_tpc_head;
	struct tpg_interface	*p_tpg;
	struct umpk_interface	*p_umpk;
};

static void *
tpcg_accept_new_channel(struct tpcg_interface *tpcg_p, int sfd, char *cip)
{
	char *log_header = "tpcg_accept_new_channel";
	struct tpcg_private *priv_p = (struct tpcg_private *)tpcg_p->tcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct tpc_interface *tpc_p;
	struct tpc_setup tpc_setup;
	struct tpcg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	tpc_p = x_calloc(1, sizeof(*tpc_p));
	tpc_setup.tpg = priv_p->p_tpg;
	tpc_setup.umpk = umpk_p;
	tpc_initialization(tpc_p, &tpc_setup);

	chan_p = x_calloc(1, sizeof(chan_p));
	chan_p->c_data = tpc_p;
	tpc_p->t_op->t_accept_new_channel(tpc_p, sfd);

	return chan_p;
}

static void
tpcg_do_channel(struct tpcg_interface *tpcg_p, void *data, struct scg_interface *scg_p)
{
	char *log_header = "tpcg_do_channel";
	struct tpcg_channel *chan_p = (struct tpcg_channel *)data;
	struct tpc_interface *tpc_p = (struct tpc_interface *)chan_p->c_data;
	struct tpcg_private *priv_p = (struct tpcg_private *)tpcg_p->tcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_tpc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	tpc_p->t_op->t_do_channel(tpc_p, scg_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	tpc_p->t_op->t_cleanup(tpc_p);
}

static struct tpg_interface *
tpcg_get_tpg(struct tpcg_interface *tpcg_p)
{
	struct tpcg_private *priv_p = (struct tpcg_private *)tpcg_p->tcg_private;
	return priv_p->p_tpg;
}

struct tpcg_operations tpcg_op = {
	.tcg_accept_new_channel = tpcg_accept_new_channel,
	.tcg_do_channel = tpcg_do_channel,
	.tcg_get_tpg = tpcg_get_tpg,
};

int
tpcg_initialization(struct tpcg_interface *tpcg_p, struct tpcg_setup *setup)
{
	char *log_header = "tpcg_initialization";
	struct tpcg_private *priv_p;

	nid_log_info("%s: start ...", log_header);

	tpcg_p->tcg_op = &tpcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	tpcg_p->tcg_private = priv_p;

	priv_p->p_tpg = setup->tpg;
	priv_p->p_umpk = setup->umpk;

	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_tpc_head);
	return 0;
}
