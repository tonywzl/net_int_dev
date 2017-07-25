#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dxacg_if.h"

static struct dxacg_setup bwccg_setup;
static struct dxacg_interface ut_bwccg;

struct dxac_interface;
struct dxac_setup;
int
dxac_initialization(struct dxac_interface *dxac_p, struct dxac_setup *setup)
{
	assert(dxac_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("dxacg ut module start ...");

	dxacg_initialization(&ut_bwccg, &bwccg_setup);

	nid_log_info("dxacg ut module end...");
	nid_log_close();

	return 0;
}
