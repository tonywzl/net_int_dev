/*
 * dxacg_if.h
 * 	Interface of Data Exchange Active Channel Guardian Module
 */

#ifndef NID_DXACG_IF_H
#define NID_DXACG_IF_H

struct dxacg_interface;
struct dxacg_operations {
	void*			(*dxcg_accept_new_channel)(struct dxacg_interface *, int);
	void			(*dxcg_do_channel)(struct dxacg_interface *, void *);
};

struct dxacg_interface {
	void			*dxcg_private;
	struct dxacg_operations	*dxcg_op;
};

struct dxag_interface;
struct umpk_interface;
struct dxacg_setup {
	struct dxag_interface		*dxag;
	struct umpk_interface		*umpk;
};

extern int dxacg_initialization(struct dxacg_interface *, struct dxacg_setup *);
#endif
