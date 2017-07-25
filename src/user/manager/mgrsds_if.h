/*
 * mgrsds_if.h
 * 	Interface of Manager to SDS Channel Module
 */
#ifndef _MGRSDS_IF_H
#define _MGRSDS_IF_H

#include <sys/types.h>

struct mgrsds_stat {
};

struct mgrsds_interface;
struct mgrsds_operations {
	int 	(*sds_add)(struct mgrsds_interface *, char *, char *, char *, char *);
	int 	(*sds_delete)(struct mgrsds_interface *, char *);
	int	(*sds_display)(struct mgrsds_interface *, char*, uint8_t);
	int	(*sds_display_all)(struct mgrsds_interface *, uint8_t);
	int	(*sds_hello)(struct mgrsds_interface *);
};

struct mgrsds_interface {
	void				*sds_private;
	struct mgrsds_operations	*sds_op;
};

struct umpk_interface;
struct mgrsds_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrsds_initialization(struct mgrsds_interface *, struct mgrsds_setup *);
#endif
