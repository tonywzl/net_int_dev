/*
 * cdsg_if.h
 * 	Interface of Cycle Data Stream Guardian Module
 */
#ifndef NID_CDSG_IF_H
#define NID_CDSG_IF_H

#define CDSG_MAX_CDS	16

struct cdsg_interface;
struct cdsg_operations {
	struct ds_interface*	(*dg_search_cds)(struct cdsg_interface *, char *);
	struct ds_interface*	(*dg_search_and_create_cds)(struct cdsg_interface *, char *);
	int			(*dg_add_cds)(struct cdsg_interface *, char *, uint8_t, uint8_t);
};

struct cdsg_interface {
	void			*dg_private;
	struct cdsg_operations	*dg_op;
};

struct ini_interface;
struct allocator_interface;
struct cdsg_setup {
	struct allocator_interface	*allocator;
	struct ini_interface		*ini;
};

extern int cdsg_initialization(struct cdsg_interface *, struct cdsg_setup *);
#endif
