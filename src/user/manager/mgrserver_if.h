/*
 * mgrserver_if.h
 *	Interface of Manager to Server Channel Module
 */
#ifndef _MGRSERVER_IF_H
#define _MGRSERVER_IF_H

#include <sys/types.h>

struct mgrserver_interface;
struct mgrserver_operations {
	int	(*server_version)(struct mgrserver_interface *);
};

struct mgrserver_interface {
	void				*ser_private;
	struct mgrserver_operations		*ser_op;
};

struct umpk_interface;
struct mgrserver_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrserver_initialization(struct mgrserver_interface *, struct mgrserver_setup *);
#endif
