/*
 * wcc_if.h
 * 	Interface of Write Cache Channel Module
 */

#ifndef _NID_WCC_IF_H
#define _NID_WCC_IF_H

#include "list.h"
#include "nid.h"

struct wcc_interface;
struct wcc_operations {
	int			(*w_accept_new_channel)(struct wcc_interface *, int);
	void			(*w_do_channel)(struct wcc_interface *);
	void			(*w_cleanup)(struct wcc_interface *);
};

struct wcc_interface {
	void			*w_private;
	struct wcc_operations	*w_op;
};

struct wccg_interface;
struct umpk_interface;
struct wcc_setup {
	struct wccg_interface	*wccg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int wcc_initialization(struct wcc_interface *, struct wcc_setup *);
#endif
