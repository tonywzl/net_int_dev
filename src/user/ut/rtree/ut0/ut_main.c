#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bit_if.h"
#include "rtn_if.h"
#include "rtree_if.h"

static struct rtree_setup rtree_setup;
static struct rtree_interface ut_rtree;

int
rtn_initialization(struct rtn_interface *rtn_p, struct rtn_setup *setup)
{
	assert(rtn_p && setup);
	return 0;
}

int
bit_initialization(struct bit_interface *bit_p, struct bit_setup *setup)
{
	assert(bit_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("rtree ut module start ...");

	rtree_initialization(&ut_rtree, &rtree_setup);

	nid_log_info("rtree ut module end...");
	nid_log_close();

	return 0;
}
