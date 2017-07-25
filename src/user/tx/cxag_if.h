/*
 * cxag_if.h
 * 	Interface of Control Exchange Active Guardian Module
 */

#ifndef NID_CXAG_IF_H
#define NID_CXAG_IF_H


struct cxag_interface;
struct cxag_operations {
	struct cxa_interface*	(*cxg_search_cxa)(struct cxag_interface *, char *);
	struct cxa_interface*	(*cxg_create_channel)(struct cxag_interface *, char *);
	int			(*cxg_add_channel_setup)(struct cxag_interface *, char *, char *, char *);
	int			(*cxg_drop_channel)(struct cxag_interface *, struct cxa_interface *);
	int			(*cxg_callback)(struct cxag_interface *, struct cxa_interface *, int);
	struct cxa_setup*	(*cxg_get_all_cxa_setup)(struct cxag_interface *, int *);
	char **			(*cxg_get_working_cxa_name)(struct cxag_interface *, int *);
};

struct cxag_interface {
	void			*cxg_private;
	struct cxag_operations	*cxg_op;
};

struct lstn_interface;
struct ini_interface;
struct umpk_interface;
struct pp2_interface;
struct cxag_setup {
	struct lstn_interface		*lstn;
	struct ini_interface		*ini;
	struct cxtg_interface		*cxtg;
	struct umpk_interface		*umpk;
	struct pp2_interface		*pp2;
};

extern int cxag_initialization(struct cxag_interface *, struct cxag_setup *);
#endif
