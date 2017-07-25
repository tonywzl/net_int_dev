/*
 * repsc_if.h
 *	Interface of Replication Source Channel Module
 */

#ifndef _NID_REPSC_IF_H
#define _NID_REPSC_IF_H

#include "list.h"
#include "nid.h"

struct repsc_interface;
struct repsc_operations {
	int			(*rsc_accept_new_channel)(struct repsc_interface *, int);
	void			(*rsc_do_channel)(struct repsc_interface *);
	void			(*rsc_cleanup)(struct repsc_interface *);
};

struct repsc_interface {
	void			*r_private;
	struct repsc_operations	*r_op;
};

struct reps_interface;
struct umpk_interface;
struct repsc_setup {
	struct repsg_interface	*repsg;
	struct umpk_interface	*umpk;
	int			*rsfd;
};

extern int repsc_initialization(struct repsc_interface *, struct repsc_setup *);
#endif
