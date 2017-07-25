/*
 * mq.c
 * 	Implementation of Message Queue Module 
 */

#include <pthread.h>

#include "list.h"
#include "nid_log.h"
#include "mq_if.h"
#include "pp2_if.h"

struct mq_private {
	int			p_size;
	int			p_len;
	struct mq_message_node	*p_mnodes;
	struct list_head	p_free_head;
	struct list_head	p_used_head;
	struct pp2_interface	*p_pp2;
	pthread_mutex_t		p_lck;
};

static struct mq_message_node*
mq_get_free_mnode(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	struct mq_message_node *mn_p = NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	if (!list_empty(&priv_p->p_free_head)) {
		mn_p = list_first_entry(&priv_p->p_free_head, struct mq_message_node, m_list);
		list_del(&mn_p->m_list);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return mn_p;
}

static void
mq_put_free_mnode(struct mq_interface *mq_p, struct mq_message_node *mn_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add(&mn_p->m_list, &priv_p->p_free_head);
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
mq_enqueue(struct mq_interface *mq_p, struct mq_message_node *mn_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&mn_p->m_list, &priv_p->p_used_head);
	priv_p->p_len++;
	pthread_mutex_unlock(&priv_p->p_lck);
}

static struct mq_message_node*
mq_dequeue(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	struct mq_message_node *mn_p = NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	if (!list_empty(&priv_p->p_used_head)) {
		mn_p = list_first_entry(&priv_p->p_used_head, struct mq_message_node, m_list);
		list_del(&mn_p->m_list);
		priv_p->p_len--;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return mn_p;
}

static int
mq_empty(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	int is_empty;
	pthread_mutex_lock(&priv_p->p_lck);
	is_empty = list_empty(&priv_p->p_used_head);
	pthread_mutex_unlock(&priv_p->p_lck);
	return is_empty;
}

static int
mq_len(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	return priv_p->p_len;
}

static void
mq_cleanup(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	nid_log_info("mq_initialization exit ...");
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_mnodes);
	priv_p->p_mnodes = NULL;
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
	mq_p->m_private = NULL;
}

static struct mq_operations mq_op = {
	.m_get_free_mnode = mq_get_free_mnode,
	.m_put_free_mnode = mq_put_free_mnode,
	.m_enqueue = mq_enqueue,
	.m_dequeue = mq_dequeue,
	.m_empty = mq_empty,
	.m_len = mq_len,
	.m_cleanup = mq_cleanup,
};

int
mq_initialization(struct mq_interface *mq_p, struct mq_setup *setup)
{
	struct mq_private *priv_p;
	struct mq_message_node *mn_p;
	struct pp2_interface *pp2_p = setup->pp2;
	int i;

	nid_log_info("mq_initialization start ...");
	priv_p = (struct mq_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	mq_p->m_private = priv_p;
	mq_p->m_op = &mq_op;

	priv_p->p_pp2 = pp2_p;
	priv_p->p_size = setup->size;
	priv_p->p_mnodes = pp2_p->pp2_op->pp2_get(pp2_p, priv_p->p_size * sizeof(*priv_p->p_mnodes));
	INIT_LIST_HEAD(&priv_p->p_free_head);
	INIT_LIST_HEAD(&priv_p->p_used_head);
	mn_p = priv_p->p_mnodes;
	for (i = 0; i < priv_p->p_size; i++, mn_p++)
		list_add_tail(&mn_p->m_list, &priv_p->p_free_head);

	pthread_mutex_init(&priv_p->p_lck, NULL);
	return 0;
}
