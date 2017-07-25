#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>

#include "nid_log.h" 
#include "nl_if.h"


struct nl_setup nl_setup;
struct nl_interface ut_nl;

static void
nl_receive(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	int rlen = 0;
	char *data_p, buf[128];

	while (skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(skb);
		if (nlh->nlmsg_len < sizeof(*nlh) || skb->len < nlh->nlmsg_len) {
			return;
		}
		rlen = NLMSG_ALIGN(nlh->nlmsg_len);
		if (rlen > skb->len) {
			rlen = skb->len;
		}

		data_p = NLMSG_DATA(nlh);
		memcpy(buf, data_p, nlh->nlmsg_len - NLMSG_HDRLEN);
		buf[nlh->nlmsg_len - NLMSG_HDRLEN] = 0;
		nid_log_debug("recv:(pid:%d){%s}", nlh->nlmsg_pid, buf);
		skb_pull(skb, rlen);
		//skb_get(skb);
	}
}

static int
ut_mod_init(void)
{
	nid_log_info("this is mn ut module");
	nl_setup.receive = nl_receive;
	nl_initialization(&ut_nl, &nl_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("mn ut module exit");
	ut_nl.n_op->n_exit(&ut_nl);
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
