/*
 * tp.c
 * 	Implementation of Thread Pool Module
 */

#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>

#include "list.h"
#include "nid_log.h"
#include "tp_if.h"
#include "pp2_if.h"

#define TP_POOL_MAX	1024

struct tp_private;
struct tp_thread {
	struct list_head	t_list;
	struct tp_private	*t_priv;
	sem_t			t_sem;
	struct tp_jobheader	*t_job;
	char 			t_stop;
};

struct tp_private {
	struct tp_stat	p_stat;
	struct pp2_interface 	*p_pp2;
	pthread_mutex_t		p_lck;
	pthread_mutex_t		p_delay_lck;
	pthread_cond_t		p_job_cond;
	pthread_cond_t		p_free_cond;
	uint16_t		p_nused;	// number of busy threads
	uint16_t		p_nfree;	// number of free threads
	uint16_t		p_min_workers;	// minimal number of workers
	uint16_t		p_max_workers;	// maximal number of workers
	uint16_t		p_workers;	// number of all threads (p_nused + p_nfree)
	uint16_t		p_extend;	// number of workers created each expend
	uint16_t		p_alive_counter;	// number of workers that has not exited
	struct list_head	p_jobs;
	struct list_head	p_delay_jobs;
	struct list_head	p_used;
	struct list_head	p_free;
	struct tp_thread	p_threads[TP_POOL_MAX];
	time_t			p_last_shrink;
	time_t			p_last_nofree;
	char			p_do_delay;
	char			p_job_busy;
	char			p_free_busy;
	char			p_stop;
	char			p_cntl_stopped;
	char			p_cntl_delay_stopped;
	char			p_name[NID_MAX_TPNAME];
};

/*
 * the caller should hold p_lck
 */
static void
_tp_shrink(struct tp_private *priv_p)
{
	struct tp_thread *thread_p;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (list_empty(&priv_p->p_free)) {
		return;
	}
	thread_p = list_first_entry(&priv_p->p_free, struct tp_thread, t_list);
	if (priv_p->p_workers > priv_p->p_max_workers ||
	    (priv_p->p_last_nofree + 10 < tv.tv_sec && priv_p->p_nfree)) {
		list_del(&thread_p->t_list);
		priv_p->p_nfree--;
		priv_p->p_workers--;
		thread_p->t_stop = 1;
		sem_post(&thread_p->t_sem);
	}
}

/*
 * thread to do a job
 */
