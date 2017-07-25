#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dev_if.h"

static struct dev_setup dev_setup;
static struct dev_interface ut_dev;

int
main()
{
	nid_log_open();
	nid_log_info("dev ut module start ...");

	dev_initialization(&ut_dev, &dev_setup);

	nid_log_info("dev ut module end...");
	nid_log_close();

	return 0;
}
