/*
 * doag_if.h
 * 	Interface of Delegation of Authority Guardian Module
 */
#ifndef NID_DOAG_IF_H
#define NID_DOAG_IF_H

struct doag_interface;
struct doag_operations {
	void*	(*dg_accept_new_channel)(struct doag_interface *, int);
	void	(*dg_do_channel)(struct doag_interface *, void *);
};

struct doag_interface {
	void			*dg_private;
	struct doag_operations	*dg_op;
};

struct doag_setup {
	struct mpk_interface	*p_mpk;
};

extern int doag_initialization(struct doag_interface *, struct doag_setup *);
#endif
