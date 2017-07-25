/*
 * dxtg_if.h
 * 	Interface of Control Exchange Transfer Guardian Module
 */
#ifndef NID_dxtg_IF_H
#define NID_dxtg_IF_H

struct dxtg_interface;
struct dxtg_operations {
	struct dxt_interface*	(*dxg_search_and_create_channel)(struct dxtg_interface *, char *, int, int);
	struct dxt_interface*	(*dxg_search_dxt)(struct dxtg_interface *, char *);
	int	(*dxg_drop_channel)(struct dxtg_interface *, struct dxt_interface *);
	void	(*dxg_cleanup)(struct dxtg_interface *);
};

struct dxtg_interface {
	void			*dxg_private;
	struct dxtg_operations	*dxg_op;
};

struct ini_interface;
struct dxtg_setup {
	struct ini_interface		*ini;
	struct sdsg_interface 		*sdsg_p;
	struct cdsg_interface 		*cdsg_p;
	struct lstn_interface		*lstn;
	struct pp2_interface		*pp2;
	struct txn_interface		*txn;
};

extern int dxtg_initialization(struct dxtg_interface *, struct dxtg_setup *);
#endif
