#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rtree_if.h"
#include "brn_if.h"
#include "blksn_if.h"
#include "dsbmp_if.h"
#include "dsmgr_if.h"

static struct dsmgr_setup dsmgr_setup;
static struct dsmgr_interface ut_dsmgr;

int
rtree_initialization(struct rtree_interface *rtree_p, struct rtree_setup *setup)
{
	assert(rtree_p && setup);
	return 0;
}

int
dsbmp_initialization(struct dsbmp_interface *dsbmp_p, struct dsbmp_setup *setup)
{
	assert(dsbmp_p && setup);
	return 0;
}

int
brn_initialization(struct brn_interface *brn_p, struct brn_setup *setup)
{
	assert(brn_p && setup);
	return 0;
}

int
blksn_initialization(struct blksn_interface *blksn_p, struct blksn_setup *setup)
{
	assert(blksn_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("dsmgr ut module start ...");
	dsmgr_setup.size = (1000 * 64 ) + 1;
	dsmgr_initialization(&ut_dsmgr, &dsmgr_setup);

	nid_log_info("dsmgr ut module end...");
	nid_log_close();

	return 0;
}
