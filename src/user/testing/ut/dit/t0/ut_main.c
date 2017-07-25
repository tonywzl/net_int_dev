#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "olap_if.h"
#include "dit_if.h"

static struct dit_setup dit_setup;
static struct dit_interface ut_dit;

int
olap_initialization(struct olap_interface * olap_p, struct olap_setup *setup)
{
	assert(olap_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("dit ut module start ...");

	dit_initialization(&ut_dit, &dit_setup);

	nid_log_info("dit ut testing end...");
	nid_log_close();

	return 0;
}
