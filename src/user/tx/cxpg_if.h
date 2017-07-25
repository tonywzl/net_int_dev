/*
 * cxpg_if.h
 * 	Interface of Control Exchange Passive Guardian Module
 */

#ifndef NID_CXPG_IF_H
#define NID_CXPG_IF_H

struct cxpg_interface;
struct cxp_interface;
struct cxpg_operations {
	struct cxp_interface*	(*cxg_create_channel)(struct cxpg_interface *cxpg_p, int csfdi, char *);
	int			(*cxg_add_channel_setup)(struct cxpg_interface *, char *, char *);
	int			(*cxg_drop_channel)(struct cxpg_interface *cxpg_p, struct cxp_interface *);
	int			(*cxg_drop_channel_by_uuid)(struct cxpg_interface *, char *);
	struct cxp_interface*	(*cxg_get_cxp_by_uuid)(struct cxpg_interface *, char *);
};

struct cxpg_interface {
	void			*cxg_private;
	struct cxpg_operations	*cxg_op;
};

struct lstn_interface;
struct ini_interface;
struct cxpg_setup {
	struct lstn_interface		*lstn;
	struct ini_interface		*ini;
	struct cxtg_interface           *cxtg;
};

extern int cxpg_initialization(struct cxpg_interface *, struct cxpg_setup *);
#endif
