/*
 * mgrdx_if.h
 * 	Interface of Manager to DX Channel Module
 */
#ifndef _MGRDX_IF_H
#define _MGRDX_IF_H

#include <sys/types.h>

struct mgrdx_stat {
};

struct mgrdx_interface;
struct mgrdx_operations {
	int 	(*dx_create)(struct mgrdx_interface *, char *, char *);
	int	(*dx_display)(struct mgrdx_interface *, char *, uint8_t);
	int	(*dx_display_all)(struct mgrdx_interface *, uint8_t);
	int	(*dx_hello)(struct mgrdx_interface *);
};

struct mgrdx_interface {
	void			*dx_private;
	struct mgrdx_operations	*dx_op;
};

struct umpk_interface;
struct mgrdx_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrdx_initialization(struct mgrdx_interface *, struct mgrdx_setup *);
#endif
