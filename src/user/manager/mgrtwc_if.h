/*
 * mgrtwc_if.h
 * 	Interface of Manager to TWC Channel Module
 */
#ifndef _MGRTWC_IF_H
#define _MGRTWC_IF_H

#include <sys/types.h>

struct mgrtwc_stat {
};

struct mgrtwc_interface;
struct mgrtwc_operations {
	int 	(*tw_information_flushing)(struct mgrtwc_interface *, char *);
	int 	(*tw_information_stat)(struct mgrtwc_interface *, char *);
	int 	(*tw_information_throughput_stat)(struct mgrtwc_interface *, char *);
	int 	(*tw_throughput_stat_reset)(struct mgrtwc_interface *, char *);
	int 	(*tw_information_rw_stat)(struct mgrtwc_interface *, char *, char *);
	int	(*tw_display)(struct mgrtwc_interface *, char *, uint8_t);
	int	(*tw_display_all)(struct mgrtwc_interface *, uint8_t);
	int	(*tw_hello)(struct mgrtwc_interface *);
};

struct mgrtwc_interface {
	void				*tw_private;
	struct mgrtwc_operations	*tw_op;
};

struct umpk_interface;
struct mgrtwc_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrtwc_initialization(struct mgrtwc_interface *, struct mgrtwc_setup *);
#endif
