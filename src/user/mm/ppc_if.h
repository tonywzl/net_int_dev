/*
 * ppc_if.h
 * 	Interface of Page Pool Channel Module
 */

#ifndef _NID_PPC_IF_H
#define _NID_PPC_IF_H

#include "list.h"
#include "nid.h"
#include "scg_if.h"

struct ppc_interface;
struct ppc_operations {
	int			(*ppc_accept_new_channel)(struct ppc_interface *, int);
	void			(*ppc_do_channel)(struct ppc_interface *, struct scg_interface *);
	void			(*ppc_cleanup)(struct ppc_interface *);
};

struct ppc_interface {
	void			*ppc_private;
	struct ppc_operations	*ppc_op;
};

struct ppg_interface;
struct umpk_interface;
struct ppc_setup {
	struct ppg_interface	*ppg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int ppc_initialization(struct ppc_interface *, struct ppc_setup *);
#endif
