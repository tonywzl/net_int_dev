/*
 * dxpcg.c
 * 	Implementation of Data Exchange Passive Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "lstn_if.h"
#include "dxpc_if.h"
#include "dxpcg_if.h"

struct dxpcg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct dxpg_interface;
struct dxpcg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_dxpc_head;
	struct dxpg_interface	*p_dxpg;
	struct umpk_interface	*p_umpk;
};

static void *
dxpcg_accept_new_channel(struct dxpcg_interface *dxpcg_p, int sfd)
{
	char *log_header = "dxpcg_accept_new_channel";
	struct dxpcg_private *priv_p = (struct dxpcg_private *)dxpcg_p->dxcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct dxpc_interface *dxpc_p;
	struct dxpc_setup dxpc_setup;
	struct dxpcg_channel *chan_p;

	nid_log_debug("%s: start ...", log_header);
	dxpc_p = calloc(1, sizeof(*dxpc_p));
	dxpc_setup.dxpg = priv_p->p_dxpg;
	dxpc_setup.umpk = umpk_p;
	dxpc_initialization(dxpc_p, &dxpc_setup);

	chan_p = calloc(1, sizeof(*chan_p));
	chan_p->c_data = dxpc_p;
	dxpc_p->dxc_op->dxc_accept_new_channel(dxpc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
dxpcg_do_channel(struct dxpcg_interface *dxpcg_p, void *data)
{
	char *log_header = "dxpcg_do_channel";
	struct dxpcg_channel *chan_p = (struct dxpcg_channel *)data;
	struct dxpc_interface *dxpc_p = (struct dxpc_interface *)chan_p->c_data;
	struct dxpcg_private *priv_p = (struct dxpcg_private *)dxpcg_p->dxcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_dxpc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	dxpc_p->dxc_op->dxc_do_channel(dxpc_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	dxpc_p->dxc_op->dxc_cleanup(dxpc_p);
}


struct dxpcg_operations dxpcg_op = {
	.dxcg_accept_new_channel = dxpcg_accept_new_channel,
	.dxcg_do_channel = dxpcg_do_channel,
};

int
dxpcg_initialization(struct dxpcg_interface *dxpcg_p, struct dxpcg_setup *setup)
{
	char *log_header = "dxpcg_initialization";
	struct dxpcg_private *priv_p;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);

	dxpcg_p->dxcg_op = &dxpcg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dxpcg_p->dxcg_private = priv_p;

	priv_p->p_dxpg = setup->dxpg;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_dxpc_head);
	return 0;
}
