/*
 * mq.c
 * 	Implementation of Message Queue Module 
 */
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include "nid_log.h"
#include "mq_if.h"

struct mq_private {
	int	p_size;
	struct mq_message_node	*p_mnodes;
	struct list_head	p_free_head;
	struct list_head	p_used_head;
	spinlock_t		p_lck;
};

static struct mq_message_node*
mn_get_free_mnode(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	struct mq_message_node *mn_p = NULL;

	spin_lock(&priv_p->p_lck);
	if (!list_empty(&priv_p->p_free_head)) {
		mn_p = list_first_entry(&priv_p->p_free_head, struct mq_message_node, m_list);
		list_del(&mn_p->m_list);
	}
	spin_unlock(&priv_p->p_lck);
	return mn_p;
}

static void
mn_put_free_mnode(struct mq_interface *mq_p, struct mq_message_node *mn_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	spin_lock(&priv_p->p_lck);
	list_add(&mn_p->m_list, &priv_p->p_free_head);
	spin_unlock(&priv_p->p_lck);
}

static void
mn_enqueue(struct mq_interface *mq_p, struct mq_message_node *mn_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	spin_lock(&priv_p->p_lck);
	list_add_tail(&mn_p->m_list, &priv_p->p_used_head);
	spin_unlock(&priv_p->p_lck);
}

static struct mq_message_node*
mn_dequeue(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	struct mq_message_node *mn_p = NULL;
	spin_lock(&priv_p->p_lck);
	if (!list_empty(&priv_p->p_used_head)) {
		mn_p = list_first_entry(&priv_p->p_used_head, struct mq_message_node, m_list);
		list_del(&mn_p->m_list);
	}
	spin_unlock(&priv_p->p_lck);
	return mn_p;
}

static int
mq_empty(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	int is_empty;
	spin_lock(&priv_p->p_lck);
	is_empty = list_empty(&priv_p->p_used_head);
	spin_unlock(&priv_p->p_lck);
	return is_empty;
}

static void
mq_exit(struct mq_interface *mq_p)
{
	struct mq_private *priv_p = (struct mq_private *)mq_p->m_private;
	nid_log_info("mq_initialization exit ...");
	kfree(priv_p->p_mnodes);
	priv_p->p_mnodes = NULL;
}

static struct mq_operations mq_op = {
	.m_get_free_mnode = mn_get_free_mnode,
	.m_put_free_mnode = mn_put_free_mnode,
	.m_enqueue = mn_enqueue,
	.m_dequeue = mn_dequeue,
	.m_empty = mq_empty,
	.m_exit = mq_exit,
};

int
mq_initialization(struct mq_interface *mq_p, struct mq_setup *setup)
{
	struct mq_private *priv_p;
	struct mq_message_node *mn_p;
	int i;

	nid_log_info("mq_initialization start ...");
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
	mq_p->m_private = priv_p;
	mq_p->m_op = &mq_op;

	priv_p->p_size = setup->size;
	priv_p->p_mnodes = kcalloc(priv_p->p_size, sizeof(*priv_p->p_mnodes), GFP_KERNEL);
	INIT_LIST_HEAD(&priv_p->p_free_head);
	INIT_LIST_HEAD(&priv_p->p_used_head);
	mn_p = priv_p->p_mnodes;
	for (i = 0; i < priv_p->p_size; i++, mn_p++)
		list_add_tail(&mn_p->m_list, &priv_p->p_free_head);

	spin_lock_init(&priv_p->p_lck);

	return 0;
}
