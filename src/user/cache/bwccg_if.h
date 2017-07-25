/*
 * bwccg_if.h
 * 	Interface of None-Memory Write Cache Channel Guardian Module
 */
#ifndef NID_BWCCG_IF_H
#define NID_BWCCG_IF_H

#include "scg_if.h"

struct bwcg_interface;
struct bwccg_interface;
struct bwccg_operations {
	void*			(*wcg_accept_new_channel)(struct bwccg_interface *, int, char *);
	void			(*wcg_do_channel)(struct bwccg_interface *, void *, struct scg_interface *);
	struct bwcg_interface*	(*wcg_get_bwcg)(struct bwccg_interface *);
};

struct bwccg_interface {
	void			*wcg_private;
	struct bwccg_operations	*wcg_op;
};

struct umpk_interface;
struct bwccg_setup {
	struct bwcg_interface	*bwcg;
	struct umpk_interface	*umpk;
};

extern int bwccg_initialization(struct bwccg_interface *, struct bwccg_setup *);
#endif
