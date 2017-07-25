/*
 * dxpg_if.h
 * 	Interface of Data Exchange Passive Guardian Module
 */

#ifndef NID_DXPG_IF_H
#define NID_DXPG_IF_H

struct dxpg_interface;
struct dxp_interface;
struct dxpg_operations {
	struct dxp_interface*	(*dxg_create_channel)(struct dxpg_interface *dxpg_p, int csfdi, char *);
	int			(*dxg_drop_channel)(struct dxpg_interface *dxpg_p, struct dxp_interface *);
	int			(*dxg_drop_channel_by_uuid)(struct dxpg_interface *, char *);
	int			(*dxg_add_channel_setup)(struct dxpg_interface *, char *, char *);
	struct dxp_interface*	(*dxg_get_dxp_by_uuid)(struct dxpg_interface *, char *);
};

struct dxpg_interface {
	void			*dxg_private;
	struct dxpg_operations	*dxg_op;
};

struct lstn_interface;
struct ini_interface;
struct dxtg_interface;
struct dxpg_setup {
	struct umpk_interface		*umpk;
	struct lstn_interface		*lstn;
	struct ini_interface		*ini;
	struct dxtg_interface 		*dxtg;
};

extern int dxpg_initialization(struct dxpg_interface *, struct dxpg_setup *);
#endif
