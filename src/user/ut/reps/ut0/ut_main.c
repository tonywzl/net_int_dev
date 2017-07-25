#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "reps_if.h"
#include "allocator_if.h"

static struct reps_setup reps_setup;
static struct reps_interface ut_reps;

int
main()
{
	nid_log_open();
	nid_log_info("reps ut module start ...");

	struct allocator_interface *allocator_p;
	struct allocator_setup allocator_setup;

	allocator_p = calloc(1, sizeof(*allocator_p));
	allocator_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(allocator_p, &allocator_setup);

	strcpy(reps_setup.rs_name, "itest_rs_name");
	strcpy(reps_setup.rt_name, "itest_rt_name");
	strcpy(reps_setup.voluuid, "itest_vol_name");
	strcpy(reps_setup.snapuuid, "itest_sp1_name");
	strcpy(reps_setup.snapuuid2, "itest_sp2_name");
	reps_setup.allocator = allocator_p;

	reps_initialization(&ut_reps, &reps_setup);

	nid_log_info("reps ut module end...");
	nid_log_close();

	return 0;
}
