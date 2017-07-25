/*
 * sdsc_if.h
 * 	Interface of Split Data Stream Channel Module
 */

#ifndef _NID_SDSC_IF_H
#define _NID_SDSC_IF_H

#include "list.h"
#include "nid.h"
#include "scg_if.h"

struct sdsc_interface;
struct sdsc_operations {
	int			(*sdsc_accept_new_channel)(struct sdsc_interface *, int);
	void			(*sdsc_do_channel)(struct sdsc_interface *, struct scg_interface *);
	void			(*sdsc_cleanup)(struct sdsc_interface *);
};

struct sdsc_interface {
	void			*sdsc_private;
	struct sdsc_operations	*sdsc_op;
};

struct sdsg_interface;
struct umpk_interface;
struct sdsc_setup {
	struct sdsg_interface	*sdsg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int sdsc_initialization(struct sdsc_interface *, struct sdsc_setup *);
#endif
