/*
 * dxac_if.h
 * 	Interface of Data Exchange Active Channel Module
 */

#ifndef _NID_DXAC_IF_H
#define _NID_DXAC_IF_H

struct dxac_interface;
struct dxac_operations {
	int			(*dxc_accept_new_channel)(struct dxac_interface *, int);
	void			(*dxc_do_channel)(struct dxac_interface *);
	void			(*dxc_cleanup)(struct dxac_interface *);
};

struct dxac_interface {
	void			*dxc_private;
	struct dxac_operations	*dxc_op;
};

struct dxacg_interface;
struct umpk_interface;
struct dxac_setup {
	struct dxacg_interface	*dxacg;
	struct umpk_interface	*umpk;
	struct dxag_interface	*dxag;
	int 			*rsfd;
};

extern int dxac_initialization(struct dxac_interface *, struct dxac_setup *);
#endif
