/*
 * snap_if.h
 * 	Interface of Snapshot Module
 */
#ifndef NID_SNAP_IF_H
#define NID_SNAP_IF_H

struct snap_interface;
struct snap_operations {
	int	(*ss_accept_new_channel)(struct snap_interface *, int);
	void	(*ss_do_channel)(struct snap_interface *);
};

struct snap_interface {
	void			*ss_private;
	struct snap_operations	*ss_op;
};

struct snap_setup {
	struct snapg_interface	*snapg;
};

extern int snap_initialization(struct snap_interface *, struct snap_setup *);
#endif
