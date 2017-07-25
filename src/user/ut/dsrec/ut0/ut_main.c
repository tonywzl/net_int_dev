#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rtree_if.h"
#include "dsrec_if.h"

static struct dsrec_setup dsrec_setup;
static struct dsrec_interface ut_dsrec;

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
	nid_log_info("dsrec ut module start ...");

	dsrec_initialization(&ut_dsrec, &dsrec_setup);

	nid_log_info("dsrec ut module end...");
	nid_log_close();

	return 0;
}
