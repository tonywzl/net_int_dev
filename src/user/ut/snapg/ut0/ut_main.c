#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "snap_if.h"

static struct snap_setup snap_setup;
static struct snap_interface ut_snap;

int
snap_initialization(struct snap_interface *snap_p, struct snap_setup *setup)
{
	assert(snap_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("snap ut module start ...");

	snap_initialization(&ut_snap, &snap_setup);

	nid_log_info("snap ut module end...");
	nid_log_close();

	return 0;
}
