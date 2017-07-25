/*
 * drwcg_if.h
 * 	Interface of Device Read Write Channel Guardian Module
 */
#ifndef NID_DRWCG_IF_H
#define NID_DRWCG_IF_H

#include "scg_if.h"

struct drwg_interface;
struct drwcg_interface;
struct drwcg_operations {
	void*			(*drwcg_accept_new_channel)(struct drwcg_interface *, int, char *);
	void			(*drwcg_do_channel)(struct drwcg_interface *, void *, struct scg_interface *);
	struct drwg_interface*	(*drwcg_get_drwg)(struct drwcg_interface *);
};

struct drwcg_interface {
	void			*drwcg_private;
	struct drwcg_operations	*drwcg_op;
};

struct umpk_interface;
struct drwcg_setup {
	struct drwg_interface	*drwg;
	struct umpk_interface	*umpk;
};

extern int drwcg_initialization(struct drwcg_interface *, struct drwcg_setup *);
#endif
