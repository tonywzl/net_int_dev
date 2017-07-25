/*
 * mgrwc_if.h
 * 	Interface of Manager to WC Channel Module
 */
#ifndef _MGRWC_IF_H
#define _MGRWC_IF_H

#include <sys/types.h>

struct mgrwc_stat {
};

struct mgrwc_interface;
struct mgrwc_operations {
	int 	(*wc_information_flushing)(struct mgrwc_interface *, char *);
	int 	(*wc_information_stat)(struct mgrwc_interface *, char *);
	int	(*wc_hello)(struct mgrwc_interface *);
};

struct mgrwc_interface {
	void			*wc_private;
	struct mgrwc_operations	*wc_op;
};

struct umpk_interface;
struct mgrwc_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrwc_initialization(struct mgrwc_interface *, struct mgrwc_setup *);
#endif
