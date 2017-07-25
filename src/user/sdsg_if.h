/*
 * sdsg_if.h
 * 	Interface of Split Data Stream Guardian Module
 */
#ifndef NID_SDSG_IF_H
#define NID_SDSG_IF_H

#define SDSG_MAX_SDS	128

struct sdsg_interface;
struct sdsg_operations {
	struct ds_interface*	(*dg_search_sds)(struct sdsg_interface *, char *);
	struct ds_interface*	(*dg_search_by_wc)(struct sdsg_interface *, char *);
	struct ds_interface*	(*dg_search_by_wc_and_create_sds)(struct sdsg_interface *, char *);
	struct ds_interface*	(*dg_search_and_create_sds)(struct sdsg_interface *, char *);
	int			(*dg_add_sds)(struct sdsg_interface *, char *, char *, char *, char *);
	int			(*dg_delete_sds)(struct sdsg_interface *, char *);
	int			(*dg_switch_sds_wc)(struct sdsg_interface *, char *, char *);
	struct sds_setup*	(*dg_get_all_sds_setup)(struct sdsg_interface *, int *);
	int*			(*dg_get_working_sds_index)(struct sdsg_interface *, int *);
};

struct sdsg_interface {
	void			*dg_private;
	struct sdsg_operations	*dg_op;
};

struct ini_interface;
struct allocator_interface;
struct sdsg_setup {
	struct allocator_interface	*allocator;
	struct ini_interface		*ini;
	struct ppg_interface		*ppg;
};

extern int sdsg_initialization(struct sdsg_interface *, struct sdsg_setup *);
#endif
