/*
 * inic_if.h
 *	Interface of ini Channel Module
 */

#ifndef _NID_INIC_IF_H
#define _NID_INIC_IF_H

#include "list.h"
#include "nid.h"

struct inic_interface;
struct inic_operations {
	int			(*i_accept_new_channel)(struct inic_interface *, int);
	void			(*i_do_channel)(struct inic_interface *);
	void			(*i_cleanup)(struct inic_interface *);
};

struct inic_interface {
	void			*i_private;
	struct inic_operations	*i_op;
};

struct ini_interface;
struct umpk_interface;
struct inic_setup {
	struct ini_interface	*ini;
	struct umpk_interface	*umpk;
	int			*rsfd;
};

extern int inic_initialization(struct inic_interface *, struct inic_setup *);
#endif
