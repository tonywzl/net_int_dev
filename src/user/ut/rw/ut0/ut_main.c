#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "drw_if.h"
#include "mrw_if.h"
#include "rw_if.h"

static struct rw_setup rw_setup;
static struct rw_interface ut_rw;

int
drw_initialization(struct drw_interface *drw_p, struct drw_setup *setup)
{
	assert(drw_p && setup);
	return 0;
}

int
mrw_initialization(struct mrw_interface *mrw_p, struct mrw_setup *setup)
{
	assert(mrw_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("rw ut module start ...");

	rw_initialization(&ut_rw, &rw_setup);

	nid_log_info("rw ut module end...");
	nid_log_close();

	return 0;
}
