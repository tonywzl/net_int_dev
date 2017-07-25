/*
 * mgrtp_if.h
 *	Interface of Manager to TP Channel Module
 */
#ifndef _MGRTP_IF_H
#define _MGRTP_IF_H

#include <sys/types.h>

struct mgrtp_stat {
};

struct mgrtp_interface;
struct mgrtp_operations {
	int 	(*tp_information_stat)(struct mgrtp_interface *, char *);
	int	(*tp_information_all)(struct mgrtp_interface *);
	int	(*tp_add)(struct mgrtp_interface *, char *, uint16_t);
	int	(*tp_delete)(struct mgrtp_interface *, char *);
	int	(*tp_display)(struct mgrtp_interface *, char *,  uint8_t);
	int	(*tp_display_all)(struct mgrtp_interface *, uint8_t);
	int	(*tp_hello)(struct mgrtp_interface *);
};

struct mgrtp_interface {
	void				*t_private;
	struct mgrtp_operations		*t_op;
};

struct umpk_interface;
struct mgrtp_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrtp_initialization(struct mgrtp_interface *, struct mgrtp_setup *);
#endif
