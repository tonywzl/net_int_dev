/*
 * dit.c
 * 	Implementation of Data Integrity Testing Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#include "nid_log.h"
#include "ini_if.h"
#include "tp_if.h"
#include "dcn_if.h"
#include "rdg_if.h"
#include "olap_if.h"
#include "dit_if.h"

#define DIT_MAX_THREADS	256

struct dit_job {
	struct tp_jobheader	header;
	struct dit_interface	*dit;
	int			idx;
	struct dcn_node		*np;
};

struct dit_timer_job {
	struct tp_jobheader	header;
	struct dit_interface	*dit;
};

struct dit_private {
	pthread_mutex_t		p_lck;
	pthread_cond_t		p_cond;
	struct ini_interface	*p_ini;
	struct tp_interface	*p_tp;
	struct dcn_interface	*p_dcn;
	struct rdg_interface	*p_rdg;
	struct olap_interface	p_olap;
	int			p_nr;		// nuber of threads
	int			p_run_nr;	// nuber of threads at this run
	int			p_nr_done;
	struct dcn_node		*p_nodes[DIT_MAX_THREADS];	
	struct dit_job		*p_jobs[DIT_MAX_THREADS];
	struct dit_job		*p_bck_jobs[DIT_MAX_THREADS];
	char			p_devname[1024];
	char			p_bck_devname[1024];
	int			p_fd;
	int			p_bck_fd;
	int			p_time_to_run;
	time_t			p_time_start;
	time_t			p_time_stop;
	uint8_t			p_to_stop;
};

static struct dcn_node *
_get_data(struct dit_interface *dit_p)
{
	struct dit_private *priv_p = (struct dit_private *)dit_p->d_private;
	struct dcn_interface *dcn_p = priv_p->p_dcn;
	struct rdg_interface *rdg_p = priv_p->p_rdg;
	struct dcn_node *np = NULL;
	off_t start_off;
	uint32_t len, power_of_2;

	rdg_p->rd_op->rd_get_range(rdg_p, &start_off, &len);
	power_of_2 = 31 - __builtin_clz(len);
	if (len > (1U << power_of_2))
		power_of_2++;	// make sure to get a data buffer with big enough size
	np = dcn_p->dc_op->dc_get_node(dcn_p, power_of_2); 
	np->n_len = len;
	np->n_offset = start_off;
	rdg_p->rd_op->rd_get_data(rdg_p, np->n_len,  np->n_data);
	return np;
}

static void
_put_data(struct dit_interface *dit_p, struct dcn_node *np)
{
	struct dit_private *priv_p = (struct dit_private *)dit_p->d_private;
	struct dcn_interface *dcn_p = priv_p->p_dcn;
	dcn_p->dc_op->dc_put_node(dcn_p, np);
}

static void
_dit_timer_job(struct tp_jobheader *header)
{
	struct dit_timer_job *job = (struct dit_timer_job *)header;
	struct dit_interface *dit_p = job->dit;
	struct dit_private *priv_p = (struct dit_private *)dit_p->d_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct tp_jobheader *jh_p = (struct tp_jobheader *)job;
	struct timeval now;

	nid_log_debug("_dit_timer_job: start ..");
	if (priv_p->p_time_stop) {
		gettimeofday(&now, NULL);
		if (now.tv_sec >= priv_p->p_time_stop) {
			priv_p->p_to_stop = 1;
			return;
		}
	}
	nid_log_debug("_dit_timer_job: waiting more ..");
	tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 5);
}

static void
_dit_run_job(struct tp_jobheader *header)
{
	struct dit_job *job = (struct dit_job *)header;
	struct dit_interface *dit_p = job->dit;
	struct dit_private *priv_p = (struct dit_private *)dit_p->d_private;
	struct dcn_node *np = job->np;
	ssize_t nwrite;
	nid_log_notice("_dit_run_job: start (idx:%d, len:%u,  offset:%lu, data:%d) ...", job->idx, np->n_len, np->n_offset, np->n_data[0]);	
	nwrite = pwrite(priv_p->p_fd, np->n_data, np->n_len, np->n_offset);
	assert(nwrite == np->n_len);
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_nr_done++;
	if (priv_p->p_nr_done == priv_p->p_run_nr)
		pthread_cond_signal(&priv_p->p_cond);
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
_dit_run_bck_job(struct tp_jobheader *header)
{
	struct dit_job *job = (struct dit_job *)header;
	struct dit_interface *dit_p = job->dit;
	struct dit_private *priv_p = (struct dit_private *)dit_p->d_private;
	struct dcn_node *np = job->np;
	ssize_t nwrite;
	nwrite = pwrite(priv_p->p_bck_fd, np->n_data, np->n_len, np->n_offset);
	assert(nwrite == np->n_len);
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_nr_done++;
	if (priv_p->p_nr_done == priv_p->p_run_nr)
		pthread_cond_signal(&priv_p->p_cond);
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
_dit_do_run(struct dit_interface *dit_p)
{
	struct dit_private *priv_p = (struct dit_private *)dit_p->d_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct olap_interface *olap_p = &priv_p->p_olap;
	struct dcn_node *np, *np2;
	struct dit_job *job;
	struct tp_jobheader *jh_p;
	struct list_head sorted_head, olap_head;
	int i, got_it;

	INIT_LIST_HEAD(&sorted_head);
	INIT_LIST_HEAD(&olap_head);
	for (i = 0; i < priv_p->p_nr; i++) {
		np = _get_data(dit_p);
		priv_p->p_nodes[i] = np;

		list_for_each_entry(np2, struct dcn_node, &sorted_head, n_list) {
			if (np2->n_offset > np->n_offset) {
				got_it = 1;
				break;
			}
		}
		if (got_it)
			__list_add(&np->n_list, np2->n_list.prev, &np2->n_list);
		else
			__list_add(&np->n_list, sorted_head.prev, &sorted_head);
	}
	olap_p->ol_op->ol_get_overlap(olap_p, &sorted_head, &olap_head);

	priv_p->p_nr_done = 0;
	priv_p->p_run_nr = priv_p->p_nr;
	for (i = 0; i < priv_p->p_nr; i++) {
		job = priv_p->p_jobs[i];
		job->np = priv_p->p_nodes[i];
		jh_p = (struct tp_jobheader *)job;
		tp_p->t_op->t_job_insert(tp_p, jh_p);
	}
	pthread_mutex_lock(&priv_p->p_lck);
	while (priv_p->p_nr_done != priv_p->p_run_nr) {
		pthread_cond_wait(&priv_p->p_cond, &priv_p->p_lck);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	priv_p->p_nr_done = 0;
	priv_p->p_run_nr = priv_p->p_nr;
	for (i = 0; i < priv_p->p_nr; i++) {
		job = priv_p->p_bck_jobs[i];
		job->np = priv_p->p_nodes[i];
		jh_p = (struct tp_jobheader *)job;
		tp_p->t_op->t_job_insert(tp_p, jh_p);
	}
	pthread_mutex_lock(&priv_p->p_lck);
	while (priv_p->p_nr_done != priv_p->p_run_nr) {
		pthread_cond_wait(&priv_p->p_cond, &priv_p->p_lck);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	for (i = 0; i < priv_p->p_nr; i++) {
		_put_data(dit_p, priv_p->p_nodes[i]);
		priv_p->p_nodes[i] = NULL;
	}

	if (list_empty(&olap_head)) 
		return;

	priv_p->p_nr_done = 0;
	priv_p->p_run_nr = 0;
	list_for_each_entry(np, struct dcn_node, &olap_head, n_list) {
		priv_p->p_run_nr++;
	}
	i = 0;
	list_for_each_entry(np, struct dcn_node, &olap_head, n_list) {
		job = priv_p->p_jobs[i++];
		job->np = np;
		jh_p = (struct tp_jobheader *)job;
		tp_p->t_op->t_job_insert(tp_p, jh_p);
	}
	pthread_mutex_lock(&priv_p->p_lck);
	while (priv_p->p_nr_done != priv_p->p_run_nr) {
		pthread_cond_wait(&priv_p->p_cond, &priv_p->p_lck);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	priv_p->p_nr_done = 0;
	i = 0;
	list_for_each_entry(np, struct dcn_node, &olap_head, n_list) {
		job = priv_p->p_bck_jobs[i++];
		job->np = np;
		jh_p = (struct tp_jobheader *)job;
		tp_p->t_op->t_job_insert(tp_p, jh_p);
	}
	pthread_mutex_lock(&priv_p->p_lck);
	while (priv_p->p_nr_done != priv_p->p_run_nr) {
		pthread_cond_wait(&priv_p->p_cond, &priv_p->p_lck);
	}
	pthread_mutex_unlock(&priv_p->p_lck);


	list_for_each_entry_safe(np, np2, struct dcn_node, &olap_head, n_list) {
		nid_log_debug("_dit_do_run: olap_offset:%lu olap_len:%u", np->n_offset, np->n_len);
		list_del(&np->n_list);
		_put_data(dit_p, np);
	}

}

static void
dit_run(struct dit_interface *dit_p)
{
	char *log_header = "dit_run";
	struct dit_private *priv_p = (struct dit_private *)dit_p->d_private;
	struct timeval now;

	gettimeofday(&now, NULL);
	priv_p->p_time_stop = now.tv_sec + priv_p->p_time_to_run;
	nid_log_debug("%s: start to run %u secs", log_header, priv_p->p_time_to_run);
	while (!priv_p->p_to_stop) {
		_dit_do_run(dit_p);
	}
}

struct dit_operations dit_op = {
	.d_run = dit_run,	
};

int
dit_initialization(struct dit_interface *dit_p, struct dit_setup *setup)
{
	char *log_header = "dit_initialization";
	struct tp_interface *tp_p;
	struct olap_setup olap_setup;
	struct olap_interface *olap_p;
	struct dit_private *priv_p;
	struct dit_job *job;
	struct dit_timer_job *t_job;
	struct tp_jobheader* jh_p;
	int i;

	nid_log_info("dit_initialization start ...");
	dit_p->d_op = &dit_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dit_p->d_private = priv_p;

	pthread_mutex_init(&priv_p->p_lck, NULL);
	pthread_cond_init(&priv_p->p_cond, NULL);
	priv_p->p_ini = setup->ini;
	priv_p->p_tp = setup->tp;
	priv_p->p_dcn = setup->dcn;
	priv_p->p_rdg = setup->rdg;
	olap_setup.dcn = priv_p->p_dcn;
	olap_p = &priv_p->p_olap;
	olap_initialization(olap_p, &olap_setup);	
	priv_p->p_time_to_run = setup->time_to_run;
	priv_p->p_nr = setup->nr_threads;
	if (!priv_p->p_nr)
		priv_p->p_nr = 1;
	strcpy(priv_p->p_devname, setup->devname);
	strcpy(priv_p->p_bck_devname, setup->bck_devname);
	priv_p->p_fd = open(priv_p->p_devname, O_RDWR);
	if (priv_p->p_fd < 0)
		nid_log_error("%s: cannot open %s, errno:%d", log_header, priv_p->p_devname, errno);
	assert(priv_p->p_fd >= 0);

	priv_p->p_bck_fd = open(priv_p->p_bck_devname, O_RDWR);
	if (priv_p->p_bck_fd < 0)
		nid_log_error("%s: cannot open %s, errno:%d", log_header, priv_p->p_devname, errno);
	assert(priv_p->p_bck_fd >= 0);

	for (i = 0; i < priv_p->p_nr; i++) {
		job = calloc(1, sizeof(*job));
		job->header.jh_do = _dit_run_job;
		job->header.jh_free = NULL;
		job->dit = dit_p;
		job->idx = i;
		priv_p->p_jobs[i] = job;

		job = calloc(1, sizeof(*job));
		job->header.jh_do = _dit_run_bck_job;
		job->header.jh_free = NULL;
		job->dit = dit_p;
		job->idx = i;
		priv_p->p_bck_jobs[i] = job;
	}

	t_job = calloc(1, sizeof(*t_job));
	t_job->header.jh_do = _dit_timer_job;
	t_job->header.jh_free = NULL;
	t_job->dit = dit_p;
	jh_p = (struct tp_jobheader *)t_job;
	tp_p = priv_p->p_tp;
	tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 1);
	return 0;
}
