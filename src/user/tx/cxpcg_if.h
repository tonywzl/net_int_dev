/*
 * cxpcg_if.h
 * 	Interface of Control Exchange Passive Channel Guardian Module
 */

#ifndef NID_CXPCG_IF_H
#define NID_CXPCG_IF_H

struct cxpcg_interface;
struct cxpcg_operations {
	void*			(*cxcg_accept_new_channel)(struct cxpcg_interface *, int);
	void			(*cxcg_do_channel)(struct cxpcg_interface *, void *);
};

struct cxpcg_interface {
	void			*cxcg_private;
	struct cxpcg_operations	*cxcg_op;
};

struct cxpg_interface;
struct umpk_interface;
struct cxpcg_setup {
	struct cxpg_interface		*cxpg;
	struct umpk_interface		*umpk;
};

extern int cxpcg_initialization(struct cxpcg_interface *, struct cxpcg_setup *);
#endif
