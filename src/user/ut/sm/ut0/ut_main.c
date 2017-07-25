#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "sm_if.h"

static struct sm_setup sw_setup;
static struct sm_interface ut_nw;

int
main()
{
	nid_log_open();
	nid_log_info("sm ut module start ...");

	sm_initialization(&ut_nw, &sw_setup);

	nid_log_info("sm ut module end...");
	nid_log_close();

	return 0;
}
