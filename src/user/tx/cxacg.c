/*
 * cxacg.c
 * 	Implementation of Control Exchange Active Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "lstn_if.h"
#include "cxa_if.h"
#include "cxac_if.h"
#include "cxacg_if.h"
#include "cxag_if.h"

struct cxacg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct cxacg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_cxac_head;
	struct umpk_interface	*p_umpk;
	struct cxag_interface	*p_cxag;
};

static void *
cxacg_accept_new_channel(struct cxacg_interface *cxacg_p, int sfd)
{
	char *log_header = "cxacg_accept_new_channel";
	struct cxacg_private *priv_p = (struct cxacg_private *)cxacg_p->cxcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct cxac_interface *cxac_p;
	struct cxac_setup cxac_setup;
	struct cxacg_channel *chan_p;

	nid_log_debug("%s: start ...", log_header);
	assert(priv_p);
	cxac_p = calloc(1, sizeof(*cxac_p));
	cxac_setup.cxacg = cxacg_p;
	cxac_setup.umpk = umpk_p;
	cxac_setup.cxag = priv_p->p_cxag;
	cxac_initialization(cxac_p, &cxac_setup);

	chan_p = calloc(1, sizeof(*chan_p));
	chan_p->c_data = cxac_p;
	cxac_p->cxc_op->cxc_accept_new_channel(cxac_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
cxacg_do_channel(struct cxacg_interface *cxacg_p, void *data)
{
	char *log_header = "cxacg_do_channel";
	struct cxacg_channel *chan_p = (struct cxacg_channel *)data;
	struct cxac_interface *cxac_p = (struct cxac_interface *)chan_p->c_data;
	struct cxacg_private *priv_p = (struct cxacg_private *)cxacg_p->cxcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_cxac_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	cxac_p->cxc_op->cxc_do_channel(cxac_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	cxac_p->cxc_op->cxc_cleanup(cxac_p);
}


struct cxacg_operations cxacg_op = {
	.cxcg_accept_new_channel = cxacg_accept_new_channel,
	.cxcg_do_channel = cxacg_do_channel,
};

int
cxacg_initialization(struct cxacg_interface *cxacg_p, struct cxacg_setup *setup)
{
	char *log_header = "cxacg_initialization";
	struct cxacg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	cxacg_p->cxcg_op = &cxacg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	cxacg_p->cxcg_private = priv_p;

	priv_p->p_umpk = setup->umpk;
	priv_p->p_cxag = setup->cxag;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_cxac_head);
	return 0;
}