static void*
_tp_worker_thread(void *p)
{
	struct tp_thread *thread_p = (struct tp_thread *)p;
	struct tp_private *priv_p = thread_p->t_priv;
	struct tp_jobheader *jh_p;
	struct timeval tv;

	nid_log_info("_tp_worker_thread start...");
next_job:
	sem_wait(&thread_p->t_sem);
	if (thread_p->t_stop) {
		nid_log_info("_tp_worker_thread: stop\n");
		sem_destroy(&thread_p->t_sem);
		thread_p->t_priv = NULL;
		assert(priv_p->p_alive_counter);
		priv_p->p_alive_counter--;
		return NULL;
	}

do_job:
	if (thread_p->t_job) {
		if (thread_p->t_job->jh_do)
			thread_p->t_job->jh_do(thread_p->t_job);
		if (thread_p->t_job->jh_free)
			thread_p->t_job->jh_free(thread_p->t_job);
		thread_p->t_job->jh_d_in_use = thread_p->t_job->jh_j_in_use = 0;
		thread_p->t_job = NULL;
	}

	jh_p = NULL;
	if (!list_empty(&priv_p->p_jobs)) {
		pthread_mutex_lock(&priv_p->p_lck);
		if (!list_empty(&priv_p->p_jobs)) {
			jh_p = list_first_entry(&priv_p->p_jobs, struct tp_jobheader, jh_list);
			list_del(&jh_p->jh_list);
		}
		pthread_mutex_unlock(&priv_p->p_lck);
	}
	if ((thread_p->t_job = jh_p)) {
		goto do_job;
	}

	/*
	 * After finished the job, move this thread from used list to free list
	 * and also send signal to the control thread in case it's waiting for
	 * a free worker thread
	 */
	gettimeofday(&tv, NULL);
	pthread_mutex_lock(&priv_p->p_lck);
	list_del(&thread_p->t_list);				// removed from p_used
	priv_p->p_nused--;
	list_add_tail(&thread_p->t_list, &priv_p->p_free);	// add to free list
	priv_p->p_nfree++;
	if (priv_p->p_free_busy == 0) {
		pthread_cond_signal(&priv_p->p_free_cond);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	goto next_job;
}

static void
_tp_worker_thread_init(struct tp_thread *thread_p, struct tp_private *priv_p)
{
	memset(thread_p, 0, sizeof(*thread_p));
	thread_p->t_priv = priv_p;
	sem_init(&thread_p->t_sem, 0, 0);
}

static void
_tp_extend(struct tp_private *priv_p)
{
	struct tp_thread *thread_p;
	pthread_t thread_id;
	pthread_attr_t attr;
	int extend, i;
	struct timeval tv;
	int counter = 0;

	//nid_log_info("_tp_extend start...");
	if (priv_p->p_workers >= priv_p->p_max_workers) {
		return;
	}
	if (priv_p->p_workers + priv_p->p_extend > priv_p->p_max_workers) {
		extend = 1;
	} else {
		extend = priv_p->p_extend;
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	thread_p = priv_p->p_threads;
	for (i = 0; i < extend; i++) {
		while (thread_p->t_priv) {
			thread_p++;	// the slot has been occupied by a worker thread
		}
		_tp_worker_thread_init(thread_p, priv_p);
		while ((pthread_create(&thread_id, &attr, _tp_worker_thread, thread_p)) < 0) {
			if (++counter > 5)
				assert(0);
			nid_log_error("_tp_extend: pthread_create error %d\n", errno);
			sleep(1);
		}
		gettimeofday(&tv, NULL);
		pthread_mutex_lock(&priv_p->p_lck);
		list_add_tail(&thread_p->t_list, &priv_p->p_free);
		priv_p->p_nfree++;
		priv_p->p_workers++;
		priv_p->p_alive_counter++;
		pthread_mutex_unlock(&priv_p->p_lck);
		thread_p++;
	}
}

static void*
_tp_cntl_thread(void *p)
{
	struct tp_interface *tp_p = (struct tp_interface *)p;
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	struct tp_stat *stat_p = &priv_p->p_stat;
	struct tp_thread *thread_p;
	struct tp_jobheader *jh_p;
	struct timespec ts;
	struct timeval tv;

	nid_log_info("_tp_cntl_thread start...");
	gettimeofday(&tv, NULL);
	priv_p->p_last_shrink = tv.tv_sec;
next_job:
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_free_busy = 1;
	priv_p->p_job_busy = 1;
	while (list_empty(&priv_p->p_jobs) && !priv_p->p_stop) {
		ts.tv_sec = time(NULL) + 1;
		ts.tv_nsec = 0;
		priv_p->p_job_busy = 0;
		pthread_cond_timedwait(&priv_p->p_job_cond, &priv_p->p_lck, &ts);
		priv_p->p_job_busy = 1;

		gettimeofday(&tv, NULL);
		if (priv_p->p_last_shrink + 5 < tv.tv_sec) {
			_tp_shrink(priv_p);
			priv_p->p_last_shrink = tv.tv_sec;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (priv_p->p_stop) {
		pthread_mutex_lock(&priv_p->p_lck);
		list_for_each_entry(thread_p, struct tp_thread, &priv_p->p_free, t_list) {
			thread_p->t_stop = 1;
			sem_post(&thread_p->t_sem);
		}

		list_for_each_entry(thread_p, struct tp_thread, &priv_p->p_used, t_list) {
			thread_p->t_stop = 1;
			sem_post(&thread_p->t_sem);
		}
		pthread_mutex_unlock(&priv_p->p_lck);
		priv_p->p_cntl_stopped = 1;
		return NULL;
	}

	/*
	 * we are here since we got some jobs to do
	 */
	while (!list_empty(&priv_p->p_jobs)) {
		pthread_mutex_lock(&priv_p->p_lck);
		if (priv_p->p_nfree == 0) {
			stat_p->s_no_free++;
			pthread_mutex_unlock(&priv_p->p_lck);
			_tp_extend(priv_p);
			pthread_mutex_lock(&priv_p->p_lck);
		}
		while (list_empty(&priv_p->p_free)) {
			priv_p->p_free_busy = 0;
			pthread_cond_wait(&priv_p->p_free_cond, &priv_p->p_lck);
			priv_p->p_free_busy = 1;
		}

		/*
		 * Need check again. we lost lock when doing pthread_cond_wait
		 * worker threads can grap jobs themselves after they finish a their jobd
		 */
		if (list_empty(&priv_p->p_jobs)) {
			pthread_mutex_unlock(&priv_p->p_lck);
			break;
		}

		jh_p = list_first_entry(&priv_p->p_jobs, struct tp_jobheader, jh_list);
		nid_log_debug("del p_jobs: %p, jh_list: %p", &priv_p->p_jobs, &jh_p->jh_list);
		list_del(&jh_p->jh_list);

		thread_p = list_entry(priv_p->p_free.next, struct tp_thread, t_list);
		list_del(&thread_p->t_list);	// removed from p_free
		priv_p->p_nfree--;
		if (!priv_p->p_nfree) {
			gettimeofday(&tv, NULL);
			priv_p->p_last_nofree = tv.tv_sec;
		}
		list_add_tail(&thread_p->t_list, &priv_p->p_used);
		priv_p->p_nused++;
		pthread_mutex_unlock(&priv_p->p_lck);
		thread_p->t_job = jh_p;		// assign the job to the thread
		sem_post(&thread_p->t_sem);	// wakeup the thread to do the job
	}

	goto next_job;
}

static void*
_tp_cntl_delay_thread(void *p)
{
	struct tp_interface *tp_p = (struct tp_interface *)p;
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	struct tp_jobheader *jh_p, *tj_p1;

next_job:
	if (priv_p->p_stop) {
		priv_p->p_cntl_delay_stopped = 1;
		return NULL;
	}

	pthread_mutex_lock(&priv_p->p_delay_lck);
	list_for_each_entry(jh_p, struct tp_jobheader, &priv_p->p_delay_jobs, jh_list) {
		if (jh_p->jh_delay-- == 0) {
			tj_p1 = (struct tp_jobheader *)jh_p->jh_list.prev;
			list_del(&jh_p->jh_list);
			tp_p->t_op->t_job_insert(tp_p, jh_p);
			jh_p = tj_p1;
		}
	}
	pthread_mutex_unlock(&priv_p->p_delay_lck);
	sleep(1);
	goto next_job;
}

/*
 * Insert a job to the thread pool job list
 */
static int
tp_job_insert(struct tp_interface *tp_p, struct tp_jobheader *jh_p)
{
	char *log_header = "tp_job_insert";
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	int rejected = 0;

	pthread_mutex_lock(&priv_p->p_lck);
	if (jh_p->jh_j_in_use) {
		rejected = 1;
		goto out;
	}
	jh_p->jh_j_in_use = 1;
	list_add_tail(&jh_p->jh_list, &priv_p->p_jobs);
	if (priv_p->p_job_busy == 0) {
		pthread_cond_signal(&priv_p->p_job_cond);
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);
	if (rejected)
		nid_log_notice("%s: rejected, d_in_use:%d, j_in_use:%d",
				log_header, (int)jh_p->jh_d_in_use, (int)jh_p->jh_j_in_use);
	return rejected;
}

/*
 * Insert a delay job to the thread pool job list
 */
static int
tp_delay_job_insert(struct tp_interface* tp_p, struct tp_jobheader* jh_p, int delay)
{
	char *log_header = "tp_delay_job_insert";
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	int rejected = 0;

	assert(priv_p->p_do_delay);
	pthread_mutex_lock(&priv_p->p_delay_lck);
	if (jh_p->jh_d_in_use) {
		rejected = 1;
		goto out;
	}
	jh_p->jh_d_in_use = 1;
	jh_p->jh_delay = delay;
	list_add_tail(&jh_p->jh_list, &priv_p->p_delay_jobs);
out:
	pthread_mutex_unlock(&priv_p->p_delay_lck);
	if (rejected)
		nid_log_notice("%s: rejected, d_in_use:%d, j_in_use:%d",
				log_header, (int)jh_p->jh_d_in_use, (int)jh_p->jh_j_in_use);
	return rejected;
}

static int
tp_get_job_in_use(struct tp_interface* tp_p, struct tp_jobheader* jh_p)
{
	(void)tp_p;

	return jh_p->jh_d_in_use || jh_p->jh_j_in_use;
}

static void
tp_stop(struct tp_interface *tp_p)
{
	char *log_header = "tp_stop";
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_stop = 1;
	if (priv_p->p_job_busy == 0) {
		pthread_cond_signal(&priv_p->p_job_cond);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static uint16_t
tp_get_max_workers(struct tp_interface *tp_p)
{
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	return priv_p->p_max_workers;
}

static void
tp_set_max_workers(struct tp_interface *tp_p, uint16_t max_workers)
{
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	priv_p->p_max_workers = max_workers;
}

static void
tp_get_stat(struct tp_interface *tp_p, struct tp_stat *stat_p)
{
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	nid_log_debug("tp_get_stat start...");
	pthread_mutex_lock(&priv_p->p_lck);
	stat_p->s_nused = priv_p->p_nused;
	stat_p->s_nfree = priv_p->p_nfree;
	stat_p->s_workers = priv_p->p_workers;
	stat_p->s_max_workers = priv_p->p_max_workers;
	stat_p->s_no_free = priv_p->p_stat.s_no_free;
	pthread_mutex_unlock(&priv_p->p_lck);
}

static char *
tp_get_name(struct tp_interface *tp_p)
{
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	nid_log_debug("tp_get_name start...");
	return priv_p->p_name;
}

static void
tp_reset_stat(struct tp_interface *tp_p)
{
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	nid_log_debug("tp_reset_stat start...");
	memset(&priv_p->p_stat, 0, sizeof(priv_p->p_stat));
	assert(priv_p);
}

static void
tp_cleanup(struct tp_interface *tp_p)
{
	char *log_header = "tp_cleanup";
	struct tp_private *priv_p = (struct tp_private *)tp_p->t_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	int nretry;

	nid_log_warning("%s: start ...", log_header);
retry:
	if (!priv_p->p_cntl_stopped || priv_p->p_alive_counter || (priv_p->p_do_delay && !priv_p->p_cntl_delay_stopped)) {
		nid_log_warning("%s: calive_counter:%u, cntl_stopped:%d, cntl_delay_stopped:%d, do_delay:%d, going to retry:%d",
				log_header, priv_p->p_alive_counter,
				(int)priv_p->p_cntl_stopped, (int)priv_p->p_cntl_delay_stopped, (int)priv_p->p_do_delay,
				++nretry);
		sleep(1);
		goto retry;
	}

	pthread_cond_destroy(&priv_p->p_job_cond);
	pthread_cond_destroy(&priv_p->p_free_cond);
	pthread_mutex_destroy(&priv_p->p_lck);
	if (priv_p->p_do_delay)
		pthread_mutex_destroy(&priv_p->p_delay_lck);
	pp2_p->pp2_op->pp2_put(pp2_p, (void*)priv_p);
	tp_p->t_private = NULL;
	nid_log_warning("%s: end ...", log_header);
}

static void
tp_destroy(struct tp_interface *tp_p)
{
	char *log_header = "tp_destroy";

	nid_log_warning("%s: start ...", log_header);
	tp_stop(tp_p);
	tp_cleanup(tp_p);
	nid_log_warning("%s: end ...", log_header);
}

struct tp_operations tp_op = {
	.t_job_insert = tp_job_insert,
	.t_delay_job_insert = tp_delay_job_insert,
	.t_stop = tp_stop,
	.t_get_max_workers = tp_get_max_workers,
	.t_set_max_workers = tp_set_max_workers,
	.t_get_stat = tp_get_stat,
	.t_reset_stat = tp_reset_stat,
	.t_cleanup = tp_cleanup,
	.t_get_name = tp_get_name,
	.t_get_job_in_use = tp_get_job_in_use,
	.t_destroy = tp_destroy,
};

/*
 * Setup a thread pool, but not start it
 */
int
tp_initialization(struct tp_interface *tp_p, struct tp_setup *setup)
{
	struct tp_private *priv_p;
	struct pp2_interface *pp2_p = setup->pp2;
	pthread_t thread_id;
	pthread_attr_t attr, attrd;

	if (!setup) {
		return -1;
	}

	nid_log_debug("tp_initialization: setup(%d, %d, %d)",
		setup->min_workers, setup->max_workers, setup->extend);
	/*
	 * threadpool interface init
	 */
	priv_p = (struct tp_private *) pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	tp_p->t_private = (void *)priv_p;
	tp_p->t_op = &tp_op;

	/*
	 * tp internal init
	 */
	pthread_mutex_init(&priv_p->p_lck, NULL);
	INIT_LIST_HEAD(&priv_p->p_used);
	INIT_LIST_HEAD(&priv_p->p_free);
	INIT_LIST_HEAD(&priv_p->p_jobs);
	INIT_LIST_HEAD(&priv_p->p_delay_jobs);
	pthread_cond_init(&priv_p->p_job_cond, NULL);
	pthread_cond_init(&priv_p->p_free_cond, NULL);
	if (!(priv_p->p_min_workers = setup->min_workers)) {
		priv_p->p_min_workers = 1;
	}
	if (!(priv_p->p_max_workers = setup->max_workers)) {
		priv_p->p_max_workers = TP_POOL_MAX;	// no limitation
	}
	if (!(priv_p->p_extend = setup->extend)) {
		priv_p->p_extend = 1;
	}
	if (setup->delay)
		priv_p->p_do_delay = 1;
	priv_p->p_pp2 = pp2_p;
//	priv_p->p_name = setup->name;
	strcpy(priv_p->p_name, setup->name);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_attr_init(&attrd);
	pthread_attr_setdetachstate(&attrd, PTHREAD_CREATE_DETACHED);

	while (priv_p->p_nfree < priv_p->p_min_workers) {
		_tp_extend(priv_p);
	}

	/* start the control thread after worker threads */
	pthread_create(&thread_id, &attr, _tp_cntl_thread, tp_p);
	/* start the delay thread */
	if (priv_p->p_do_delay) {
		pthread_mutex_init(&priv_p->p_delay_lck, NULL);
		pthread_create(&thread_id, &attrd, _tp_cntl_delay_thread, tp_p);
	}

	return 0;
}
