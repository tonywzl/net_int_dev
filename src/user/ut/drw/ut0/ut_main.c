#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "drw_if.h"

static struct drw_setup drw_setup;
static struct drw_interface ut_drw;

struct fpc_interface;
struct fpc_setup;
int
fpc_initialization(struct fpc_interface *fpc, struct fpc_setup *setup)
{
	assert(fpc && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("drw ut module start ...");

	drw_initialization(&ut_drw, &drw_setup);

	nid_log_info("drw ut module end...");
	nid_log_close();

	return 0;
}
