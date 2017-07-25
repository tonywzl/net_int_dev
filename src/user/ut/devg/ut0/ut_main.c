#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "devg_if.h"

static struct devg_setup devg_setup;
static struct devg_interface ut_devg;

int
devg_prepare(struct devg_interface *devg_p, struct devg_setup *setup)
{
	setup->ini = NULL;
	(void)devg_p;

	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("devg ut module start ...");

	devg_prepare(&ut_devg, &devg_setup);
	devg_initialization(&ut_devg, &devg_setup);

	nid_log_info("devg ut module end...");
	nid_log_close();

	return 0;
}
