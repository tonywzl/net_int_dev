/*
 * mrwcg_if.h
 * 	Interface of Meta Server Read Write Guardian Module
 */
#ifndef NID_MRWCG_IF_H
#define NID_MRWCG_IF_H

#include "scg_if.h"

struct mrwg_interface;
struct mrwcg_interface;
struct mrwcg_operations {
	void*			(*mrwcg_accept_new_channel)(struct mrwcg_interface *, int, char *);
	void			(*mrwcg_do_channel)(struct mrwcg_interface *, void *, struct scg_interface *);
	struct mrwg_interface*	(*mrwcg_get_mrwg)(struct mrwcg_interface *);
};

struct mrwcg_interface {
	void			*mrwcg_private;
	struct mrwcg_operations	*mrwcg_op;
};

struct umpk_interface;
struct mrwcg_setup {
	struct mrwg_interface	*mrwg;
	struct umpk_interface	*umpk;
};

extern int mrwcg_initialization(struct mrwcg_interface *, struct mrwcg_setup *);
#endif
