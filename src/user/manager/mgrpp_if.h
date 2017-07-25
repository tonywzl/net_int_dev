/*
 * mgrpp_if.h
 * 	Interface of Manager to PP Channel Module
 */
#ifndef _MGRPP_IF_H
#define _MGRPP_IF_H

#include <sys/types.h>

struct mgrpp_stat {
};

struct mgrpp_interface;
struct mgrpp_operations {
	int	(*pp_stat)(struct mgrpp_interface *, char *);
	int	(*pp_stat_all)(struct mgrpp_interface *);
	int 	(*pp_add)(struct mgrpp_interface *, char *, uint8_t, uint32_t, uint32_t);
	int 	(*pp_delete)(struct mgrpp_interface *, char *);
	int	(*pp_display)(struct mgrpp_interface *, char *, uint8_t);
	int	(*pp_display_all)(struct mgrpp_interface *, uint8_t);
	int	(*pp_hello)(struct mgrpp_interface *);
};

struct mgrpp_interface {
	void				*pp_private;
	struct mgrpp_operations		*pp_op;
};

struct umpk_interface;
struct mgrpp_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrpp_initialization(struct mgrpp_interface *, struct mgrpp_setup *);
#endif
