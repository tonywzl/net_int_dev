/*
 * snapg.c
 * 	Implementation of Snapshot Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "snap_if.h"
#include "snapg_if.h"

struct snapg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct snapg_private {
	pthread_mutex_t		p_mlck;
	struct list_head	p_snap_head;
};

static void *
snapg_accept_new_channel(struct snapg_interface *snapg_p, int sfd)
{
	struct snapg_private *priv_p = (struct snapg_private *)snapg_p->ssg_private;
	struct snap_interface *snap_p;
	struct snap_setup snap_setup;
	struct snapg_channel *chan_p;

	assert(priv_p);
	snap_p = x_calloc(1, sizeof(*snap_p));
	snap_setup.snapg = snapg_p;
	snap_initialization(snap_p, &snap_setup);

	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = snap_p;
	snap_p->ss_op->ss_accept_new_channel(snap_p, sfd);

	return chan_p;
}

/*
 * Algorithm:
 *
 */
static void
snapg_do_channel(struct snapg_interface *snapg_p, void *data)
{
	char *log_header = "snapg_do_channel";
	struct snapg_channel *chan_p = (struct snapg_channel *)data;
	struct snap_interface *snap_p = (struct snap_interface *)chan_p->c_data;
	struct snapg_private *priv_p = (struct snapg_private *)snapg_p->ssg_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_snap_head);
	pthread_mutex_unlock(&priv_p->p_mlck);

	snap_p->ss_op->ss_do_channel(snap_p);

	pthread_mutex_lock(&priv_p->p_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_mlck);
}

struct snapg_operations snapg_op = {
	.ssg_accept_new_channel = snapg_accept_new_channel,
	.ssg_do_channel = snapg_do_channel,
};

int
snapg_initialization(struct snapg_interface *snapg_p, struct snapg_setup *setup)
{
	char *log_header = "snapg_initialization";
	struct snapg_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);

	snapg_p->ssg_op = &snapg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	snapg_p->ssg_private = priv_p;

	pthread_mutex_init(&priv_p->p_mlck, NULL);
	INIT_LIST_HEAD(&priv_p->p_snap_head);
	return 0;
}
