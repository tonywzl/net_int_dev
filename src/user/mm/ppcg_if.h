/*
 * ppcg_if.h
 * 	Interface of Page Pool Channel Guardian Module
 */
#ifndef NID_PPCG_IF_H
#define NID_PPCG_IF_H

#include "scg_if.h"

struct ppg_interface;
struct ppcg_interface;
struct ppcg_operations {
	void*			(*ppcg_accept_new_channel)(struct ppcg_interface *, int, char *);
	void			(*ppcg_do_channel)(struct ppcg_interface *, void *, struct scg_interface *);
	struct ppg_interface*	(*ppcg_get_ppg)(struct ppcg_interface *);
};

struct ppcg_interface {
	void			*ppcg_private;
	struct ppcg_operations	*ppcg_op;
};

struct umpk_interface;
struct ppcg_setup {
	struct ppg_interface	*ppg;
	struct umpk_interface	*umpk;
};

extern int ppcg_initialization(struct ppcg_interface *, struct ppcg_setup *);
#endif
