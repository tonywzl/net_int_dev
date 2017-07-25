#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "blksn_if.h"

static struct blksn_setup blksn_setup;
static struct blksn_interface ut_blksn;

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
	nid_log_info("blksn ut module start ...");

	blksn_initialization(&ut_blksn, &blksn_setup);

	nid_log_info("blksn ut module end...");
	nid_log_close();

	return 0;
}
