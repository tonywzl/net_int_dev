/*
 * rccg.c
 * 	Implementation of Read Cache Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "crcc_if.h"
#include "crccg_if.h"

struct crccg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct crccg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_crcc_head;
	struct crcg_interface	*p_crcg;
	struct umpk_interface	*p_umpk;
};

static void *
crccg_accept_new_channel(struct crccg_interface *crccg_p, int sfd, char *cip)
{
	char *log_header = "crccg_accept_new_channel";
	struct crccg_private *priv_p = (struct crccg_private *)crccg_p->crg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct crcc_interface *crcc_p;
	struct crcc_setup crcc_setup;
	struct crccg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	crcc_p = x_calloc(1, sizeof(*crcc_p));
	crcc_setup.crcg = priv_p->p_crcg;
	crcc_setup.umpk = umpk_p;
	crcc_initialization(crcc_p, &crcc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = crcc_p;
	crcc_p->r_op->crcc_accept_new_channel(crcc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
crccg_do_channel(struct crccg_interface *crccg_p, void *data, struct scg_interface *scg_p)
{
	char *log_header = "crccg_do_channel";
	struct crccg_channel *chan_p = (struct crccg_channel *)data;
	struct crcc_interface *crcc_p = (struct crcc_interface *)chan_p->c_data;
	struct crccg_private *priv_p = (struct crccg_private *)crccg_p->crg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_crcc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);
	nid_log_error("%s : first lock", log_header);
	crcc_p->r_op->crcc_do_channel(crcc_p, scg_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);

	pthread_mutex_unlock(&priv_p->p_mlck);
	nid_log_error("%s : second lock", log_header);
	crcc_p->r_op->crcc_cleanup(crcc_p);
}

struct crccg_operations crccg_op = {
	.crg_accept_new_channel = crccg_accept_new_channel,
	.crg_do_channel = crccg_do_channel,
};

int
crccg_initialization(struct crccg_interface *crccg_p, struct crccg_setup *setup)
{
	char *log_header = "crccg_initialization";
	struct crccg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	crccg_p->crg_op = &crccg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	crccg_p->crg_private = priv_p;

	priv_p->p_crcg = setup->crcg;
	priv_p->p_umpk = setup->umpk;

	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_crcc_head);
	return 0;
}
