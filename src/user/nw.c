/*
 * nw.c
 * 	Implementation of Network Module
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "tp_if.h"
#include "tpg_if.h"
#include "nw.h"
#include "nw_if.h"
#include "pp2_if.h"

#define NW_ACCEPT_TIMEOUT_S	10
#define NW_ACCEPT_TIMEOUT_U	(1000*100)	// 100 million secs, i.e. 0.1 sec

struct nw_private {
	pthread_mutex_t		p_io_lck;
	struct list_head	p_io_select_head;
	struct list_head	p_io_head;
	struct tp_interface	*p_tp;
	void			*p_cg;	// channel guardian
	struct nw_operations	*p_op;
	struct pp2_interface	*p_pp2;
	struct pp2_interface	*p_dyn_pp2;
	char			p_ipstr[16];
	u_short			p_port;
	char			p_type;
};

static void
do_connection(struct tp_jobheader *jh_p)
{
	struct nw_job *job = (struct nw_job *)jh_p;
	struct nw_interface *nw_p = job->j_nw;
	struct nw_private *priv_p = (struct nw_private *)nw_p->n_private;

	nid_log_debug("a connection ready for i/o, j_res:%d, j_new:%d", job->j_res, job->j_new?1:0);
	if (job->j_new) {
		nid_log_debug("do_connection: calling module_accept_new_channel");
		job->j_data = module_accept_new_channel(priv_p->p_cg, job->j_sfd, job->j_cip);
		job->j_new = 0;
		if (!job->j_data) {
			/* Let the application close the fd inside module_accept_new_channel() */
//			close(job->j_sfd);
			return;
		}
	}
	nid_log_debug("do_connection: calling module_do_channel");
	module_do_channel(priv_p->p_cg, job->j_data);
}

static void
free_nw_job(struct tp_jobheader *jh_p)
{
	struct nw_job *job_p = (struct nw_job *)jh_p;
	struct nw_private *priv_p = (struct nw_private *)job_p->j_nw->n_private;
	struct pp2_interface *dyn_pp2_p = priv_p->p_dyn_pp2;

	dyn_pp2_p->pp2_op->pp2_put(dyn_pp2_p, jh_p);
}

static void*
_nw_job_select_thread(void *arg_nw_p)
{
	struct nw_job *job, *job1;
	struct nw_interface *nw_p = (struct nw_interface *)arg_nw_p;
	struct nw_private *priv_p = (struct nw_private *)nw_p->n_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	int maxfd;
	fd_set readfds;
	struct timeval tval;
	int rc;

	nid_log_info("nw_job_select start...");

next_try:
	FD_ZERO(&readfds);
	maxfd = -1;
	tval.tv_sec = NW_ACCEPT_TIMEOUT_S;
	tval.tv_usec = NW_ACCEPT_TIMEOUT_U;
	pthread_mutex_lock(&priv_p->p_io_lck);
	list_for_each_entry(job, struct nw_job, &priv_p->p_io_select_head, j_list) {
		FD_SET(job->j_sfd, &readfds);
		if (job->j_sfd > maxfd) {
			maxfd = job->j_sfd;
		}
	}
	pthread_mutex_unlock(&priv_p->p_io_lck);

	rc = select(maxfd + 1, &readfds, NULL, NULL, &tval);
	if (rc < 0) {
		nid_log_error("nw_job_select: select error %d", errno);
		goto out;
	}
	if (rc > 0) {
		pthread_mutex_lock(&priv_p->p_io_lck);
		list_for_each_entry(job, struct nw_job, &priv_p->p_io_select_head, j_list) {
			if (FD_ISSET(job->j_sfd, &readfds)) {
				job1 = list_entry(job->j_list.prev, struct nw_job, j_list);
				list_del(&job->j_list);
				tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job);
				job = job1;
			}
		}
		pthread_mutex_unlock(&priv_p->p_io_lck);
	}
	goto next_try;

out:
	return 0;
}

