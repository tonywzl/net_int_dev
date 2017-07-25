/*
 * devg_if.h
 * 	Interface of Device Guardian Module
 */
#ifndef NID_DEVG_IF_H
#define NID_DEVG_IF_H

struct devg_interface;
struct dev_interface;
struct devg_operations {
	struct dev_interface*	(*dg_search_and_create)(struct devg_interface *, char *);
};

struct devg_interface {
	void			*dg_private;
	struct devg_operations	*dg_op;
};

struct ini_interface;
struct devg_setup {
	struct ini_interface	*ini;
};

extern int devg_initialization(struct devg_interface *, struct devg_setup *);
#endif
