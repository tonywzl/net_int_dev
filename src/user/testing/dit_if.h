/*
 * dit_if.h
 * 	Interface of Data Integrity Testing Module
 */
#ifndef NID_DIT_IF_H
#define NID_DIT_IF_H

#include <stdint.h>

struct dit_interface;
struct dit_operations {
	void	(*d_run)(struct dit_interface *);
};

struct dit_interface {
	void			*d_private;
	struct dit_operations	*d_op;
};

struct tp_interface;
struct dcn_interface;
struct rdg_interface;
struct dit_setup {
	struct ini_interface	*ini;
	struct tp_interface	*tp;
	struct dcn_interface	*dcn;
	struct rdg_interface	*rdg;
	int			nr_threads;
	char			*devname;
	char			*bck_devname;
	uint32_t		time_to_run;
};

extern int dit_initialization(struct dit_interface *, struct dit_setup *);
#endif
