/*
 * repscg_if.h
 *	Interface of Replication Source Channel Guardian Module
 */
#ifndef NID_REPSCG_IF_H
#define NID_REPSCG_IF_H

struct reps_interface;
struct repscg_interface;
struct repscg_operations {
	void*			(*rscg_accept_new_channel)(struct repscg_interface *, int, char *);
	void			(*rscg_do_channel)(struct repscg_interface *, void *);
};

struct repscg_interface {
	void			*r_private;
	struct repscg_operations *r_op;
};

struct umpk_interface;
struct repscg_setup {
	struct repsg_interface *repsg;
	struct umpk_interface *umpk;
};

extern int repscg_initialization(struct repscg_interface *, struct repscg_setup *);
#endif
