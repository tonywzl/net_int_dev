#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rio_if.h"

static struct rio_setup rio_setup;
static struct rio_interface ut_rio;

int
main()
{
	nid_log_open();
	nid_log_info("rio ut module start ...");

	rio_initialization(&ut_rio, &rio_setup);

	nid_log_info("rio ut module end...");
	nid_log_close();

	return 0;
}
