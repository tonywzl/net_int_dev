/*
 * inicg.c
 *	Implementation of ini Channel Guaridan Module
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "inic_if.h"
#include "inicg_if.h"

struct inicg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct umpk_interface;
struct inicg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_inic_head;
	struct ini_interface	*p_ini;
	struct umpk_interface	*p_umpk;
};

static void *
inicg_accept_new_channel(struct inicg_interface *inicg_p, int sfd, char *cip)
{
	char *log_header = "inicg_accept_new_channel";
	struct inicg_private *priv_p = (struct inicg_private *)inicg_p->icg_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct inic_interface *inic_p;
	struct inic_setup inic_setup;
	struct inicg_channel *chan_p;

	nid_log_debug("%s: start (cip:%s)...", log_header, cip);
	assert(priv_p);
	inic_p = x_calloc(1, sizeof(*inic_p));
	inic_setup.ini = priv_p->p_ini;
	inic_setup.umpk = umpk_p;
	inic_initialization(inic_p, &inic_setup);
	
	chan_p = x_calloc(1, sizeof(chan_p));
	chan_p->c_data = inic_p;
	inic_p->i_op->i_accept_new_channel(inic_p, sfd);

	return chan_p;
}

static void
inicg_do_channel(struct inicg_interface *inicg_p, void *data)
{
	char *log_header = "inicg_do_channel";
	struct inicg_channel *chan_p = (struct inicg_channel *)data;
	struct inic_interface *inic_p = (struct inic_interface *)chan_p->c_data;
	struct inicg_private *priv_p = (struct inicg_private *)inicg_p->icg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_inic_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	inic_p->i_op->i_do_channel(inic_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
	inic_p->i_op->i_cleanup(inic_p);
}

struct inicg_operations inicg_op = {
	.icg_accept_new_channel = inicg_accept_new_channel,
	.icg_do_channel = inicg_do_channel,
};

int
inicg_initialization(struct inicg_interface *inicg_p, struct inicg_setup *setup)
{
	char *log_header = "inicg_initialization";
	struct inicg_private *priv_p;

	nid_log_info("%s: start ...", log_header);

	inicg_p->icg_op = &inicg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	inicg_p->icg_private = priv_p;

	priv_p->p_ini = setup->ini;
	priv_p->p_umpk = setup->umpk;

	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_inic_head);
	return 0;
}
