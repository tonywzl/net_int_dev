/*
 * mwl_if.h
 * 	Interface of Message Waiting List Module
 */

#ifndef NID_MWL_IF_H
#define NID_MWL_IF_H

#include <stdint.h>
#include "list.h" 

struct mwl_message_node {
	struct list_head	m_list;	// message list
	int			m_id;	// device minor | channel id
	int			m_type;
	uint8_t			m_req;	// request code
	void			*m_data;
	void			(*m_free)(void *);
	void			*m_container;
};

struct mwl_interface;
struct mwl_operations {
	struct mwl_message_node*	(*m_get_free_mnode)(struct mwl_interface *);
	void				(*m_put_free_mnode)(struct mwl_interface *, struct mwl_message_node *);
	void				(*m_insert)(struct mwl_interface *, struct mwl_message_node *);
	struct mwl_message_node*	(*m_remove)(struct mwl_interface *, int, int, uint8_t);	
	struct mwl_message_node*	(*m_remove_next)(struct mwl_interface *, int, int);
	void				(*m_cleanup)(struct mwl_interface *);
};

struct mwl_interface {
	void			*m_private;
	struct mwl_operations	*m_op;
};

struct mwl_setup {
	int	size;
	struct pp2_interface *pp2;
};

extern int mwl_initialization(struct mwl_interface *, struct mwl_setup *);
#endif
