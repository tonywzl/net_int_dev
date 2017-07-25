#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#include "nid_log.h"
#include "tp_if.h"

static struct tp_interface ut_tp;

struct job {
	struct tp_jobheader	header;
	int			res;
	int			n;
	struct tp_interface	*tp;
};

void job_do(struct tp_jobheader *header)
{
	struct job *job = (struct job *)header;
	struct tp_interface *tp_p = job->tp;

	if (job->n < 10 || job->n%10 == 0)
		nid_log_debug("this is %dth job", job->n);
	job->res = 0;

	tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, job->n);
}

int
main(int argc, char* argv[])
{
	struct tp_setup setup = {"",10, 100, 5, 1};
	struct job *the_job;
	int i, num;

	nid_log_open();
	nid_log_info("threadpool start ...");

	if (argc < 2) {
		num = 100;
	} else {
		num = atoi(argv[1]);
	}

	tp_initialization(&ut_tp, &setup);
	for (i = 0; i < num; i++) {
		the_job = calloc(1, sizeof(*the_job));
		the_job->n = i;
		the_job->header.jh_do = job_do;
		the_job->tp = &ut_tp;
		ut_tp.t_op->t_delay_job_insert(&ut_tp, (struct tp_jobheader *)the_job, i);
	}
	sleep(10);
	ut_tp.t_op->t_exit(&ut_tp);
	nid_log_info("threadpool end...");
	nid_log_close();

	return 0;
}
