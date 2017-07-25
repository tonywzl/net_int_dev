/*
 * smc_if.h
 * 	Interface of Server Manager Channel Module
 */

#ifndef _NID_SMC_IF_H
#define _NID_SMC_IF_H

#include "list.h"
#include "nid.h"

struct smc_stat_record {
	struct list_head	r_list;
	int			r_type;
	void			*r_data;	
};

struct smc_interface;
struct smc_operations {
	int			(*s_accept_new_channel)(struct smc_interface *, int);
	void			(*s_do_channel)(struct smc_interface *);
	void			(*s_cleanup)(struct smc_interface *);
};

struct smc_interface {
	void			*s_private;
	struct smc_operations	*s_op;
};

struct smcg_interface;
struct mpk_interface;
struct smc_setup {
	struct smcg_interface	*smcg;
	struct mpk_interface	*mpk;
	int 			*rsfd;
	struct ini_interface    *ini;
	struct mqtt_interface	*mqtt;
};

extern int smc_initialization(struct smc_interface *, struct smc_setup *);
#endif
