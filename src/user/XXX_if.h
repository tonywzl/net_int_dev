/*
 * XXX_if.h
 * 	Interface of Message Waiting Queue Module
 */
#ifndef NID_XXX_IF_H
#define NID_XXX_IF_H

struct XXX_interface;
struct XXX_operations {
};

struct XXX_interface {
	void			*s_private;
	struct XXX_operations	*s_op;
};

struct XXX_setup {
};

extern int XXX_initialization(struct XXX_interface *, struct XXX_setup *);
#endif
