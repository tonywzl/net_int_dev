/* mrp.c
 * 	Implementation of Net Link Module
 */
#include <stdio.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stddef.h>

#include "nid_log.h"
#include "list.h"
#include "mq_if.h"
#include "nl_if.h"
#include "pp2_if.h"
#include "agent.h"

#define MAX_NBUFF 256

struct nl_private {
	int			p_sfd;
	pid_t			p_pid;
	struct sockaddr_nl	p_saddr;
	struct sockaddr_nl	p_daddr;
	struct mq_interface	*p_mq;
	struct pp2_interface	*p_pp2;
	struct pp2_interface	*p_dyn2_pp2;
	uint8_t			p_stop;
	uint8_t			p_recv_stop;
};

static int
nl_send(struct nl_interface *nl_p, char *msg, int len)
{
	struct nl_private *priv_p = (struct nl_private *)nl_p->n_private;
	struct nlmsghdr nlhdr;
	struct msghdr msghdr;
	struct sockaddr_nl *daddrp = &priv_p->p_daddr;
	int ret = 0;
	struct iovec vecp[2];

	nlhdr.nlmsg_len = NLMSG_SPACE(0) + len;
	nlhdr.nlmsg_pid = getpid();
	nlhdr.nlmsg_flags = 0;
	nlhdr.nlmsg_type = 1;
	vecp[0].iov_base = (void *) &nlhdr;
	vecp[0].iov_len = NLMSG_SPACE(0);
	vecp[1].iov_base = (void *)msg;
	vecp[1].iov_len = len;

	memset(&msghdr, 0, sizeof(msghdr));
	msghdr.msg_name = (void *) daddrp;
	msghdr.msg_namelen = sizeof(*daddrp);
	msghdr.msg_iov = vecp;
	msghdr.msg_iovlen = 2;

	nid_log_debug("nl_send: sending (len:%d) ...", len);
	if (sendmsg(priv_p->p_sfd, &msghdr, 0) < 0) {
		ret = errno;
		nid_log_error("nl_send: can not send, ret:%d\n", ret);
		goto out;
	}
out:
	return ret;
}

static struct nlmsghdr* 
nl_receive(struct nl_interface *nl_p)
{
	struct nl_private *priv_p = (struct nl_private *)nl_p->n_private;
	struct sockaddr_nl *daddrp = &priv_p->p_daddr;
	struct nlmsghdr *nlhdr = NULL;
	struct msghdr msghdr;
	struct iovec iov;
	struct pp2_interface *dyn2_pp2_p = priv_p->p_dyn2_pp2;
	int len;
	ssize_t readn;
	fd_set readset;
	struct timeval tval;
	int rc;

	while (1) {
                FD_ZERO(&readset);
                tval.tv_sec = 1;
                tval.tv_usec = 0;
                FD_SET(priv_p->p_sfd, &readset);
                rc = select(priv_p->p_sfd + 1, &readset, NULL, NULL, &tval);
                if (rc < 0) {
                        goto out;
                }
		if (priv_p->p_stop == 1){
			priv_p->p_recv_stop = 1;
			goto out;
		}
                if (FD_ISSET(priv_p->p_sfd, &readset)) {
                        break;
                }
        }

	len = 1024;	// big enough for only recv request from kernel mrp
	nlhdr = (struct nlmsghdr *)dyn2_pp2_p->pp2_op->pp2_get(dyn2_pp2_p, NLMSG_SPACE(len));
	iov.iov_base = (void *) nlhdr;
	iov.iov_len = NLMSG_SPACE(len);
	msghdr.msg_name = (void *) daddrp;
	msghdr.msg_namelen = sizeof(struct sockaddr_nl);
	msghdr.msg_iov = &iov;
	msghdr.msg_iovlen = 1;
	readn = recvmsg(priv_p->p_sfd, &msghdr, MSG_WAITALL);
	if (readn <= 0) {
		nid_log_error("nl_receive: return %lu, error:%d", readn, errno);
		dyn2_pp2_p->pp2_op->pp2_put(dyn2_pp2_p, (void *)nlhdr);
		return NULL;
	}
out:
	return nlhdr;
}

static void
nl_stop(struct nl_interface *nl_p)
{
	struct nl_private *priv_p = (struct nl_private *)nl_p->n_private;

	priv_p->p_stop = 1;
}

static void
nl_cleanup(struct nl_interface *nl_p)
{
	struct nl_private *priv_p = (struct nl_private *)nl_p->n_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;

	if (priv_p->p_sfd >= 0) {
		close(priv_p->p_sfd);
		priv_p->p_sfd = -1;
	}
	while (priv_p->p_recv_stop == 0) {
		sleep(1);
	}
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
	nl_p->n_private = NULL;
}

static struct nl_operations nl_op = {
	.n_send = nl_send,
	.n_receive = nl_receive,
	.n_stop = nl_stop,
	.n_cleanup = nl_cleanup,
};

int
nl_initialization(struct nl_interface *nl_p, struct nl_setup *setup)
{
	struct nl_private *priv_p;
	struct sockaddr_nl *saddrp, *daddrp;
	struct pp2_interface *pp2_p = setup->pp2;

	nid_log_debug("nl_initialization start ...");
	assert(setup);
	priv_p = (struct nl_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	nl_p->n_private = priv_p;
	priv_p->p_pp2 = setup->pp2;
	priv_p->p_dyn2_pp2 = setup->dyn2_pp2;
	priv_p->p_mq = setup->mq;
	priv_p->p_stop = 0;
	priv_p->p_recv_stop = 0;
	priv_p->p_pid = getpid();
	priv_p->p_sfd = socket(AF_NETLINK, SOCK_RAW|SOCK_CLOEXEC, NL_NID_PROTOCOL);
	if (priv_p->p_sfd < 0) {
		nid_log_error("nl_initialization: failed to create netlink socket: (%d)%s", 
			errno, strerror(errno));
		pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
		return -1;
	}

	saddrp = &priv_p->p_saddr;
	memset(saddrp, 0, sizeof(*saddrp));
	saddrp->nl_family = AF_NETLINK;
	saddrp->nl_pid = priv_p->p_pid;
	saddrp->nl_groups = 0;
	bind(priv_p->p_sfd, (struct sockaddr *)saddrp, sizeof(*saddrp));
	daddrp = &priv_p->p_daddr;
	daddrp->nl_family = AF_NETLINK;
	daddrp->nl_pid = 0;
	daddrp->nl_groups = 0;
	nl_p->n_op = &nl_op;
	return 0;
}
