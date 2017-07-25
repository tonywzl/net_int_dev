#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rgdev_if.h"

static struct rgdev_setup rgdev_setup;
static struct rgdev_interface ut_dev;

int
main()
{
	nid_log_open();
	nid_log_info("dev ut module start ...");

	rgdev_initialization(&ut_dev, &rgdev_setup);

	nid_log_info("dev ut module end...");
	nid_log_close();

	return 0;
}
