/*
 * tpc_if.h
 *	Interface of Read Cache Channel Module
 */

#ifndef _NID_TPC_IF_H
#define _NID_TPC_IF_H

#include "list.h"
#include "nid.h"
#include "scg_if.h"

struct tpc_interface;
struct tpc_operations {
	int			(*t_accept_new_channel)(struct tpc_interface *, int);
	void			(*t_do_channel)(struct tpc_interface *, struct scg_interface *);
	void			(*t_cleanup)(struct tpc_interface *);
};

struct tpc_interface {
	void			*t_private;
	struct tpc_operations	*t_op;
};

struct tpg_interface;
struct umpk_interface;
struct tpc_setup {
	struct tpg_interface	*tpg;
	struct umpk_interface	*umpk;
	int			*rsfd;
};

extern int tpc_initialization(struct tpc_interface *, struct tpc_setup *);
#endif
