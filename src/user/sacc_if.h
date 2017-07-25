/*
 * sacc_if.h
 * 	Interface of Server Agent Channel (sac) Channel Module
 */

#ifndef _NID_SACC_IF_H
#define _NID_SACC_IF_H

#include "list.h"
#include "nid.h"
#include "scg_if.h"

struct sacc_interface;
struct sacc_operations {
	int			(*sacc_accept_new_channel)(struct sacc_interface *, int);
	void			(*sacc_do_channel)(struct sacc_interface *, struct scg_interface *);
	void			(*sacc_cleanup)(struct sacc_interface *);
};

struct sacc_interface {
	void			*sacc_private;
	struct sacc_operations	*sacc_op;
};

struct sacg_interface;
struct umpk_interface;
struct sacc_setup {
	struct sacg_interface	*sacg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int sacc_initialization(struct sacc_interface *, struct sacc_setup *);
#endif
