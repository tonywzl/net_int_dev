/*
 * dxpcg_if.h
 * 	Interface of Data Exchange Passive Channel Guardian Module
 */

#ifndef NID_dxpCG_IF_H
#define NID_dxpCG_IF_H

struct dxpcg_interface;
struct dxpcg_operations {
	void*			(*dxcg_accept_new_channel)(struct dxpcg_interface *, int);
	void			(*dxcg_do_channel)(struct dxpcg_interface *, void *);
};

struct dxpcg_interface {
	void			*dxcg_private;
	struct dxpcg_operations	*dxcg_op;
};

struct dxpg_interface;
struct umpk_interface;
struct dxpcg_setup {
	struct dxpg_interface		*dxpg;
	struct umpk_interface		*umpk;
};

extern int dxpcg_initialization(struct dxpcg_interface *, struct dxpcg_setup *);
#endif
