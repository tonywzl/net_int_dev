/* 
 * dat.c
 * 	Implementation of Driver Agent Transfer Module
 */

#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/netlink.h>

#include "nid_log.h"
#include "nl_if.h"
#include "mq_if.h"
#include "dat_if.h"

struct dat_private {
	struct task_struct	*p_send_thread;
	struct nl_interface	*p_nl;
	struct mq_interface	*p_imq;
	struct mq_interface	*p_omq;
	wait_queue_head_t	p_send_wq;
	char			p_send_busy:1;
	char			p_send_exist:1;
	char			p_stop:1;
};

static int
_dat_send_thread(void *data)
{
	struct dat_interface *dat_p = (struct dat_interface *)data;
	struct dat_private *priv_p = dat_p->d_private;
	struct nl_interface *nl_p = priv_p->p_nl;
	struct mq_interface *mq_p = priv_p->p_omq;
	struct mq_message_node *mn_p;

next_mnode:
	if (kthread_should_stop() || priv_p->p_stop)
		goto out;

	priv_p->p_send_busy = 0;
	while (!(mn_p = mq_p->m_op->m_dequeue(mq_p))) {
		wait_event_interruptible_timeout(priv_p->p_send_wq, priv_p->p_send_busy, 1*HZ);
		if (kthread_should_stop() || priv_p->p_stop)
			goto out;
	}

	nl_p->n_op->n_send(nl_p, mn_p->m_data, mn_p->m_len);

	if (mn_p->m_free) {
		mn_p->m_free(mn_p->m_container);
		mn_p->m_container = NULL;
	}
	mq_p->m_op->m_put_free_mnode(mq_p, mn_p);
	goto next_mnode;

out:
	priv_p->p_send_exist = 1;
	return 0; 
}

static void
dat_receive(struct dat_interface *dat_p, struct sk_buff *skb)
{
	char *log_header = "dat_receive";
	struct dat_private *priv_p = dat_p->d_private;
	struct nl_interface *nl_p = priv_p->p_nl;
	struct mq_interface *mq_p = priv_p->p_imq;
	struct mq_message_node *mn_p;
	struct nlmsghdr *nlh;
	int rlen = 0;
	char *p;
	static int dat_counter = 0;

	dat_counter++;
	nid_log_debug("dat_receive: receving, dat_counter:%d ...", dat_counter);
	while (skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(skb);
		if (nlh->nlmsg_len < sizeof(*nlh) || skb->len < nlh->nlmsg_len) {
			nid_log_error("%s: invalid msg, dat_counter:%d", log_header, dat_counter);
			return;
		}
		while (!(mn_p = mq_p->m_op->m_get_free_mnode(mq_p))) {
			nid_log_warning("%s: out of mnode", log_header);
			msleep(100);
		}

		rlen = NLMSG_ALIGN(nlh->nlmsg_len);
		if (rlen > skb->len) {
			rlen = skb->len;
		}
		skb_pull(skb, rlen);
		skb_get(skb);
		mn_p->m_portid = nlh->nlmsg_pid;
		nl_p->n_op->n_set_portid(nl_p, nlh->nlmsg_pid);
		mn_p->m_container = (void *)skb;
		mn_p->m_free = (void *)kfree_skb;
		mn_p->m_data = NLMSG_DATA(nlh);
		mn_p->m_len = nlh->nlmsg_len - NLMSG_HDRLEN;
		mq_p->m_op->m_enqueue(mq_p, mn_p);
		p = (char *)mn_p->m_data;
		nid_log_debug("%s: req:%d, code:%d,len:%d dat_counter:%d",
			log_header, *p, *(p+1),  mn_p->m_len, dat_counter); 
	}
	nid_log_debug("dat_receive: done, dat_counter:%d", dat_counter); 
}

static void
dat_send(struct dat_interface *dat_p, char *msg, int len)
{
	struct dat_private *priv_p = dat_p->d_private;
	struct mq_interface *mq_p = priv_p->p_omq;
	struct mq_message_node *mn_p;

//	nid_log_debug("dat_send: data::%p", msg);
	while (!(mn_p = mq_p->m_op->m_get_free_mnode(mq_p))) {
		nid_log_warning("nl_rece: out of mnode");
		msleep(100);
	}
	mn_p->m_portid = 0;
	mn_p->m_container = (void *)msg;
	mn_p->m_free = (void *)kfree;
	mn_p->m_data = (char *)msg;
	mn_p->m_len = len;
//	nid_log_debug("dat_send: data:%p, to enqueue", msg);
	mq_p->m_op->m_enqueue(mq_p, mn_p);

	if (!priv_p->p_send_busy) {
		priv_p->p_send_busy = 1;
		wake_up_all(&priv_p->p_send_wq);
	}
//	nid_log_debug("dat_send: data:%p, done", msg);
}

static void
dat_exit(struct dat_interface *dat_p)
{
	struct dat_private *priv_p = dat_p->d_private;

	nid_log_info("dat_exit ...");
	priv_p->p_stop = 1;
	while (!priv_p->p_send_exist)
		msleep(100);
	kfree(priv_p);
}

struct dat_operations dat_op = {
	.d_receive = dat_receive,
	.d_send = dat_send,
	.d_exit = dat_exit,
};

int
dat_initialization(struct dat_interface *dat_p, struct dat_setup *setup)
{
	struct dat_private *priv_p;

	nid_log_debug("dat_initialization  start ...");
	if (!setup) {
		return -1;
	}
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
	dat_p->d_private = priv_p;
	priv_p->p_nl = setup->nl;
	priv_p->p_imq = setup->imq;
	priv_p->p_omq = setup->omq;
	init_waitqueue_head(&priv_p->p_send_wq);
	dat_p->d_op = &dat_op;

	priv_p->p_send_thread = kthread_create(_dat_send_thread, dat_p, "%s", "nid_dat_send");
	if (IS_ERR(priv_p->p_send_thread)) {
		nid_log_error("dat_initialization: cannot create _dat_send_thread");
		kfree(priv_p);
		return -1;
	}
	wake_up_process(priv_p->p_send_thread);
	return 0;
}
