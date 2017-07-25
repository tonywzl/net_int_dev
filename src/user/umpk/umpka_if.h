/*
 * umpka_if.h
 * 	Interface of Userland Message Packaging Agent Module
 */
#ifndef _NID_UMPKA_IF_H
#define _NID_UMPKA_IF_H

#include <stdint.h>
#include "nid_shared.h"


struct umpka_interface;
struct umpka_operations {
	int	(*um_decode)(struct umpka_interface *, char *, uint32_t, int ctype, void *);
	int	(*um_encode)(struct umpka_interface *, char *, uint32_t *, int ctype, void *);
	void	(*um_cleanup)(struct umpka_interface *);
};

struct umpka_interface {
	void			*um_private;
	struct umpka_operations	*um_op;
};

struct umpka_setup {
};

extern int umpka_initialization(struct umpka_interface *, struct umpka_setup *);

#endif
