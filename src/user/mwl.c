/*
 * mwl.c
 * 	Implementation of  Message Waiting List Module
 */

#include <stdlib.h>
#include <pthread.h>

#include "nid_log.h"
#include "list.h"
#include "mwl_if.h"
#include "pp2_if.h"

struct mwl_private {
	int			p_size;
	struct mwl_message_node	*p_mnodes;
	struct list_head	p_free_head;
	struct list_head	p_used_head;
	struct pp2_interface	*p_pp2;
	pthread_mutex_t		p_lck;
	pthread_cond_t		p_cond;
};

static struct mwl_message_node*
mn_get_free_mnode(struct mwl_interface *mwl_p)
{
	struct mwl_private *priv_p = (struct mwl_private *)mwl_p->m_private;
	struct mwl_message_node *mn_p = NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	if (!list_empty(&priv_p->p_free_head)) {
		mn_p = list_first_entry(&priv_p->p_free_head, struct mwl_message_node, m_list);
		list_del(&mn_p->m_list);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return mn_p;
}

static void
mn_put_free_mnode(struct mwl_interface *mwl_p, struct mwl_message_node *mn_p)
{
	struct mwl_private *priv_p = (struct mwl_private *)mwl_p->m_private;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add(&mn_p->m_list, &priv_p->p_free_head);
	pthread_mutex_unlock(&priv_p->p_lck);
}


static void 
mwl_insert(struct mwl_interface *mwl_p, struct mwl_message_node *mn_p)
{
	struct mwl_private *priv_p = (struct mwl_private *)mwl_p->m_private;
	struct mwl_message_node *the_mn;
	int got_it = 0;

	pthread_mutex_lock(&priv_p->p_lck);
retry:
	got_it = 0;
	list_for_each_entry(the_mn, struct mwl_message_node, &priv_p->p_used_head, m_list) {
		if (mn_p->m_id == the_mn->m_id &&
		    mn_p->m_type == the_mn->m_type &&
		    mn_p->m_req == the_mn->m_req) {
			got_it = 1;
			break;
		}
	}
	if (!got_it) {
		list_add_tail(&mn_p->m_list, &priv_p->p_used_head);
	} else {
		pthread_cond_wait(&priv_p->p_cond, &priv_p->p_lck);
		goto retry;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static struct mwl_message_node*
mwl_remove(struct mwl_interface *mwl_p, int type, int id, uint8_t req)
{
	struct mwl_private *priv_p = (struct mwl_private *)mwl_p->m_private;
	struct mwl_message_node *mn_p;
	int got_it = 0;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(mn_p, struct mwl_message_node, &priv_p->p_used_head, m_list) {
		if (mn_p->m_type == type && mn_p->m_id == id && mn_p->m_req == req) {
			got_it = 1;
			break;
		}
	}
	if (got_it) {
		list_del(&mn_p->m_list);
		pthread_cond_signal(&priv_p->p_cond);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (!got_it)
		mn_p = NULL;
	return mn_p;
}

static struct mwl_message_node*
mwl_remove_next(struct mwl_interface *mwl_p, int type, int id)
{
	struct mwl_private *priv_p = (struct mwl_private *)mwl_p->m_private;
	struct mwl_message_node *mn_p;
	int got_it = 0;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(mn_p, struct mwl_message_node, &priv_p->p_used_head, m_list) {
		if (mn_p->m_type == type && mn_p->m_id == id) {
			got_it = 1;
			break;
		}
	}
	if (got_it) {
		list_del(&mn_p->m_list);
		pthread_cond_signal(&priv_p->p_cond);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (!got_it)
		mn_p = NULL;
	return mn_p;
}

static void
mwl_cleanup(struct mwl_interface *mwl_p)
{
	struct mwl_private *priv_p = (struct mwl_private *)mwl_p->m_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;

	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_mnodes);
        priv_p->p_mnodes = NULL;                                                                                      
        pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);                                                               
        mwl_p->m_private = NULL; 
}

struct mwl_operations mwl_op = {
	.m_get_free_mnode = mn_get_free_mnode,
	.m_put_free_mnode = mn_put_free_mnode,	
	.m_insert = mwl_insert,
	.m_remove = mwl_remove,
	.m_remove_next = mwl_remove_next,
	.m_cleanup = mwl_cleanup,
};

int
mwl_initialization(struct mwl_interface *mwl_p, struct mwl_setup *setup)
{
	struct mwl_private *priv_p;
	struct mwl_message_node *mn_p;
	struct pp2_interface *pp2_p = setup->pp2;
	int i;

	nid_log_info("mwl_initialization start ...");
	priv_p = (struct mwl_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	mwl_p->m_private = priv_p;
	mwl_p->m_op = &mwl_op;

	priv_p->p_pp2 = pp2_p;
	pthread_mutex_init(&priv_p->p_lck, NULL);
	pthread_cond_init(&priv_p->p_cond, NULL);
	priv_p->p_size = setup->size;
	priv_p->p_mnodes = (struct mwl_message_node *)pp2_p->pp2_op->pp2_get(pp2_p, priv_p->p_size * sizeof(*priv_p->p_mnodes));
	INIT_LIST_HEAD(&priv_p->p_free_head);
	INIT_LIST_HEAD(&priv_p->p_used_head);
	mn_p = priv_p->p_mnodes;
	for (i = 0; i < priv_p->p_size; i++, mn_p++)
		list_add_tail(&mn_p->m_list, &priv_p->p_free_head);
	return 0;
}
