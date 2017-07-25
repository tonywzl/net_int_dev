/* 
 * adt.c
 * 	Implementation of Agent Driver Transfer Module
 */

#include <stdlib.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <pthread.h>
#include <malloc.h>

#include "nid_log.h"
#include "nl_if.h"
#include "mq_if.h"
#include "adt_if.h"
#include "pp2_if.h"
#include "pp2_if.h"
#include "agent.h"

struct adt_private {
	struct nl_interface	*p_nl;
	struct mq_interface	*p_imq;
	struct mq_interface	*p_omq;
	struct pp2_interface	*p_pp2;
	struct pp2_interface	*p_dyn2_pp2;
	pthread_mutex_t		p_recv_lck;
	pthread_cond_t		*p_recv_cond;
	pthread_mutex_t		p_send_lck;
	pthread_cond_t		*p_send_cond;
	char			p_recv_busy;
	char			p_send_busy;
	uint8_t			p_stop;
	uint8_t			p_recv_stop;
};

static void
_adt_mfree(void * mn_input)
{
	struct mq_message_node *mn_p = (struct mq_message_node *)mn_input;
	struct adt_interface *adt_p = (struct adt_interface *)mn_p->m_handle;
	struct adt_private *priv_p = (struct adt_private *)adt_p->a_private;
	struct pp2_interface *dyn2_pp2_p = priv_p->p_dyn2_pp2;

	dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)mn_p->m_container);
}
static void*

_adt_recv_thread(void *data)
{
	struct adt_interface *adt_p = (struct adt_interface *)data;
	struct adt_private *priv_p = adt_p->a_private;
	struct nl_interface *nl_p = priv_p->p_nl;
	struct mq_interface *mq_p = priv_p->p_imq;
	struct nlmsghdr *nlhdr;
	struct mq_message_node *mn_p;

next_msg:
	nlhdr = nl_p->n_op->n_receive(nl_p);
	if (nlhdr != NULL) {
		while (!(mn_p = mq_p->m_op->m_get_free_mnode(mq_p))) {
			nid_log_warning("_adt_recv_thread: nl_rece: out of mnode");
			sleep(1);
		}
		mn_p->m_portid = 0;
		mn_p->m_container = (void *)nlhdr;
		mn_p->m_free = (void *)_adt_mfree;
		mn_p->m_data = (char *)NLMSG_DATA(nlhdr);
		mn_p->m_len = nlhdr->nlmsg_len - NLMSG_HDRLEN;
		mn_p->m_handle = (void *)adt_p;
		mq_p->m_op->m_enqueue(mq_p, mn_p);

		pthread_mutex_lock(&priv_p->p_recv_lck);
		if (!priv_p->p_recv_busy)
		pthread_cond_signal(priv_p->p_recv_cond);
		pthread_mutex_unlock(&priv_p->p_recv_lck);
	}
	if (!agent_is_stop() && priv_p->p_stop == 0) {
		goto next_msg;
	}
	priv_p->p_recv_stop = 1;
	return NULL;
}

static void*
_adt_send_thread(void *data)
{
	struct adt_interface *adt_p = (struct adt_interface *)data;
	struct adt_private *priv_p = adt_p->a_private;
	struct nl_interface *nl_p = priv_p->p_nl;
	struct mq_interface *mq_p = priv_p->p_omq;
	struct mq_message_node *mn_p;

	pthread_mutex_lock(&priv_p->p_send_lck);
next_msg:
	while (!(mn_p = mq_p->m_op->m_dequeue(mq_p))) {
		priv_p->p_send_busy = 0;
		pthread_cond_wait(priv_p->p_send_cond, &priv_p->p_send_lck);
		priv_p->p_send_busy = 1;
	}

	nl_p->n_op->n_send(nl_p, mn_p->m_data, mn_p->m_len);

	if (mn_p->m_free) {
		mn_p->m_free(mn_p);
		mn_p->m_container = NULL;
	}
	mq_p->m_op->m_put_free_mnode(mq_p, mn_p);

	goto next_msg;
	return NULL;
}

static struct mq_message_node* 
adt_receive(struct adt_interface *adt_p)
{
	struct adt_private *priv_p = adt_p->a_private;
	struct mq_interface *mq_p = priv_p->p_imq;
	struct mq_message_node *mn_p;

	pthread_mutex_lock(&priv_p->p_recv_lck);
	while (!(mn_p = mq_p->m_op->m_dequeue(mq_p))) {
		priv_p->p_recv_busy = 0;
		pthread_cond_wait(priv_p->p_recv_cond, &priv_p->p_recv_lck);
		priv_p->p_recv_busy = 1;
	}
	pthread_mutex_unlock(&priv_p->p_recv_lck);
	return mn_p;
}

static void
adt_collect_mnode(struct adt_interface *adt_p, struct mq_message_node *mn_p)
{
	struct adt_private *priv_p = adt_p->a_private;
	struct mq_interface *mq_p = priv_p->p_imq;
	mq_p->m_op->m_put_free_mnode(mq_p, mn_p);
}

