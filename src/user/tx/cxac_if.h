/*
 * cxac_if.h
 * 	Interface of Control Exchange Active Channel Module
 */

#ifndef _NID_CXAC_IF_H
#define _NID_CXAC_IF_H

struct cxac_interface;
struct cxac_operations {
	int			(*cxc_accept_new_channel)(struct cxac_interface *, int);
	void			(*cxc_do_channel)(struct cxac_interface *);
	void			(*cxc_cleanup)(struct cxac_interface *);
};

struct cxac_interface {
	void			*cxc_private;
	struct cxac_operations	*cxc_op;
};

struct cxacg_interface;
struct umpk_interface;
struct cxac_setup {
	struct cxacg_interface	*cxacg;
	struct cxag_interface	*cxag;
	struct umpk_interface	*umpk;
	int 			*rsfd;
};

extern int cxac_initialization(struct cxac_interface *, struct cxac_setup *);
#endif
