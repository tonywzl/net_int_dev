#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "drv_if.h"

static struct drv_setup drv_setup;
static struct drv_interface ut_drv;

int
main()
{
	nid_log_open();
	nid_log_info("drv ut module start ...");

	drv_initialization(&ut_drv, &drv_setup);

	nid_log_info("drv ut module end...");
	nid_log_close();

	return 0;
}