static void
adt_send(struct adt_interface *adt_p, char *msg, int len)
{
	struct adt_private *priv_p = adt_p->a_private;
	struct mq_interface *mq_p = priv_p->p_omq;
	struct mq_message_node *mn_p;

	while (!(mn_p = mq_p->m_op->m_get_free_mnode(mq_p))) {
		nid_log_warning("nl_rece: out of mnode");
		sleep(1);
	}
	mn_p->m_portid = 0;
	mn_p->m_container = (void *)msg;
	mn_p->m_free = (void *)_adt_mfree;
	mn_p->m_data = (char *)msg;
	mn_p->m_len = len;
	mn_p->m_handle = (void *)adt_p;
	mq_p->m_op->m_enqueue(mq_p, mn_p);

	pthread_mutex_lock(&priv_p->p_send_lck);
	if (!priv_p->p_send_busy)
		pthread_cond_signal(priv_p->p_send_cond);
	pthread_mutex_unlock(&priv_p->p_send_lck);
}

static char *
adt_get_buffer(struct adt_interface *adt_p, int len)
{
	struct adt_private *priv_p = (struct adt_private *)adt_p->a_private;
	struct pp2_interface *dyn2_pp2_p = priv_p->p_dyn2_pp2;

	return (char *)dyn2_pp2_p->pp2_op->pp2_get(dyn2_pp2_p, len);
}

static void
adt_stop(struct adt_interface *adt_p)
{
	struct adt_private *priv_p = (struct adt_private *)adt_p->a_private;
        struct nl_interface *nl_p = priv_p->p_nl;

	priv_p->p_stop = 1;
        nl_p->n_op->n_stop(nl_p);

}

static void
adt_cleanup(struct adt_interface *adt_p)
{
	struct adt_private *priv_p = (struct adt_private *)adt_p->a_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct mq_interface *imq_p = priv_p->p_imq;
	struct mq_interface *omq_p = priv_p->p_omq;
	struct nl_interface *nl_p = priv_p->p_nl;

	while (priv_p->p_recv_stop == 0) {
		sleep(1);
	}

	nl_p->n_op->n_cleanup(nl_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)nl_p);

	imq_p->m_op->m_cleanup(imq_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)imq_p);

	omq_p->m_op->m_cleanup(omq_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)omq_p);

	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
	adt_p->a_private = NULL;
}

struct adt_operations adt_op = {
	.a_receive = adt_receive,
	.a_send = adt_send,
	.a_collect_mnode = adt_collect_mnode,
	.a_get_buffer = adt_get_buffer,
	.a_stop = adt_stop,
	.a_cleanup = adt_cleanup,
};

int
adt_initialization(struct adt_interface *adt_p, struct adt_setup *setup)
{
	struct adt_private *priv_p;
	struct pp2_interface *pp2_p;
	struct nl_setup nl_setup;
	struct nl_interface *nl_p;
	struct mq_setup imq_setup, omq_setup;
	struct mq_interface *imq_p, *omq_p;
	pthread_t thread_id;
	pthread_attr_t recv_attr, send_attr;

	nid_log_debug("adt_initialization  start ...");

	pp2_p = setup->pp2;

	priv_p = (struct adt_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	adt_p->a_private = priv_p;

	imq_setup.size = 1024;
	imq_setup.pp2 = pp2_p;
        imq_p = (struct mq_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*imq_p));
        mq_initialization(imq_p, &imq_setup);

	omq_setup.size = 1024;
	omq_setup.pp2 = pp2_p;
        omq_p = (struct mq_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*omq_p));
        mq_initialization(omq_p, &omq_setup);

	nl_setup.mq = imq_p;
	nl_setup.pp2 = pp2_p;
	nl_setup.dyn2_pp2 = setup->dyn2_pp2;
        nl_p = (struct nl_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*nl_p));
        nl_initialization(nl_p, &nl_setup);

	priv_p->p_imq = imq_p;
	priv_p->p_omq = omq_p;
	priv_p->p_nl = nl_p;
	priv_p->p_pp2 = pp2_p;
	priv_p->p_dyn2_pp2 = setup->dyn2_pp2;
	priv_p->p_stop = 0;
	priv_p->p_recv_stop = 0;

	priv_p->p_recv_cond = (pthread_cond_t*)memalign(4, sizeof(pthread_cond_t));
	priv_p->p_send_cond = (pthread_cond_t*)memalign(4, sizeof(pthread_cond_t));

	pthread_mutex_init(&priv_p->p_recv_lck, NULL);
	pthread_cond_init(priv_p->p_recv_cond, NULL);
	pthread_mutex_init(&priv_p->p_send_lck, NULL);
	pthread_cond_init(priv_p->p_send_cond, NULL);
	adt_p->a_op = &adt_op;

	pthread_attr_init(&send_attr);
	pthread_attr_setdetachstate(&send_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &send_attr, _adt_send_thread, adt_p);

	pthread_attr_init(&recv_attr);
	pthread_attr_setdetachstate(&recv_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &recv_attr, _adt_recv_thread, adt_p);
	return 0;
}
