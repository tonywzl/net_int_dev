/*
 * drwc_if.h
 * 	Interface of Device Read Write Channel Module
 */

#ifndef _NID_DRWC_IF_H
#define _NID_DRWC_IF_H

#include "list.h"
#include "nid.h"
#include "scg_if.h"

struct drwc_interface;
struct drwc_operations {
	int			(*drwc_accept_new_channel)(struct drwc_interface *, int);
	void			(*drwc_do_channel)(struct drwc_interface *, struct scg_interface *);
	void			(*drwc_cleanup)(struct drwc_interface *);
};

struct drwc_interface {
	void			*drwc_private;
	struct drwc_operations	*drwc_op;
};

struct drwcg_interface;
struct umpk_interface;
struct drwc_setup {
	struct drwcg_interface	*drwcg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int drwc_initialization(struct drwc_interface *, struct drwc_setup *);
#endif
