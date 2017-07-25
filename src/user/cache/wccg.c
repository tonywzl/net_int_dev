/*
 * wccg.c
 * 	Implementation of Read Cache Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "lstn_if.h"
#include "wc_if.h"
#include "wcc_if.h"
#include "scg_if.h"
#include "wccg_if.h"
#include "bwcg_if.h"
#include "twcg_if.h"

struct wccg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct wccg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_wcc_head;
	struct bwcg_interface	*p_bwcg;
	struct twcg_interface	*p_twcg;
	struct umpk_interface	*p_umpk;
};

static void *
wccg_accept_new_channel(struct wccg_interface *wccg_p, int sfd, char *cip)
{
	char *log_header = "wccg_accept_new_channel";
	struct wccg_private *priv_p = (struct wccg_private *)wccg_p->wcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct wcc_interface *wcc_p;
	struct wcc_setup wcc_setup;
	struct wccg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	wcc_p = x_calloc(1, sizeof(*wcc_p));
	wcc_setup.wccg = wccg_p;
	wcc_setup.umpk = umpk_p;
	wcc_initialization(wcc_p, &wcc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = wcc_p;
	wcc_p->w_op->w_accept_new_channel(wcc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
wccg_do_channel(struct wccg_interface *wccg_p, void *data)
{
	char *log_header = "wccg_do_channel";
	struct wccg_channel *chan_p = (struct wccg_channel *)data;
	struct wcc_interface *wcc_p = (struct wcc_interface *)chan_p->c_data;
	struct wccg_private *priv_p = (struct wccg_private *)wccg_p->wcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_wcc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	wcc_p->w_op->w_do_channel(wcc_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	wcc_p->w_op->w_cleanup(wcc_p);
}

static struct wc_interface *
wccg_get_wc(struct wccg_interface *wccg_p, char *wc_uuid)
{
	struct wccg_private *priv_p = (struct wccg_private *)wccg_p->wcg_private;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	struct twcg_interface *twcg_p = priv_p->p_twcg;
	struct wc_interface *wc_p = NULL;


	if (bwcg_p)
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, wc_uuid);
	if (!wc_p && twcg_p) {
		wc_p = twcg_p->wg_op->wg_search_twc(twcg_p, wc_uuid);
	}
	return wc_p;
}

struct wccg_operations wccg_op = {
	.wcg_accept_new_channel = wccg_accept_new_channel,
	.wcg_do_channel = wccg_do_channel,
	.wcg_get_wc = wccg_get_wc,
};

int
wccg_initialization(struct wccg_interface *wccg_p, struct wccg_setup *setup)
{
	char *log_header = "wccg_initialization";
	struct wccg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	wccg_p->wcg_op = &wccg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	wccg_p->wcg_private = priv_p;

	priv_p->p_bwcg = setup->bwcg;
	priv_p->p_twcg = setup->twcg;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_wcc_head);
	return 0;
}