static void*
_nw_job_accept_thread(void *arg_nw_p)
{
	struct nw_interface *nw_p = (struct nw_interface *)arg_nw_p;
	struct nw_private *priv_p = (struct nw_private *)nw_p->n_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct pp2_interface *dyn_pp2_p = priv_p->p_dyn_pp2;
	u_short port = priv_p->p_port;
	int sfd, newsfd;
	int opt = 1;
	socklen_t len = sizeof(opt);
	struct sockaddr_in saddr, caddr;
	socklen_t caddrlen;
	char cip[20];
	int bind_counter = 0;
	struct timeval timeout;

	nid_log_info("nw_job_accept start (port:%d)...", port);

	sfd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	if (sfd < 0) {
		nid_log_error("nw_server: can not make server socket, errno:%d\n", errno);
		exit(101);
	}

	timeout.tv_sec = SOCKET_WRITE_TIME_OUT;
	timeout.tv_usec = 0;
	if (setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		nid_log_error("set SO_SNDTIMEO failed, errno:%d", errno);
	}

	/* reuse the port immediately from crash */
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	if (priv_p->p_ipstr[0] == 0) {
		saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		saddr.sin_addr.s_addr = inet_addr(priv_p->p_ipstr);
	}
	saddr.sin_port = htons(port);
	nid_log_debug("nw_server: binding port:%d, port:%d",
		saddr.sin_port, port);
	while ((bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) && (errno == EADDRINUSE)) {
		nid_log_error("nw_server: can not bind to port:%d, errno:%d",
			port, errno);
		if (++bind_counter > 5)
			exit(102);
	}

	if (listen(sfd, 10) < 0 ) {
		nid_log_error("nw_server: can not listen");
		exit(103);
	}

	while (1) {
		struct nw_job *new_job;
		static int counter = 0;
		caddrlen = sizeof(caddr);
		newsfd = accept(sfd, (struct sockaddr *)&caddr, &caddrlen);
		if (newsfd == -1) {
			nid_log_warning("Accept not ready: %s\n", strerror(errno));
			continue;
		}
		if (fcntl(newsfd, F_SETFD, fcntl(newsfd, F_GETFL)|FD_CLOEXEC) < 0) {
			nid_log_warning("fcntl(FD_CLOEXEC) failed while accept connection, errno: %d\n", errno);
		}
		strcpy(cip, inet_ntoa(caddr.sin_addr));
		nid_log_info("got connection from %s", cip);
		new_job = (struct nw_job *)dyn_pp2_p->pp2_op->pp2_get(dyn_pp2_p, sizeof(*new_job));
		new_job->j_new = 1;
		new_job->j_header.jh_do = do_connection;
		new_job->j_header.jh_free = free_nw_job;
		new_job->j_nw = nw_p;
		new_job->j_sfd = newsfd;
		strcpy(new_job->j_cip, inet_ntoa(caddr.sin_addr));
		new_job->j_res = counter++;
		tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)new_job);
	}
}

static void
nw_insert_waiting_io(struct nw_interface *nw_p, struct list_head *list)
{
	struct nw_private *priv_p = (struct nw_private *)nw_p->n_private;
	pthread_mutex_lock(&priv_p->p_io_lck);
	list_add_tail(list, &priv_p->p_io_select_head);
	pthread_mutex_unlock(&priv_p->p_io_lck);
}

static void
nw_cleanup(struct nw_interface *nw_p)
{
	struct nw_private *priv_p = (struct nw_private *)nw_p->n_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;

	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
	nw_p->n_private = NULL;
}

struct nw_operations nw_op = {
	.n_insert_waiting_io = nw_insert_waiting_io,
	.n_cleanup = nw_cleanup,
};

/*
 * setup:
 * 	tp: must be a inited threadpool with workers at least 3
 * 	port: must be a valid port number, network does not have any pre-defined default port
 * 	ipstr: an ip address string like "100.100.100.100" or null string ""
 */
int
nw_initialization(struct nw_interface *nw_p, struct nw_setup *setup)
{
	struct nw_private *priv_p;
	struct pp2_interface *pp2_p = setup->pp2;
	struct tpg_interface *tpg_p;
	pthread_attr_t attr;
	pthread_t thread_id;

	nid_log_info("nw_initialization start ...");
	if (!setup) {
		nid_log_info("nw_initialization has no setup");
		return -1;
	}

	memset(nw_p, 0, sizeof(*nw_p));

	priv_p = (struct nw_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	nw_p->n_private = priv_p;
	INIT_LIST_HEAD(&priv_p->p_io_select_head);
	INIT_LIST_HEAD(&priv_p->p_io_head);
	pthread_mutex_init(&priv_p->p_io_lck, NULL);
	priv_p->p_cg = setup->cg;
	priv_p->p_type = setup->type;
	priv_p->p_port = setup->port;
	priv_p->p_pp2 = pp2_p;
	priv_p->p_dyn_pp2 = setup->dyn_pp2;

	tpg_p = setup->tpg;
	priv_p->p_tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, NID_NW_TP);

	if (setup->ipstr) {
		strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	} else {
		priv_p->p_ipstr[0] = 0;
	}
	nw_p->n_op = &nw_op;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _nw_job_accept_thread, nw_p);
	pthread_create(&thread_id, &attr, _nw_job_select_thread, nw_p);
	return 0;
}
