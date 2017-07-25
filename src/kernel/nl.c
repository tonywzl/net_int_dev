/*
 * nl.c:
 * 	Implementation of Net Link Module
 */
#include <linux/netlink.h>
#include <net/sock.h>

#include "nid_log.h"
#include "nl_if.h"

struct nl_private {
	u32			p_portid;
	struct sock		*p_sock;
};

static struct sk_buff *
_nl_alloc_skb(int data_len, char **data)
{
	struct sk_buff *nl_skb;
	struct nlmsghdr *nlh = NULL;

	nl_skb = alloc_skb(NLMSG_SPACE(data_len), GFP_NOWAIT);
	if(!nl_skb) {
		return NULL;
	}
	nlh = nlmsg_put(nl_skb, 0, 0, 0, data_len, 0);
	*data = (char *)NLMSG_DATA(nlh);
	return nl_skb;
}

static int
nl_send(struct nl_interface *nl_p, char *msg, int len)
{
	struct nl_private *priv_p = (struct nl_private *)nl_p->n_private;
	struct sock *sock = priv_p->p_sock;
        int ret;
	struct sk_buff *nl_skb;
	char *p;

	nl_skb = _nl_alloc_skb(len, &p);
	if (!nl_skb) {
		nid_log_error("nl_send: cannot alloc skb\n");
		return -1;
	}
	memcpy(p, msg, len);
//	nid_log_debug("nl_send: calling netlink_unicast (port:%u) len:%d", priv_p->p_portid, len);
	ret = netlink_unicast(sock, nl_skb, priv_p->p_portid, MSG_DONTWAIT);
//	nid_log_debug("nl_send: netlink_unicast done");
	return ret;
}

static void
nl_set_portid(struct nl_interface *nl_p, u32 portid)
{
	struct nl_private *priv_p = (struct nl_private *)nl_p->n_private;
	priv_p->p_portid = portid;	
}

static void
nl_exit(struct nl_interface *nl_p)
{
	struct nl_private *priv_p = nl_p->n_private;
	nid_log_info("nl_exit ...");
	netlink_kernel_release(priv_p->p_sock);
}

static struct nl_operations nl_op = {
	.n_set_portid = nl_set_portid,
	.n_send = nl_send,	
	.n_exit = nl_exit,	
};

int
nl_initialization(struct nl_interface *nl_p, struct nl_setup *setup)
{
	struct nl_private *priv_p;
	struct netlink_kernel_cfg nl_cfg = {.input = setup->receive};

	nid_log_info("nl_initialization start ...");
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
	nl_p->n_private = priv_p;
	nl_p->n_op = &nl_op;

	priv_p->p_sock = netlink_kernel_create(&init_net, NL_NID_PROTOCOL, &nl_cfg);
	if (!priv_p->p_sock) {
		nid_log_info("Fail to create netlink socket.");
		return -1;
	}
	return 0;
}
