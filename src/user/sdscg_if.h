/*
 * sdscg_if.h
 * 	Interface of Split Data Stream Channel Guardian Module
 */
#ifndef NID_SDSCG_IF_H
#define NID_SDSCG_IF_H

#include "scg_if.h"

struct sdsg_interface;
struct sdscg_interface;
struct sdscg_operations {
	void*			(*sdscg_accept_new_channel)(struct sdscg_interface *, int, char *);
	void			(*sdscg_do_channel)(struct sdscg_interface *, void *, struct scg_interface *);
	struct sdsg_interface*	(*sdscg_get_sdsg)(struct sdscg_interface *);
};

struct sdscg_interface {
	void			*sdscg_private;
	struct sdscg_operations	*sdscg_op;
};

struct umpk_interface;
struct sdscg_setup {
	struct sdsg_interface	*sdsg;
	struct umpk_interface	*umpk;
};

extern int sdscg_initialization(struct sdscg_interface *, struct sdscg_setup *);
#endif
