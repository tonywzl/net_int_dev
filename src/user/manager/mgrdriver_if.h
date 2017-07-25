/*
 * mgrdriver_if.h
 *	Interface of Manager to Driver Channel Module
 */
#ifndef _MGRDRIVER_IF_H
#define _MGRDRIVER_IF_H

#include <sys/types.h>

struct mgrdriver_interface;
struct mgrdriver_operations {
	int	(*driver_version)(struct mgrdriver_interface *);
};

struct mgrdriver_interface {
	void				*drv_private;
	struct mgrdriver_operations		*drv_op;
};

struct umpk_interface;
struct mgrdriver_setup {
	char			*ipstr;
	u_short			port;
	struct umpka_interface	*umpka;
};

extern int mgrdriver_initialization(struct mgrdriver_interface *, struct mgrdriver_setup *);
#endif
