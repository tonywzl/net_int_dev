#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rccg_if.h"

static struct rccg_setup rccg_setup;
static struct rccg_interface ut_rccg;

struct rcc_interface;
struct rcc_setup;
int
rcc_initialization(struct rcc_interface *rcc_p, struct rcc_setup *setup)
{
	assert(rcc_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("rccg ut module start ...");

	rccg_initialization(&ut_rccg, &rccg_setup);

	nid_log_info("rccg ut module end...");
	nid_log_close();

	return 0;
}
