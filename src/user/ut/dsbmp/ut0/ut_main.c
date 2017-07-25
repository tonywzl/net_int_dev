#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rtree_if.h"
#include "dsbmp_if.h"

static struct dsbmp_setup dsbmp_setup;
static struct dsbmp_interface ut_dsbmp;

int
rtree_initialization(struct rtree_interface *rtree_p, struct rtree_setup *setup)
{
	assert(rtree_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("dsbmp ut module start ...");

	dsbmp_initialization(&ut_dsbmp, &dsbmp_setup);

	nid_log_info("dsbmp ut module end...");
	nid_log_close();

	return 0;
}
