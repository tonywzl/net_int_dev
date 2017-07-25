#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#include "nid_log.h"
#include "tp_if.h"

static struct tp_interface ut_tp;

int
main()
{
	struct tp_setup setup = {NULL, 1, 128, 1, 1};

	nid_log_open();
	nid_log_info("threadpool start ...");

	tp_initialization(&ut_tp, &setup);

	nid_log_info("threadpool end...");
	nid_log_close();

	return 0;
}
