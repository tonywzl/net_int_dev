/*
 * cxacg_if.h
 * 	Interface of Control Exchange Active Channel Guardian Module
 */

#ifndef NID_CXACG_IF_H
#define NID_CXACG_IF_H

struct cxacg_interface;
struct cxacg_operations {
	void*			(*cxcg_accept_new_channel)(struct cxacg_interface *, int);
	void			(*cxcg_do_channel)(struct cxacg_interface *, void *);
};

struct cxacg_interface {
	void			*cxcg_private;
	struct cxacg_operations	*cxcg_op;
};

struct cxag_interface;
struct umpk_interface;
struct cxacg_setup {
	struct cxag_interface		*cxag;
	struct umpk_interface		*umpk;
};

extern int cxacg_initialization(struct cxacg_interface *, struct cxacg_setup *);
#endif
