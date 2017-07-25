/*
 * cxtg_if.h
 * 	Interface of Control Exchange Transfer Guardian Module
 */
#ifndef NID_CXTG_IF_H
#define NID_CXTG_IF_H

struct cxtg_interface;
struct cxtg_operations {
	struct cxt_interface*	(*cxg_search_and_create_channel)(struct cxtg_interface *, char *, int);
	struct cxt_interface*	(*cxg_search_cxt)(struct cxtg_interface *, char *);
	int 			(*cxg_add_channel_setup)(struct cxtg_interface *, char *, char *, int);
	int			(*cxg_drop_channel)(struct cxtg_interface *, struct cxt_interface *);
	void			(*cxg_cleanup)(struct cxtg_interface *);
};

struct cxtg_interface {
	void			*cxg_private;
	struct cxtg_operations	*cxg_op;
};

struct ini_interface;
struct sdsg_interface;
struct cdsg_interface;
struct lstn_interface;
struct txn_interface;
struct pp2_interface;
struct cxtg_setup {
	struct ini_interface		*ini;
	struct sdsg_interface 		*sdsg_p;
	struct cdsg_interface 		*cdsg_p;
	struct lstn_interface		*lstn;
	struct txn_interface		*txn;
	struct pp2_interface		*pp2;
};

extern int cxtg_initialization(struct cxtg_interface *, struct cxtg_setup *);
#endif
