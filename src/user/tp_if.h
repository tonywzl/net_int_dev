/*
 * tp_if.h
 * 	Interface of Thread Pool Module
 */

#ifndef _TP_IF_H
#define _TP_IF_H

#include <stdint.h>
#include <time.h>
#include "list.h"
#include "nid_shared.h"

/*
 * job header
 */
struct tp_interface;
struct tp_jobheader {
	struct list_head	jh_list;
	int			jh_delay;
	char			jh_d_in_use;	// delay job in use
	char			jh_j_in_use;	// job in use
	void			(*jh_do)(struct tp_jobheader *);
	void			(*jh_free)(struct tp_jobheader *);
};

struct tp_stat {
	uint16_t	s_nused;
	uint16_t	s_nfree;
	uint16_t	s_workers;
	uint16_t	s_max_workers;
	uint32_t	s_no_free;
};

struct tp_operations {
	int		(*t_job_insert)(struct tp_interface *, struct tp_jobheader *);
	int		(*t_delay_job_insert)(struct tp_interface *, struct tp_jobheader *, int);
	void		(*t_stop)(struct tp_interface *);
	uint16_t	(*t_get_max_workers)(struct tp_interface *);
	void		(*t_set_max_workers)(struct tp_interface *, uint16_t);
	void		(*t_get_stat)(struct tp_interface *, struct tp_stat *);
	void		(*t_reset_stat)(struct tp_interface *);
	void		(*t_cleanup)(struct tp_interface *);
	char*		(*t_get_name)(struct tp_interface *);
	int		(*t_get_job_in_use)(struct tp_interface *, struct tp_jobheader *);
	void		(*t_destroy)(struct tp_interface *);
};

struct tp_interface {
	void			*t_private;	// used internal
	struct tp_operations	*t_op;		// operations
};

struct tp_setup {
	char			name[NID_MAX_TPNAME];
	struct pp2_interface	*pp2;
	int			min_workers;
	int			max_workers;
	int			extend;
	int			delay;
};

extern int tp_initialization(struct tp_interface *, struct tp_setup *);

#endif
