/*
 * sm_if.h
 * 	Interface of Service Manager Module
 */
#ifndef _SM_IF_H
#define _SM_IF_H

struct sm_interface;
struct sm_operations {
	void	(*s_display)(struct sm_interface *);
};

struct sm_interface {
	void			*s_private;
	struct sm_operations	*s_op;
};

struct ini_interface;
struct sm_setup {
	struct ini_interface	**ini;
};

extern int sm_initialization(struct sm_interface *, struct sm_setup *);
#endif
