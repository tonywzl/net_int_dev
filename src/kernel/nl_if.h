/*
 * nl_if.h
 * 	Interface of Net Link Module
 */

#ifndef _NL_IF_H
#define _NL_IF_H

#define NL_NID_PROTOCOL	22

struct sk_buff;

struct nl_request {
	struct sk_buff	*n_skb;
	char		*n_data;
	int		n_len;
};

struct nl_interface;
struct nl_operations {
	void	(*n_set_portid)(struct nl_interface *, u32);
	int 	(*n_send)(struct nl_interface *, char *, int);
	void	(*n_exit)(struct nl_interface *);
};

struct nl_interface {
	void			*n_private;
	struct nl_operations	*n_op;
};

struct mq_interface;
struct nl_setup {
	void			(*receive)(struct sk_buff *skb);
};

extern int nl_initialization(struct nl_interface *, struct nl_setup *);
#endif
