/*
 * dat_if.h
 * 	Interface of Driver Agent Transfer Moulde
 */

#ifndef _NID_DAT_IF_H
#define _NID_DAT_IF_H

struct dat_interface;
struct sk_buff;
struct dat_operations {
	void	(*d_receive)(struct dat_interface *, struct sk_buff *);
	void	(*d_send)(struct dat_interface *, char *, int);
	void	(*d_exit)(struct dat_interface *);
};

struct dat_interface {
	void			*d_private;
	struct dat_operations	*d_op;
};

struct nl_interface;
struct mq_interface;
struct dat_setup {
	struct nl_interface	*nl;
	struct mq_interface	*imq;
	struct mq_interface	*omq;
};

extern int dat_initialization(struct dat_interface *, struct dat_setup *);

#endif
