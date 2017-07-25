/*
 * dxacg.c
 * 	Implementation of Data Exchange Active Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "lstn_if.h"
#include "dxa_if.h"
#include "dxac_if.h"
#include "dxacg_if.h"

struct dxacg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct dxacg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_dxac_head;
	struct umpk_interface	*p_umpk;
	struct dxag_interface	*p_dxag;
};

static void *
dxacg_accept_new_channel(struct dxacg_interface *dxacg_p, int sfd)
{
	char *log_header = "dxacg_accept_new_channel";
	struct dxacg_private *priv_p = (struct dxacg_private *)dxacg_p->dxcg_private;
	struct dxac_interface *dxac_p;
	struct dxac_setup dxac_setup;
	struct dxacg_channel *chan_p;

	nid_log_debug("%s: start ...", log_header);
	assert(priv_p);
	dxac_p = calloc(1, sizeof(*dxac_p));
	dxac_setup.dxacg = dxacg_p;
	dxac_setup.umpk = priv_p->p_umpk;
	dxac_setup.dxag = priv_p->p_dxag;
	dxac_initialization(dxac_p, &dxac_setup);

	chan_p = calloc(1, sizeof(*chan_p));
	chan_p->c_data = dxac_p;
	dxac_p->dxc_op->dxc_accept_new_channel(dxac_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
dxacg_do_channel(struct dxacg_interface *dxacg_p, void *data)
{
	char *log_header = "dxacg_do_channel";
	struct dxacg_channel *chan_p = (struct dxacg_channel *)data;
	struct dxac_interface *dxac_p = (struct dxac_interface *)chan_p->c_data;
	struct dxacg_private *priv_p = (struct dxacg_private *)dxacg_p->dxcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_dxac_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	dxac_p->dxc_op->dxc_do_channel(dxac_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);

	dxac_p->dxc_op->dxc_cleanup(dxac_p);
}


struct dxacg_operations dxacg_op = {
	.dxcg_accept_new_channel = dxacg_accept_new_channel,
	.dxcg_do_channel = dxacg_do_channel,
};

int
dxacg_initialization(struct dxacg_interface *dxacg_p, struct dxacg_setup *setup)
{
	char *log_header = "dxacg_initialization";
	struct dxacg_private *priv_p;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);

	dxacg_p->dxcg_op = &dxacg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dxacg_p->dxcg_private = priv_p;

	priv_p->p_umpk = setup->umpk;
	priv_p->p_dxag = setup->dxag;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_dxac_head);
	return 0;
}
