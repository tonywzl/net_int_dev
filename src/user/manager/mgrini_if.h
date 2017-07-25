/*
 * mgrini_if.h
 *	Interface of Manager to ini Channel Module
 */
#ifndef _MGRINI_IF_H
#define _MGRINI_IF_H

#include <sys/types.h>

struct mgrini_interface;
struct mgrini_operations {
	int	(*ini_display_section_detail)(struct mgrini_interface *, char *, uint8_t);
	int	(*ini_display_all)(struct mgrini_interface *, uint8_t);
	int	(*ini_display_section)(struct mgrini_interface *, uint8_t);
	int	(*ini_hello)(struct mgrini_interface *);
};

struct mgrini_interface {
	void				*i_private;
	struct mgrini_operations	*i_op;
};

struct umpk_interface;
struct mgrini_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrini_initialization(struct mgrini_interface *, struct mgrini_setup *);
#endif
