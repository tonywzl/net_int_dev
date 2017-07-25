#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "fpn_if.h"

static struct fpn_setup fpn_setup;
static struct fpn_interface ut_fpn;

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
	nid_log_info("fpn ut module start ...");

	fpn_initialization(&ut_fpn, &fpn_setup);

	nid_log_info("fpn ut module end...");
	nid_log_close();

	return 0;
}
