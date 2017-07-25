/*
 * cxpcg.c
 * 	Implementation of Control Exchange Passive Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "lstn_if.h"
#include "cxpc_if.h"
#include "cxpcg_if.h"

struct cxpcg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct cxpg_interface;
struct cxpcg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_cxpc_head;
	struct cxpg_interface	*p_cxpg;
	struct umpk_interface	*p_umpk;
};

static void *
cxpcg_accept_new_channel(struct cxpcg_interface *cxpcg_p, int sfd)
{
	char *log_header = "cxpcg_accept_new_channel";
	struct cxpcg_private *priv_p = (struct cxpcg_private *)cxpcg_p->cxcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct cxpc_interface *cxpc_p;
	struct cxpc_setup cxpc_setup;
	struct cxpcg_channel *chan_p;

	nid_log_debug("%s: start ...", log_header);
	cxpc_p = calloc(1, sizeof(*cxpc_p));
	cxpc_setup.cxpg = priv_p->p_cxpg;
	cxpc_setup.umpk = umpk_p;
	cxpc_initialization(cxpc_p, &cxpc_setup);

	chan_p = calloc(1, sizeof(*chan_p));
	chan_p->c_data = cxpc_p;
	cxpc_p->cxc_op->cxc_accept_new_channel(cxpc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
cxpcg_do_channel(struct cxpcg_interface *cxpcg_p, void *data)
{
	char *log_header = "cxpcg_do_channel";
	struct cxpcg_channel *chan_p = (struct cxpcg_channel *)data;
	struct cxpc_interface *cxpc_p = (struct cxpc_interface *)chan_p->c_data;
	struct cxpcg_private *priv_p = (struct cxpcg_private *)cxpcg_p->cxcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_cxpc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	cxpc_p->cxc_op->cxc_do_channel(cxpc_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	cxpc_p->cxc_op->cxc_cleanup(cxpc_p);
}


struct cxpcg_operations cxpcg_op = {
	.cxcg_accept_new_channel = cxpcg_accept_new_channel,
	.cxcg_do_channel = cxpcg_do_channel,
};

int
cxpcg_initialization(struct cxpcg_interface *cxpcg_p, struct cxpcg_setup *setup)
{
	char *log_header = "cxpcg_initialization";
	struct cxpcg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	cxpcg_p->cxcg_op = &cxpcg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	cxpcg_p->cxcg_private = priv_p;

	priv_p->p_cxpg = setup->cxpg;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_cxpc_head);
	return 0;
}
