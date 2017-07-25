/*
 * umpk_if.h
 * 	Interface of Userland Message Packaging Module
 */
#ifndef _NID_UMPK_IF_H
#define _NID_UMPK_IF_H

#include <stdint.h>
#include "nid_shared.h"


struct umpk_interface;
struct umpk_operations {
	int	(*um_decode)(struct umpk_interface *, char *, uint32_t, int ctype, void *);
	int	(*um_encode)(struct umpk_interface *, char *, uint32_t *, int ctype, void *);
	void	(*um_cleanup)(struct umpk_interface *);
};

struct umpk_interface {
	void			*um_private;
	struct umpk_operations	*um_op;
};

struct umpk_setup {
};

extern int umpk_initialization(struct umpk_interface *, struct umpk_setup *);

#endif
