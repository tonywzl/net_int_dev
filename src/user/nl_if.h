/*
 * nl_if.h
 * 	Interface of Net Link Module
 */

#ifndef _NL_IF_H
#define _NL_IF_H

#define NL_NID_PROTOCOL	22

struct nl_interface;
struct nlmsghdr;
struct nl_operations {
	int			(*n_send)(struct nl_interface *, char *, int);	
	struct nlmsghdr*	(*n_receive)(struct nl_interface *);	
	void			(*n_stop)(struct nl_interface *);
	void			(*n_cleanup)(struct nl_interface *);
};

struct nl_interface {
	void			*n_private;
	struct nl_operations	*n_op;
};

struct mq_interface;
struct nl_setup {
	struct mq_interface	*mq;
	struct pp2_interface	*pp2;
	struct pp2_interface	*dyn2_pp2;
};

extern int nl_initialization(struct nl_interface *, struct nl_setup *);
#endif
