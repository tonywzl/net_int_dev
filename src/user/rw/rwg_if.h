/*
 * rwg_if.h
 *	Interface of Read Write Guardian Module
 */

#ifndef NID_RWG_IF_H
#define NID_RWG_IF_H

struct rwg_interface;
struct rw_interface;
struct rwg_operations {
	struct mrwg_interface* (*rwg_get_mrwg)(struct rwg_interface *);
	struct drwg_interface* (*rwg_get_drwg)(struct rwg_interface *);
	struct rw_interface* (*rwg_search)(struct rwg_interface *, char *);
	struct rw_interface* (*rwg_search_and_create)(struct rwg_interface *, char *);
};

struct rwg_interface {
	void			*rwg_private;
	struct rwg_operations	*rwg_op;
};

struct ini_interface;
struct allocator_interface;
struct fpn_interface;
struct rwg_setup {
	int 				support_mrw;
	int 				support_drw;
	struct ini_interface		**ini;
	struct allocator_interface	*allocator;
	struct fpn_interface 		*fpn;
};

extern int rwg_initialization(struct rwg_interface *, struct rwg_setup *);
#endif /* SRC_USER_RW_RWG_IF_H_ */
