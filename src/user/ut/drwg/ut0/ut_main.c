#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "drwg_if.h"

static struct drwg_setup drwg_setup;
static struct drwg_interface ut_drwg;

int
main()
{
	nid_log_open();
	nid_log_info("drwg ut module start ...");

	drwg_initialization(&ut_drwg, &drwg_setup);

	nid_log_info("drwg ut module end...");
	nid_log_close();

	return 0;
}
