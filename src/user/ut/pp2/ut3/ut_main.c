#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "allocator_if.h"
#include "tp_if.h"
#include "pp2_if.h"


#define N_THREADS	10
#define N_REPEAT	1000000

int nid_log_level = LOG_WARNING;

struct sleep_job {
	struct tp_jobheader	j_header;
	int			j_s;
	struct pp2_interface	*j_pp2;
};

struct r_sleep_job {
	struct tp_jobheader	j_header;
	int			j_s;
	int			j_nr;
	struct tp_interface	*j_tp;
//	struct pp2_interface	*j_pp2;
};

struct thread_args
{
	struct tp_interface	*tp;
	struct pp2_interface	*pp2;
	int			do_r;
};

static void
__do_sleep_job(struct tp_jobheader *jheader)
{
	struct sleep_job *job = (struct sleep_job *)jheader;

	sleep(job->j_s);
}

static void
__free_sleep_job(struct tp_jobheader *jheader)
{
	struct sleep_job *job = (struct sleep_job *)jheader;
	struct pp2_interface *pp2_p = job->j_pp2;

	pp2_p->pp2_op->pp2_put(pp2_p, (void *)job);
}

static void
__do_r_sleep_job(struct tp_jobheader *jheader)
{
	struct r_sleep_job *job = (struct r_sleep_job *)jheader;
	struct tp_interface *tp_p = job->j_tp;

	if (job->j_nr--) {
		sleep(job->j_s);
		tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, 1);
	}
}

static void *
_load_thread(void *p)
{
	struct thread_args *args = (struct thread_args *)p;
	struct tp_interface *tp_p = args->tp;
	struct pp2_interface *pp2_p = args->pp2;

	if (args->do_r) {
		struct r_sleep_job *job = (struct r_sleep_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
		job->j_s = rand() % 3 + 1;
		job->j_tp = tp_p;
//		job->j_pp2 = pp2_p;
		job->j_nr = N_REPEAT;
		job->j_header.jh_do = __do_r_sleep_job;
		job->j_header.jh_free = NULL;
		tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, 1);
	} else {
		int i;
		for (i = 0; i < N_REPEAT; i++) {
			struct sleep_job *job = (struct sleep_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
			job->j_s = rand() % 3 + 1;
			job->j_pp2 = pp2_p;
			job->j_header.jh_do = __do_sleep_job;
			job->j_header.jh_free = __free_sleep_job;
			tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, 1);
		}
	}

	return NULL;
}

int
main()
{
	char *log_header = "pp2 ut3";
	struct pp2_setup pp2_setup;
	struct pp2_interface ut_pp2;
	struct allocator_setup allocator_setup;
	struct allocator_interface allocator;
	struct pp2_interface *pp2_p = &ut_pp2;
	struct tp_setup tp_setup;
	struct tp_interface *tp_p;
	pthread_t threads[N_THREADS];
	struct thread_args thread_args[N_THREADS];
	int i, half;
#ifdef DEBUG_LEVEL
	nid_log_level = DEBUG_LEVEL;
#endif
	nid_log_open();
	nid_log_info("%s module start ...", log_header);

	allocator_setup.a_role = 1;
	allocator_initialization(&allocator, &allocator_setup);
	pp2_setup.name = "pp2 ut3";
	pp2_setup.allocator = &allocator;
	pp2_setup.set_id = ALLOCATOR_SET_AGENT_PP2;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 16;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 1;
	pp2_p = &ut_pp2;
	pp2_initialization(pp2_p, &pp2_setup);

	strcpy(tp_setup.name, "agent_tp");
	tp_setup.pp2 = pp2_p;
	tp_setup.min_workers = 1;
	tp_setup.max_workers = 0;
	tp_setup.extend = 1;
	tp_setup.delay = 1;
	tp_p = (struct tp_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*tp_p));
	tp_initialization(tp_p, &tp_setup);

	half = N_THREADS / 2;
	for (i = 0; i < N_THREADS; i++) {
		thread_args[i].pp2 = pp2_p;
		thread_args[i].tp = tp_p;
		if (i < half)
			thread_args[i].do_r = 1;
		else
			thread_args[i].do_r = 0;

		pthread_create(&threads[i], NULL, _load_thread, &thread_args[i]);
	}

	for (i = 0; i < N_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	nid_log_info("%s module end...", log_header);
	nid_log_close();

	return 0;
}
