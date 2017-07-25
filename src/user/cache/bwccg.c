/*
 * bwccg.c
 * 	Implementation of None-Memory Write Cache Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "bwcc_if.h"
#include "bwccg_if.h"
#include "wc_if.h"

struct bwccg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct bwccg_private {
	pthread_mutex_t		p_mlck;			// protects p_bwcc_head
	struct list_head	p_bwcc_head;
	struct bwcg_interface	*p_bwcg;
	struct umpk_interface	*p_umpk;
};

static void *
bwccg_accept_new_channel(struct bwccg_interface *bwccg_p, int sfd, char *cip)
{
	char *log_header = "bwccg_accept_new_channel";
	struct bwccg_private *priv_p = (struct bwccg_private *)bwccg_p->wcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct bwcc_interface *bwcc_p;
	struct bwcc_setup bwcc_setup;
	struct bwccg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	bwcc_p = x_calloc(1, sizeof(*bwcc_p));
	bwcc_setup.bwcg = priv_p->p_bwcg;
	bwcc_setup.umpk = umpk_p;
	bwcc_initialization(bwcc_p, &bwcc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = bwcc_p;
	bwcc_p->w_op->w_accept_new_channel(bwcc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
bwccg_do_channel(struct bwccg_interface *bwccg_p, void *data, struct scg_interface *scg_p)
{
	char *log_header = "bwccg_do_channel";
	struct bwccg_channel *chan_p = (struct bwccg_channel *)data;
	struct bwcc_interface *bwcc_p = (struct bwcc_interface *)chan_p->c_data;
	struct bwccg_private *priv_p = (struct bwccg_private *)bwccg_p->wcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_bwcc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	bwcc_p->w_op->w_do_channel(bwcc_p, scg_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	bwcc_p->w_op->w_cleanup(bwcc_p);
}

static struct bwcg_interface *
bwccg_get_bwcg(struct bwccg_interface *bwccg_p)
{
	struct bwccg_private *priv_p = (struct bwccg_private *)bwccg_p->wcg_private;
	return priv_p->p_bwcg;
}

struct bwccg_operations bwccg_op = {
	.wcg_accept_new_channel = bwccg_accept_new_channel,
	.wcg_do_channel = bwccg_do_channel,
	.wcg_get_bwcg = bwccg_get_bwcg,
};

int
bwccg_initialization(struct bwccg_interface *bwccg_p, struct bwccg_setup *setup)
{
	char *log_header = "bwccg_initialization";
	struct bwccg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	bwccg_p->wcg_op = &bwccg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bwccg_p->wcg_private = priv_p;

	priv_p->p_bwcg = setup->bwcg;
	priv_p->p_umpk = setup->umpk;

	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_bwcc_head);
	return 0;
}
