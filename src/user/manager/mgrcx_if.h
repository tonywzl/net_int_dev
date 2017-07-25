/*
 * mgrcx_if.h
 * 	Interface of Manager to CX Channel Module
 */
#ifndef _MGRCX_IF_H
#define _MGRCX_IF_H

#include <sys/types.h>

struct mgrcx_stat {
};

struct mgrcx_interface;
struct mgrcx_operations {
	int	(*cx_display)(struct mgrcx_interface *, char *, uint8_t);
	int	(*cx_display_all)(struct mgrcx_interface *, uint8_t);
	int	(*cx_hello)(struct mgrcx_interface *);
};

struct mgrcx_interface {
	void				*cx_private;
	struct mgrcx_operations	*cx_op;
};

struct umpk_interface;
struct mgrcx_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrcx_initialization(struct mgrcx_interface *, struct mgrcx_setup *);
#endif
