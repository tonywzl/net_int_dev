/*
 * doag.c
 * 	Implementation of Delegation of Authority Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "doa_if.h"
#include "doag_if.h"

struct doag_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct doag_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_doa_head;
	struct mpk_interface	*p_mpk;
};

static void *
doag_accept_new_channel(struct doag_interface *doag_p, int sfd)
{
	char *log_header = "doag_accept_new_channel";
	struct doag_private *priv_p = (struct doag_private *)doag_p->dg_private;
	struct doa_interface *doa_p;
	struct doa_setup doa_setup;
	struct doag_channel *chan_p;

	assert(priv_p);
	doa_p = x_calloc(1, sizeof(*doa_p));
	doa_setup.doag = doag_p;
	doa_setup.umpk = priv_p->p_mpk;
	doa_initialization(doa_p, &doa_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = doa_p;
	doa_p->d_op->d_accept_new_channel(doa_p, sfd);

	nid_log_debug("%s: end (chan_p:%p)", log_header, chan_p);
	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
doag_do_channel(struct doag_interface *doag_p, void *data)
{
	char *log_header = "doag_do_channel";
	struct doag_channel *chan_p = (struct doag_channel *)data;
	struct doa_interface *doa_p = (struct doa_interface *)chan_p->c_data;
	struct doag_private *priv_p = (struct doag_private *)doag_p->dg_private;
	struct doag_channel *old_chan = NULL;
	struct doa_interface *old_doa = NULL, *ret_doa;
	char *the_id, *my_id;
	int got_it = 0;

	nid_log_debug("%s: start (chan_p:%p) ...", log_header, chan_p);
	my_id = doa_p->d_op->d_get_id(doa_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_for_each_entry(old_chan, struct doag_channel, &priv_p->p_doa_head, c_list) {
		old_doa = (struct doa_interface *)old_chan->c_data;
		the_id = old_doa->d_op->d_get_id(old_doa);
		nid_log_debug(" doa list entry : the id = %s",the_id);
		if (!strcmp(my_id, the_id)) {
			got_it =1;
			break;
		}
	}
	if (!got_it)
		old_doa = NULL;

	ret_doa = doa_p->d_op->d_do_channel(doa_p, old_doa);

	if (ret_doa != old_doa) {
		if (!ret_doa) {
			/*
			 * keep neither doa_p, nor old_doa
			 */
			nid_log_debug("%s: remove the doa", log_header);

			old_doa->d_op->d_free(old_doa);
			free(old_doa);
			list_del(&old_chan->c_list);
			free(old_chan);
			old_chan = NULL;

			doa_p->d_op->d_free(doa_p);
			free(doa_p);
			free(chan_p);
		} else {
			/*
			 * drop old_doa, keep doa_p
			 */
			assert(ret_doa == doa_p);
			nid_log_debug("%s: replace the old_doa (%p) with doa (chan_p:%p, data:%p)",
				log_header, old_doa, chan_p, chan_p->c_data);
			if (old_doa) {
				old_doa->d_op->d_free(old_doa);
				free(old_doa);
				list_del(&old_chan->c_list);
				free(old_chan);
				old_chan = NULL;
			}
			list_add_tail(&chan_p->c_list, &priv_p->p_doa_head);
			assert(chan_p->c_data == doa_p);
		}

	} else {
		doa_p->d_op->d_free(doa_p);
		free(doa_p);
		free(chan_p);
		chan_p = NULL;
		nid_log_debug("%s: keep the old_doa",log_header);
	}
	pthread_mutex_unlock(&priv_p->p_mlck);
}

struct doag_operations doag_op = {
	.dg_accept_new_channel = doag_accept_new_channel,
	.dg_do_channel = doag_do_channel,
};

int
doag_initialization(struct doag_interface *doag_p, struct doag_setup *setup)
{
	char *log_header = "doag_initialization";
	struct doag_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	doag_p->dg_op = &doag_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	doag_p->dg_private = priv_p;
	priv_p->p_mpk = setup->p_mpk;
	INIT_LIST_HEAD(&priv_p->p_doa_head);
	return 0;
}
