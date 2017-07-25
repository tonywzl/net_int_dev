/*
 * reptc_if.h
 *	Interface of Replication Target Channel Module
 */

#ifndef _NID_REPTC_IF_H
#define _NID_REPTC_IF_H

#include "list.h"
#include "nid.h"

struct reptc_interface;
struct reptc_operations {
	int			(*rtc_accept_new_channel)(struct reptc_interface *, int);
	void			(*rtc_do_channel)(struct reptc_interface *);
	void			(*rtc_cleanup)(struct reptc_interface *);
};

struct reptc_interface {
	void			*r_private;
	struct reptc_operations	*r_op;
};

struct rept_interface;
struct umpk_interface;
struct reptc_setup {
	struct reptg_interface	*reptg;
	struct umpk_interface	*umpk;
	int			*rsfd;
};

extern int reptc_initialization(struct reptc_interface *, struct reptc_setup *);
#endif
