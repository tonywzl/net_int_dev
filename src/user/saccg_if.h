/*
 * saccg_if.h
 * 	Interface of Server Agent Channel (sac) Channel Guardian Module
 */
#ifndef NID_SACCG_IF_H
#define NID_SACCG_IF_H

#include "scg_if.h"

struct saccg_interface;
struct sac_interface;
struct saccg_operations {
	void*			(*saccg_accept_new_channel)(struct saccg_interface *, int, char *);
	void			(*saccg_do_channel)(struct saccg_interface *, void *, struct scg_interface *);
	struct sacg_interface*	(*saccg_get_sacg)(struct saccg_interface *);
};

struct saccg_interface {
	void			*saccg_private;
	struct saccg_operations	*saccg_op;
};

struct sacg_interface;
struct umpk_interface;
struct saccg_setup {
	struct sacg_interface	*sacg;
	struct umpk_interface	*umpk;
};

extern int saccg_initialization(struct saccg_interface *, struct saccg_setup *);
#endif
