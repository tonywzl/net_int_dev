/*
 * adt_if.h
 * 	Interface of Agent Driver Transfer Moulde
 */

#ifndef _NID_ADT_IF_H
#define _NID_ADT_IF_H


struct adt_interface;
struct adt_operations {
	struct mq_message_node*	(*a_receive)(struct adt_interface *);
	void			(*a_send)(struct adt_interface *, char *, int);
	void			(*a_collect_mnode)(struct adt_interface *, struct mq_message_node *);
	char *			(*a_get_buffer)(struct adt_interface *, int );
	void			(*a_stop)(struct adt_interface *);
	void			(*a_cleanup)(struct adt_interface *);
};

struct adt_interface {
	void			*a_private;
	struct adt_operations	*a_op;
};

struct nl_interface;
struct mq_interface;
struct adt_setup {
	struct pp2_interface *pp2;
	struct pp2_interface *dyn2_pp2;
};

extern int adt_initialization(struct adt_interface *, struct adt_setup *);

#endif
