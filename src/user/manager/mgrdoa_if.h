/*
 * mgrdoa_if.h
 *
 * Interface of Manage Delegation of Authority  Module
 */

#ifndef _mgrdoa_IF_H
#define _mgrdoa_IF_H

#include <sys/types.h>


struct mgrdoa_interface;
struct umessage_doa_information;
struct mgrdoa_operations {
	int (*md_doa_request)(struct mgrdoa_interface *, struct umessage_doa_information *);
	int (*md_doa_check)(struct mgrdoa_interface *, struct umessage_doa_information *);
	int (*md_doa_release)(struct mgrdoa_interface *, struct umessage_doa_information *);
	int (*md_doa_hello)(struct mgrdoa_interface *);
};

struct mgrdoa_interface {
	void			*md_private;
	struct mgrdoa_operations	*md_op;
};

struct mpk_interface;
struct mgrdoa_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrdoa_initialization(struct mgrdoa_interface *, struct mgrdoa_setup *);
#endif
