/*
 * drwcg.c
 * 	Implementation of Device Read Write Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "drwc_if.h"
#include "drwcg_if.h"

struct drwcg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct drwcg_private {
	pthread_mutex_t		p_mlck;			// protects p_drwc_head
	struct list_head	p_drwc_head;
	struct drwg_interface	*p_drwg;
	struct umpk_interface	*p_umpk;
};

static void *
drwcg_accept_new_channel(struct drwcg_interface *drwcg_p, int sfd, char *cip)
{
	char *log_header = "drwcg_accept_new_channel";
	struct drwcg_private *priv_p = (struct drwcg_private *)drwcg_p->drwcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct drwc_interface *drwc_p;
	struct drwc_setup drwc_setup;
	struct drwcg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	drwc_p = x_calloc(1, sizeof(*drwc_p));
	drwc_setup.drwcg = drwcg_p;
	drwc_setup.umpk = umpk_p;
	drwc_initialization(drwc_p, &drwc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = drwc_p;
	drwc_p->drwc_op->drwc_accept_new_channel(drwc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
drwcg_do_channel(struct drwcg_interface *drwcg_p, void *data, struct scg_interface *scg_p)
{
	char *log_header = "drwcg_do_channel";
	struct drwcg_channel *chan_p = (struct drwcg_channel *)data;
	struct drwc_interface *drwc_p = (struct drwc_interface *)chan_p->c_data;
	struct drwcg_private *priv_p = (struct drwcg_private *)drwcg_p->drwcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_drwc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	drwc_p->drwc_op->drwc_do_channel(drwc_p, scg_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	drwc_p->drwc_op->drwc_cleanup(drwc_p);
}

static struct drwg_interface *
drwcg_get_drwg(struct drwcg_interface *drwcg_p)
{
	struct drwcg_private *priv_p = (struct drwcg_private *)drwcg_p->drwcg_private;
	return priv_p->p_drwg;
}

struct drwcg_operations drwcg_op = {
	.drwcg_accept_new_channel = drwcg_accept_new_channel,
	.drwcg_do_channel = drwcg_do_channel,
	.drwcg_get_drwg = drwcg_get_drwg,
};

int
drwcg_initialization(struct drwcg_interface *drwcg_p, struct drwcg_setup *setup)
{
	char *log_header = "drwcg_initialization";
	struct drwcg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	drwcg_p->drwcg_op = &drwcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	drwcg_p->drwcg_private = (void *)priv_p;
	priv_p->p_drwg = setup->drwg;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_drwc_head);
	return 0;
}
