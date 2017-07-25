/*
 * amc_if.h
 * 	Interface of Agent Manager Channel Module
 */

#ifndef _NID_AMC_IF_H
#define _NID_AMC_IF_H

#include "list.h"
#include "nid.h"

struct amc_stat_record {
	struct list_head	r_list;
	int                     r_type;
	void                    *r_data;
};

struct amc_interface;
struct amc_operations {
	int			(*a_accept_new_channel)(struct amc_interface *, int);
	void			(*a_do_channel)(struct amc_interface *);
	void			(*a_cleanup)(struct amc_interface *);
};

struct amc_interface {
	void			*a_private;
	struct amc_operations	*a_op;
};

struct ascg_interface;
struct mpk_interface;
struct amc_setup {
	struct ascg_interface	*ascg;
	struct mpk_interface	*mpk;
	struct pp2_interface	*dyn2_pp2;
};

extern int amc_initialization(struct amc_interface *, struct amc_setup *);
#endif
