#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rgdevg_if.h"

static struct rgdevg_setup rgdevg_setup;
static struct rgdevg_interface ut_rgdevg;

int
main()
{
	nid_log_open();
	nid_log_info("rgdevg ut module start ...");

	rgdevg_initialization(&ut_rgdevg, &rgdevg_setup);

	nid_log_info("rgdevg ut module end...");
	nid_log_close();

	return 0;
}
