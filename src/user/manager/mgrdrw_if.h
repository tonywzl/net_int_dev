/*
 * mgrdrw_if.h
 * 	Interface of Manager to DRW Channel Module
 */
#ifndef _MGRDRW_IF_H
#define _MGRDRW_IF_H

#include <sys/types.h>

struct mgrdrw_stat {
};

struct mgrdrw_interface;
struct mgrdrw_operations {
	int 	(*drw_add)(struct mgrdrw_interface *, char *, char *, uint8_t);
	int 	(*drw_delete)(struct mgrdrw_interface *, char *);
	int 	(*drw_ready)(struct mgrdrw_interface *, char *);
	int	(*drw_display)(struct mgrdrw_interface *, char *, uint8_t);
	int	(*drw_display_all)(struct mgrdrw_interface *, uint8_t);
	int	(*drw_hello)(struct mgrdrw_interface *);
};

struct mgrdrw_interface {
	void				*drw_private;
	struct mgrdrw_operations	*drw_op;
};

struct umpk_interface;
struct mgrdrw_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrdrw_initialization(struct mgrdrw_interface *, struct mgrdrw_setup *);
#endif
