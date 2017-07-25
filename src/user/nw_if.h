/*
 * nw_if.h
 * 	Interface of Network Module
 */
#ifndef NID_NW_IF_H
#define NID_NW_IF_H

#include <sys/types.h>
#include "list.h"

struct tpg_interface;
struct cm_interface;
struct nw_interface;
struct nw_operations {
	void	(*n_insert_waiting_io)(struct nw_interface *, struct list_head *);
	void	(*n_cleanup)(struct nw_interface *);
};

struct nw_interface {
	void			*n_private;
	struct nw_operations	*n_op;
};

struct nw_setup {
	struct tpg_interface	*tpg;
	struct pp2_interface	*pp2;
	struct pp2_interface	*dyn_pp2;
	void			*cg;
	char			*ipstr;
	u_short			port;
	char			type;
};

extern int nw_initialization(struct nw_interface *, struct nw_setup *);

#endif
