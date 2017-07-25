#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "brn_if.h"

static struct brn_setup brn_setup;
static struct brn_interface ut_brn;

int
node_initialization(struct node_interface *node_p, struct node_setup *setup)
{
	assert(node_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("brn ut module start ...");

	brn_initialization(&ut_brn, &brn_setup);

	nid_log_info("brn ut module end...");
	nid_log_close();

	return 0;
}
