/*
 * crcc_if.h
 * 	Interface of Read Cache Channel Module
 */

#ifndef _NID_CRCC_IF_H
#define _NID_CRCC_IF_H

#include "list.h"
#include "nid.h"
#include "scg_if.h"

struct crcc_interface;
struct crcc_operations {
	int			(*crcc_accept_new_channel)(struct crcc_interface *, int);
	void			(*crcc_do_channel)(struct crcc_interface *, struct scg_interface *);
	void			(*crcc_cleanup)(struct crcc_interface *);
};

struct crcc_interface {
	void			*r_private;
	struct crcc_operations	*r_op;
};

struct crcg_interface;
struct umpk_interface;
struct crcc_setup {
	struct crcg_interface	*crcg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int crcc_initialization(struct crcc_interface *, struct crcc_setup *);
#endif
