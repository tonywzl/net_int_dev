/*
 * reptg_if.h
 *	Interface of Replication Target Guardian Module
 */
#ifndef NID_REPTG_IF_H
#define NID_REPTG_IF_H

struct reptg_interface;
struct reptg_operations {
	struct rept_interface*	(*rtg_search_rt)(struct reptg_interface *, char *);
	struct rept_interface*	(*rtg_search_and_create_rt)(struct reptg_interface *, char *);
	struct rept_setup*	(*rtg_get_all_rt_setup)(struct reptg_interface *, int *);
	char **			(*rtg_get_working_rt_name)(struct reptg_interface *, int *);
};

struct reptg_interface {
	void			*rtg_private;
	struct reptg_operations	*rtg_op;
};

struct lstn_interface;
struct ini_interface;
struct reptg_setup{
	struct ini_interface		*ini;
	struct tpg_interface		*tpg;
	struct allocator_interface	*allocator;
	struct dxpg_interface           *dxpg;
	};

extern int reptg_initialization(struct reptg_interface *, struct reptg_setup *);
#endif
