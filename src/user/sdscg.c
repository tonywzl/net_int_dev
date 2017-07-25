/*
 * sdscg.c
 * 	Implementation of Split Data Stream Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "sdsc_if.h"
#include "sdscg_if.h"

struct sdscg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct sdscg_private {
	pthread_mutex_t		p_mlck;			// protects p_sdsc_head
	struct list_head	p_sdsc_head;
	struct sdsg_interface	*p_sdsg;
	struct umpk_interface	*p_umpk;
};

static void *
sdscg_accept_new_channel(struct sdscg_interface *sdscg_p, int sfd, char *cip)
{
	char *log_header = "sdscg_accept_new_channel";
	struct sdscg_private *priv_p = (struct sdscg_private *)sdscg_p->sdscg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sdsc_interface *sdsc_p;
	struct sdsc_setup sdsc_setup;
	struct sdscg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	sdsc_p = x_calloc(1, sizeof(*sdsc_p));
	sdsc_setup.sdsg = priv_p->p_sdsg;
	sdsc_setup.umpk = umpk_p;
	sdsc_initialization(sdsc_p, &sdsc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = sdsc_p;
	sdsc_p->sdsc_op->sdsc_accept_new_channel(sdsc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
sdscg_do_channel(struct sdscg_interface *sdscg_p, void *data, struct scg_interface *scg_p)
{
	char *log_header = "sdscg_do_channel";
	struct sdscg_channel *chan_p = (struct sdscg_channel *)data;
	struct sdsc_interface *sdsc_p = (struct sdsc_interface *)chan_p->c_data;
	struct sdscg_private *priv_p = (struct sdscg_private *)sdscg_p->sdscg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_sdsc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	sdsc_p->sdsc_op->sdsc_do_channel(sdsc_p, scg_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	sdsc_p->sdsc_op->sdsc_cleanup(sdsc_p);
}

static struct sdsg_interface *
sdscg_get_sdsg(struct sdscg_interface *sdscg_p)
{
	struct sdscg_private *priv_p = (struct sdscg_private *)sdscg_p->sdscg_private;
	return priv_p->p_sdsg;
}

struct sdscg_operations sdscg_op = {
	.sdscg_accept_new_channel = sdscg_accept_new_channel,
	.sdscg_do_channel = sdscg_do_channel,
	.sdscg_get_sdsg = sdscg_get_sdsg,
};

int
sdscg_initialization(struct sdscg_interface *sdscg_p, struct sdscg_setup *setup)
{
	char *log_header = "sdscg_initialization";
	struct sdscg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	sdscg_p->sdscg_op = &sdscg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	sdscg_p->sdscg_private = (void *)priv_p;
	priv_p->p_sdsg = setup->sdsg;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_sdsc_head);
	return 0;
}
