/*
 * ppcg.c
 * 	Implementation of Page Pool Channel Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "ppc_if.h"
#include "ppcg_if.h"

struct ppcg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct ppcg_private {
	pthread_mutex_t		p_mlck;			// protects p_ppc_head
	struct list_head	p_ppc_head;
	struct ppg_interface	*p_ppg;
	struct umpk_interface	*p_umpk;
};

static void *
ppcg_accept_new_channel(struct ppcg_interface *ppcg_p, int sfd, char *cip)
{
	char *log_header = "ppcg_accept_new_channel";
	struct ppcg_private *priv_p = (struct ppcg_private *)ppcg_p->ppcg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct ppc_interface *ppc_p;
	struct ppc_setup ppc_setup;
	struct ppcg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	ppc_p = x_calloc(1, sizeof(*ppc_p));
	ppc_setup.ppg = priv_p->p_ppg;
	ppc_setup.umpk = umpk_p;
	ppc_initialization(ppc_p, &ppc_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = ppc_p;
	ppc_p->ppc_op->ppc_accept_new_channel(ppc_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
ppcg_do_channel(struct ppcg_interface *ppcg_p, void *data, struct scg_interface *scg_p)
{
	char *log_header = "ppcg_do_channel";
	struct ppcg_channel *chan_p = (struct ppcg_channel *)data;
	struct ppc_interface *ppc_p = (struct ppc_interface *)chan_p->c_data;
	struct ppcg_private *priv_p = (struct ppcg_private *)ppcg_p->ppcg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_ppc_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	ppc_p->ppc_op->ppc_do_channel(ppc_p, scg_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	ppc_p->ppc_op->ppc_cleanup(ppc_p);
}

static struct ppg_interface *
ppcg_get_ppg(struct ppcg_interface *ppcg_p)
{
	struct ppcg_private *priv_p = (struct ppcg_private *)ppcg_p->ppcg_private;
	return priv_p->p_ppg;
}

struct ppcg_operations ppcg_op = {
	.ppcg_accept_new_channel = ppcg_accept_new_channel,
	.ppcg_do_channel = ppcg_do_channel,
	.ppcg_get_ppg = ppcg_get_ppg,
};

int
ppcg_initialization(struct ppcg_interface *ppcg_p, struct ppcg_setup *setup)
{
	char *log_header = "ppcg_initialization";
	struct ppcg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	ppcg_p->ppcg_op = &ppcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	ppcg_p->ppcg_private = (void *)priv_p;
	priv_p->p_ppg = setup->ppg;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_ppc_head);
	return 0;
}
