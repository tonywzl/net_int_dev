/*
 * twcc_if.h
 * 	Interface of Write Through Cache (twc) Channel Module
 */

#ifndef _NID_TWCC_IF_H
#define _NID_TWCC_IF_H

#include "list.h"
#include "nid.h"

struct twcc_interface;
struct twcc_operations {
	int			(*w_accept_new_channel)(struct twcc_interface *, int);
	void			(*w_do_channel)(struct twcc_interface *);
	void			(*w_cleanup)(struct twcc_interface *);
};

struct twcc_interface {
	void			*w_private;
	struct twcc_operations	*w_op;
};

struct twcg_interface;
struct umpk_interface;
struct twcc_setup {
	struct twcg_interface	*twcg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int twcc_initialization(struct twcc_interface *, struct twcc_setup *);
#endif
