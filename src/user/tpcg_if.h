/*
 * tpcg_if.h
 *	Interface of Thread Pool Channel Guardian Module
 */
#ifndef NID_TPCG_IF_H
#define NID_TPCG_IF_H

#include "scg_if.h"

struct tpg_interface;
struct tpcg_interface;
struct tpcg_operations {
	void*			(*tcg_accept_new_channel)(struct tpcg_interface *, int, char *);
	void			(*tcg_do_channel)(struct tpcg_interface *, void *, struct scg_interface *);
	struct tpg_interface*	(*tcg_get_tpg)(struct tpcg_interface *);
};

struct tpcg_interface {
	void			*tcg_private;
	struct tpcg_operations *tcg_op;
};

struct umpk_interface;
struct tpcg_setup {
	struct tpg_interface *tpg;
	struct umpk_interface *umpk;
};

extern int tpcg_initialization(struct tpcg_interface *, struct tpcg_setup *);
#endif
