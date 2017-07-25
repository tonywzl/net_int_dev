/*
 * bwcc_if.h
 * 	Interface of Memoey Write Cache (bwc) Channel Module
 */

#ifndef _NID_BWCC_IF_H
#define _NID_BWCC_IF_H

#include "list.h"
#include "nid.h"
#include "scg_if.h"

struct bwcc_interface;
struct bwcc_operations {
	int			(*w_accept_new_channel)(struct bwcc_interface *, int);
	void			(*w_do_channel)(struct bwcc_interface *, struct scg_interface *);
	void			(*w_cleanup)(struct bwcc_interface *);
};

struct bwcc_interface {
	void			*w_private;
	struct bwcc_operations	*w_op;
};

struct bwcg_interface;
struct umpk_interface;
struct bwcc_setup {
	struct bwcg_interface	*bwcg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int bwcc_initialization(struct bwcc_interface *, struct bwcc_setup *);
#endif
