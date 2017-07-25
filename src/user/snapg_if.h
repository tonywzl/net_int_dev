/*
 * snapg_if.h
 * 	Interface of Snapshot Guardian Module
 */
#ifndef NID_SNAPG_IF_H
#define NID_SNAPG_IF_H

struct snapg_interface;
struct snapg_operations {
	void*	(*ssg_accept_new_channel)(struct snapg_interface *, int);
	void	(*ssg_do_channel)(struct snapg_interface *, void *);
};

struct snapg_interface {
	void			*ssg_private;
	struct snapg_operations	*ssg_op;
};

struct snapg_setup {
};

extern int snapg_initialization(struct snapg_interface *, struct snapg_setup *);
#endif
