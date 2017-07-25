/*
 * repsg_if.h
 *	Interface of Replication Src Guardian Module
 */
#ifndef NID_REPSG_IF_H
#define NID_REPSG_IF_H

struct repsg_interface;
struct repsg_operations {
	struct reps_interface*	(*rsg_search_rs)(struct repsg_interface *, char *);
	struct reps_interface*	(*rsg_search_and_create_rs)(struct repsg_interface *, char *);
	struct reps_setup*	(*rsg_get_all_rs_setup)(struct repsg_interface *, int *);
	char **			(*rsg_get_working_rs_name)(struct repsg_interface *, int *);
};

struct repsg_interface {
	void			*rsg_private;
	struct repsg_operations	*rsg_op;
};

struct lstn_interface;
struct ini_interface;
struct repsg_setup{
	struct ini_interface		*ini;
	struct tpg_interface		*tpg;
	struct allocator_interface	*allocator;
	struct umpk_interface		*umpk;
	struct dxag_interface		*dxag;
};

extern int repsg_initialization(struct repsg_interface *, struct repsg_setup *);
#endif
