#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rtn_if.h"

static struct rtn_setup rtn_setup;
static struct rtn_interface ut_rtn;

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
	nid_log_info("rtn ut module start ...");

	rtn_initialization(&ut_rtn, &rtn_setup);

	nid_log_info("rtn ut module end...");
	nid_log_close();

	return 0;
}
