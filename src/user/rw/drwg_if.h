/*
 * drwg_if.h
 * 	Interface of Device Read Write Guardian Module
 */
#ifndef NID_DRWG_IF_H
#define NID_DRWG_IF_H

#define DRWG_MAX_DRW	128

struct drwg_interface;
struct drwg_operations {
	struct rw_interface*	(*rwg_search_drw)(struct drwg_interface *, char *);
	struct rw_interface*	(*rwg_search_and_create_drw)(struct drwg_interface *, char *);
	int			(*rwg_add_drw)(struct drwg_interface *, char *, char *, int);
	int			(*rwg_delete_drw)(struct drwg_interface *, char *);
	int			(*rwg_do_device_ready)(struct drwg_interface *, char *);
	struct drw_setup*	(*rwg_get_all_drw_setup)(struct drwg_interface *, int *);
	int*			(*rwg_get_working_drw_index)(struct drwg_interface *, int *);
};

struct drwg_interface {
	void			*rwg_private;
	struct drwg_operations	*rwg_op;
};

struct allocator_interface;
struct fpn_interface;
struct ini_interface;
struct drwg_setup {
	struct ini_interface		**ini;
	struct allocator_interface	*allocator;
	struct fpn_interface 		*fpn;
};

extern int drwg_initialization(struct drwg_interface *, struct drwg_setup *);
#endif
