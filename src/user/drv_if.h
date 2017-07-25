/*
 * drv_if.h
 * 	Interface of Driver Module
 */
#ifndef NID_DRV_IF_H
#define NID_DRV_IF_H

struct ascg_interface;
struct drv_interface;
struct nid_message;
struct drv_operations {
	void	(*dr_set_ascg)(struct drv_interface *, struct ascg_interface *);
	void	(*dr_send_message)(struct drv_interface *, struct nid_message *, int *);
	int	(*dr_is_stop)(struct drv_interface *);
	int	(*dr_get_chan_id)(struct drv_interface *, char *);
	void	(*dr_cleanup)(struct drv_interface *);
};

struct drv_interface {
	void			*dr_private;
	struct drv_operations	*dr_op;
};

struct mpk_interface;
struct adt_interface;
struct drv_setup {
	struct ascg_interface	*ascg;
	struct mpk_interface	*mpk;
	struct umpka_interface	*umpka;
	struct pp2_interface	*pp2;
	struct pp2_interface	*dyn2_pp2;
};

extern int drv_initialization(struct drv_interface *, struct drv_setup *);
#endif
