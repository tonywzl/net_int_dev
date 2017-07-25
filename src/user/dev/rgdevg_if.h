/*
 * rgdevg_if.h
 * 	Interface of Regular Device Guardian Module
 */
#ifndef NID_rgdevg_IF_H
#define NID_rgdevg_IF_H

struct rgdevg_interface;
struct rgdevg_operations {
	void*	(*dg_search_and_create)(struct rgdevg_interface *, char *);
};

struct rgdevg_interface {
	void				*dg_private;
	struct rgdevg_operations	*dg_op;
};

struct ini_interface;
struct rgdevg_setup {
	struct ini_interface	*ini;
};

extern int rgdevg_initialization(struct rgdevg_interface *, struct rgdevg_setup *);
#endif
