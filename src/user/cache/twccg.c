/*
 * twccg.c
 * 	Implementation of Write Through Cache (twc) Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "twcc_if.h"
#include "twccg_if.h"

struct twccg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct twccg_private {
	pthread_mutex_t		p_mlck;		// protects p_twcc_head
	struct list_head	p_twcc_head;
	struct twcg_interface	*p_twcg;
	struct umpk_interface	*p_umpk;
};

static void *
twccg_accept_new_channel(struct twccg_interface *twccg_p, int sfd, char *cip)
{
	char *log_header = "twccg_accept_new_channel";
	struct twccg_private *priv_p = (struct twccg_private *)twccg_p->wcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct twcc_interface *twcc_p;
	struct twcc_setup twcc_setup;
	struct twccg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	twcc_p = x_calloc(1, sizeof(*twcc_p));
	twcc_setup.twcg = priv_p->p_twcg;
	twcc_setup.umpk = umpk_p;
	twcc_initialization(twcc_p, &twcc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = twcc_p;
	twcc_p->w_op->w_accept_new_channel(twcc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
twccg_do_channel(struct twccg_interface *twccg_p, void *data)
{
	char *log_header = "twccg_do_channel";
	struct twccg_channel *chan_p = (struct twccg_channel *)data;
	struct twcc_interface *twcc_p = (struct twcc_interface *)chan_p->c_data;
	struct twccg_private *priv_p = (struct twccg_private *)twccg_p->wcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_twcc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	twcc_p->w_op->w_do_channel(twcc_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	twcc_p->w_op->w_cleanup(twcc_p);
}

static struct twcg_interface *
twccg_get_twcg(struct twccg_interface *twccg_p)
{
	struct twccg_private *priv_p = (struct twccg_private *)twccg_p->wcg_private;
	return priv_p->p_twcg;
}

struct twccg_operations twccg_op = {
	.wcg_accept_new_channel = twccg_accept_new_channel,
	.wcg_do_channel = twccg_do_channel,
	.wcg_get_twcg = twccg_get_twcg,
};

int
twccg_initialization(struct twccg_interface *twccg_p, struct twccg_setup *setup)
{
	char *log_header = "twccg_initialization";
	struct twccg_private *priv_p;
	struct list_head *all_twc_heads;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	twccg_p->wcg_op = &twccg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	twccg_p->wcg_private = priv_p;

	all_twc_heads = x_calloc(1, sizeof(*all_twc_heads));
	INIT_LIST_HEAD(all_twc_heads);

	priv_p->p_twcg = setup->twcg;
	priv_p->p_umpk = setup->umpk;

	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_twcc_head);
	return 0;
}
