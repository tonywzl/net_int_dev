#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dxpg_if.h"

static struct dxpg_setup dxpg_setup;
static struct dxpg_interface ut_dxpg;

struct dxp_interface;
struct dxp_setup;
int
dxp_initialization(struct dxp_interface *dxp_p, struct dxp_setup *setup)
{
	assert(dxp_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("dxpg ut module start ...");

	dxpg_initialization(&ut_dxpg, &dxpg_setup);

	nid_log_info("dxpg ut module end...");
	nid_log_close();

	return 0;
}
