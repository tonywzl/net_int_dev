/*
 * mgragent_if.h
 *	Interface of Manager to Server Channel Module
 */
#ifndef _MGRAGENT_IF_H
#define _MGRAGENT_IF_H

#include <sys/types.h>

struct mgragent_interface;
struct mgragent_operations {
	int	(*agent_version)(struct mgragent_interface *);
};

struct mgragent_interface {
	void				*agt_private;
	struct mgragent_operations		*agt_op;
};

struct umpk_interface;
struct mgragent_setup {
	char			*ipstr;
	u_short			port;
	struct umpka_interface	*umpka;
};

extern int mgragent_initialization(struct mgragent_interface *, struct mgragent_setup *);
#endif
