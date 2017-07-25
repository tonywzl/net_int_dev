/*
 * crccg_if.h
 * 	Interface of Read Cache Channel Guardian Module
 */
#ifndef NID_RCCG_IF_H
#define NID_RCCG_IF_H

#include "scg_if.h"

struct crccg_interface;
struct crccg_operations {
	void*			(*crg_accept_new_channel)(struct crccg_interface *, int, char *);
	void			(*crg_do_channel)(struct crccg_interface *, void *, struct scg_interface *);
};

struct crccg_interface {
	void			*crg_private;
	struct crccg_operations	*crg_op;
};

struct crcg_interface;
struct umpk_interface;
struct crccg_setup {
	struct crcg_interface	*crcg;
	struct umpk_interface	*umpk;
};

extern int crccg_initialization(struct crccg_interface *, struct crccg_setup *);
#endif
