/*
 * mrwg_if.h
 * 	Interface of Meta Server Read Write Guardian Module
 */
#ifndef NID_MRWG_IF_H
#define NID_MRWG_IF_H

#define MRWG_MAX_MRW	2

struct mrw_stat;
struct mrwg_interface;
struct mrwg_operations {
	struct rw_interface*	(*rwg_search_mrw)(struct mrwg_interface *, char *);
	struct rw_interface*	(*rwg_search_and_create_mrw)(struct mrwg_interface *, char *);
	int			(*rwg_add_mrw)(struct mrwg_interface *, char *);
	void			(*rwg_get_mrw_status)(struct mrwg_interface *, char *, struct mrw_stat *);
	struct mrw_setup*	(*rwg_get_all_mrw_setup)(struct mrwg_interface *, int *);
	int*			(*rwg_get_working_mrw_index)(struct mrwg_interface *, int *);
};

struct mrwg_interface {
	void			*rwg_private;
	struct mrwg_operations	*rwg_op;
};

struct allocator_interface;
struct fpn_interface;
struct ini_interface;
struct mrwg_setup {
	struct ini_interface		**ini;
	struct allocator_interface	*allocator;
	struct fpn_interface 		*fpn;
};

extern int mrwg_initialization(struct mrwg_interface *, struct mrwg_setup *);
#endif
