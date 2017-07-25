/*
 * saccg.c
 * 	Implementation of Server Agent Channel (sac) Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "sacc_if.h"
#include "saccg_if.h"

struct saccg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct saccg_private {
	pthread_mutex_t		p_mlck;			// protects p_sacc_head
	struct list_head	p_sacc_head;
	struct sacg_interface	*p_sacg;
	struct umpk_interface	*p_umpk;
};

static void *
saccg_accept_new_channel(struct saccg_interface *saccg_p, int sfd, char *cip)
{
	char *log_header = "saccg_accept_new_channel";
	struct saccg_private *priv_p = (struct saccg_private *)saccg_p->saccg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct sacc_interface *sacc_p;
	struct sacc_setup sacc_setup;
	struct saccg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	sacc_p = x_calloc(1, sizeof(*sacc_p));
	sacc_setup.sacg = priv_p->p_sacg;
	sacc_setup.umpk = umpk_p;
	sacc_initialization(sacc_p, &sacc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = sacc_p;
	sacc_p->sacc_op->sacc_accept_new_channel(sacc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
saccg_do_channel(struct saccg_interface *saccg_p, void *data, struct scg_interface *scg_p)
{
	char *log_header = "saccg_do_channel";
	struct saccg_channel *chan_p = (struct saccg_channel *)data;
	struct sacc_interface *sacc_p = (struct sacc_interface *)chan_p->c_data;
	struct saccg_private *priv_p = (struct saccg_private *)saccg_p->saccg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_sacc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	sacc_p->sacc_op->sacc_do_channel(sacc_p, scg_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	sacc_p->sacc_op->sacc_cleanup(sacc_p);
}

static struct sacg_interface *
saccg_get_sacg(struct saccg_interface *saccg_p)
{
	struct saccg_private *priv_p = (struct saccg_private *)saccg_p->saccg_private;
	return priv_p->p_sacg;
}

struct saccg_operations saccg_op = {
	.saccg_accept_new_channel = saccg_accept_new_channel,
	.saccg_do_channel = saccg_do_channel,
	.saccg_get_sacg = saccg_get_sacg,
};

int
saccg_initialization(struct saccg_interface *saccg_p, struct saccg_setup *setup)
{
	char *log_header = "saccg_initialization";
	struct saccg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	saccg_p->saccg_op = &saccg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	saccg_p->saccg_private = (void *)priv_p;
	priv_p->p_sacg = setup->sacg;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_sacc_head);
	return 0;
}
