/*
 * mgrmrw_if.h
 * 	Interface of Manager to mrw Channel Module
 */
#ifndef _MGRMRW_IF_H
#define _MGRMRW_IF_H

#include <sys/types.h>

struct mgrmrw_stat {
};

struct mgrmrw_interface;
struct mgrmrw_operations {

	int 	(*mr_information_stat)(struct mgrmrw_interface *, char *);
	int 	(*mr_add)(struct mgrmrw_interface *, char *);
	int	(*mr_display)(struct mgrmrw_interface *, char *, uint8_t);
	int	(*mr_display_all)(struct mgrmrw_interface *, uint8_t);
	int	(*mr_hello)(struct mgrmrw_interface *);
};

struct mgrmrw_interface {
	void				*mr_private;
	struct mgrmrw_operations	*mr_op;
};

struct umpk_interface;
struct mgrmrw_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrmrw_initialization(struct mgrmrw_interface *, struct mgrmrw_setup *);
#endif
