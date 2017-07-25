/*
 * cxpc_if.h
 * 	Interface of Control Exchange Passive Channel Module
 */

#ifndef _NID_CXPC_IF_H
#define _NID_CXPC_IF_H

struct cxpc_interface;
struct cxpc_operations {
	int			(*cxc_accept_new_channel)(struct cxpc_interface *, int);
	void			(*cxc_do_channel)(struct cxpc_interface *);
	void			(*cxc_cleanup)(struct cxpc_interface *);
};

struct cxpc_interface {
	void			*cxc_private;
	struct cxpc_operations	*cxc_op;
};

struct cxpg_interface;
struct umpk_interface;
struct cxpc_setup {
	struct cxpg_interface	*cxpg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int cxpc_initialization(struct cxpc_interface *, struct cxpc_setup *);
#endif
