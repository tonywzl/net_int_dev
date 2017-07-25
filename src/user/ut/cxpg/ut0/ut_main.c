#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "cxpg_if.h"

static struct cxpg_setup cxpg_setup;
static struct cxpg_interface ut_cxpg;

struct cxp_interface;
struct cxp_setup;
int
cxp_initialization(struct cxp_interface *cxp_p, struct cxp_setup *setup)
{
	assert(cxp_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("cxpg ut module start ...");

	cxpg_initialization(&ut_cxpg, &cxpg_setup);

	nid_log_info("cxpg ut module end...");
	nid_log_close();

	return 0;
}
