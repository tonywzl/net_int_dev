/*
 * mrwc_if.h
 * 	Interface of Memoey Write Cache (mrw) Channel Module
 */

#ifndef _NID_MRWC_IF_H
#define _NID_MRWC_IF_H

#include "list.h"
#include "nid.h"
#include "scg_if.h"

struct mrwc_interface;
struct mrwc_operations {
	int			(*w_accept_new_channel)(struct mrwc_interface *, int);
	void			(*w_do_channel)(struct mrwc_interface *, struct scg_interface *);
	void			(*w_cleanup)(struct mrwc_interface *);
};

struct mrwc_interface {
	void			*w_private;
	struct mrwc_operations	*w_op;
};

struct mrwcg_interface;
struct umpk_interface;
struct mrwc_setup {
	struct mrwcg_interface	*mrwcg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int mrwc_initialization(struct mrwc_interface *, struct mrwc_setup *);
#endif
