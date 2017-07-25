#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rept_if.h"
#include "allocator_if.h"

static struct rept_setup ut_rept_setup;
static struct rept_interface ut_rept;

int
main()
{
	nid_log_open();
	nid_log_info("rept ut module start ...");

	struct allocator_interface *allocator_p;
	struct allocator_setup allocator_setup;

	allocator_p = calloc(1, sizeof(*allocator_p));
	allocator_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(allocator_p, &allocator_setup);

	strcpy(ut_rept_setup.rt_name, "itest_rt_name");
	strcpy(ut_rept_setup.voluuid, "itest_vol_name");
	ut_rept_setup.allocator = allocator_p;

	rept_initialization(&ut_rept, &ut_rept_setup);

	nid_log_info("rept ut module end...");
	nid_log_close();

	return 0;
}
