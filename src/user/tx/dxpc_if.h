/*
 * dxpc_if.h
 * 	Interface of Data Exchange Passive Channel Module
 */

#ifndef _NID_dxpC_IF_H
#define _NID_dxpC_IF_H

struct dxpc_interface;
struct dxpc_operations {
	int			(*dxc_accept_new_channel)(struct dxpc_interface *, int);
	void			(*dxc_do_channel)(struct dxpc_interface *);
	void			(*dxc_cleanup)(struct dxpc_interface *);
};

struct dxpc_interface {
	void			*dxc_private;
	struct dxpc_operations	*dxc_op;
};

struct dxpg_interface;
struct umpk_interface;
struct dxpc_setup {
	struct dxpg_interface	*dxpg;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int dxpc_initialization(struct dxpc_interface *, struct dxpc_setup *);
#endif
