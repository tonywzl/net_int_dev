/*
 * mq_if.h
 * 	Interface of Message Queue Moulde
 */

#ifndef _MQ_IF_H
#define _MQ_IF_H

#include <stdint.h>
#include "list.h"

struct mq_message_node {
	struct list_head	m_list;
	void *			*m_handle;
	uint32_t		m_portid;
	void			(*m_free)(void *);
	void			*m_container;
	int			m_len;
	void			*m_data;
};

struct mq_interface;
struct mq_operations {
	struct mq_message_node*	(*m_get_free_mnode)(struct mq_interface *);
	void			(*m_put_free_mnode)(struct mq_interface *, struct mq_message_node *);
	void			(*m_enqueue)(struct mq_interface *, struct mq_message_node *);
	struct mq_message_node*	(*m_dequeue)(struct mq_interface *);
	int			(*m_empty)(struct mq_interface *);
	int			(*m_len)(struct mq_interface *);
	void			(*m_cleanup)(struct mq_interface *);
};

struct mq_interface {
	void			*m_private;
	struct mq_operations	*m_op;
};

struct mq_setup {
	int	size;	// number of message nodes
	struct pp2_interface *pp2;
};

extern int mq_initialization(struct mq_interface *, struct mq_setup *);

#endif
