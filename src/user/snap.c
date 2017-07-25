/*
 * snap.c
 * 	Implementation of  snap Module
 */

#include <unistd.h>
#include <stdlib.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "snap_if.h"

struct snap_private {
	int	p_sfd;
};

static int
snap_accept_new_channel(struct snap_interface *snap_p, int sfd)
{
	char *log_header = "snap_accept_new_channel";
	struct snap_private *priv_p = snap_p->ss_private;
	char cmd;
	int rc = 0;

	priv_p->p_sfd = sfd;
	read(sfd, &cmd, 1);
	if (cmd != NID_REQ_SNAP) {
		nid_log_error("%s: got wrong cmd (%d)", log_header, cmd);
		rc = -1;
	}
	return rc;
}

static void
snap_do_channel(struct snap_interface *snap_p)
{
	struct snap_private *priv_p = snap_p->ss_private;
	int sfd = priv_p->p_sfd;
	close(sfd);
}

struct snap_operations snap_op = {
	.ss_accept_new_channel = snap_accept_new_channel,
	.ss_do_channel = snap_do_channel,
};

int
snap_initialization(struct snap_interface *snap_p, struct snap_setup *setup)
{
	char *log_header = "snap_initialization";
	struct snap_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	snap_p->ss_op = &snap_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	snap_p->ss_private = priv_p;

	return 0;
}
