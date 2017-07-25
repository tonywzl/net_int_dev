/*
 * mrwcg.c
 * 	Implementation of Meta Server Read Write  Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "mrwc_if.h"
#include "mrwcg_if.h"

struct mrwcg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct mrwcg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_mrwc_head;
	struct mrwg_interface	*p_mrwg;
	struct umpk_interface	*p_umpk;

};

static void *
mrmrwcg_accept_new_channel(struct mrwcg_interface *mrwcg_p, int sfd, char *cip)
{
	char *log_header = "mrmrwcg_accept_new_channel";
	struct mrwcg_private *priv_p = (struct mrwcg_private *)mrwcg_p->mrwcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct mrwc_interface *mrwc_p;
	struct mrwc_setup mrwc_setup;
	struct mrwcg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	mrwc_p = x_calloc(1, sizeof(*mrwc_p));
	mrwc_setup.mrwcg = mrwcg_p;
	mrwc_setup.umpk = umpk_p;
	mrwc_initialization(mrwc_p, &mrwc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = mrwc_p;
	mrwc_p->w_op->w_accept_new_channel(mrwc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
mrwcg_do_channel(struct mrwcg_interface *mrwcg_p, void *data, struct scg_interface *scg_p)
{
	char *log_header = "mrwcg_do_channel";
	struct mrwcg_channel *chan_p = (struct mrwcg_channel *)data;
	struct mrwc_interface *mrwc_p = (struct mrwc_interface *)chan_p->c_data;
	struct mrwcg_private *priv_p = (struct mrwcg_private *)mrwcg_p->mrwcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_mrwc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	mrwc_p->w_op->w_do_channel(mrwc_p, scg_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	mrwc_p->w_op->w_cleanup(mrwc_p);
}

static struct mrwg_interface *
mrwcg_get_mrwg (struct mrwcg_interface *mrwcg_p)
{
	struct mrwcg_private *priv_p = (struct mrwcg_private *)mrwcg_p->mrwcg_private;
	return priv_p->p_mrwg;
}


struct mrwcg_operations mrwcg_op = {
	.mrwcg_accept_new_channel = mrmrwcg_accept_new_channel,
	.mrwcg_do_channel = mrwcg_do_channel,
	.mrwcg_get_mrwg = mrwcg_get_mrwg,

};

int
mrwcg_initialization(struct mrwcg_interface *mrwcg_p, struct mrwcg_setup *setup)
{
	char *log_header = "mrwcg_initialization";
	struct mrwcg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	mrwcg_p->mrwcg_op = &mrwcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	mrwcg_p->mrwcg_private = priv_p;

	priv_p->p_mrwg = setup->mrwg;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_mrwc_head);
	return 0;
}
