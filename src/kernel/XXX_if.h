/*
 * XXX_if.h
 * 	Interface of XXX Moulde
 */

#ifndef _XXX_IF_H
#define _XXX_IF_H

#include <linux/list.h>

struct XXX_interface;
struct XXX_operations {
};

struct XXX_interface {
	void			*XXX_private;
	struct XXX_operations	*XXX_op;
};

struct XXX_setup {
};

extern int XXX_initialization(struct XXX_interface *, struct XXX_setup *);

#endif
