#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ddn_if.h"

static struct ddn_setup ddn_setup;
static struct ddn_interface ut_ddn;

struct node_interface;
struct node_setup;
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
	nid_log_info("ddn ut module start ...");

	ddn_initialization(&ut_ddn, &ddn_setup);

	nid_log_info("ddn ut module end...");
	nid_log_close();

	return 0;
}
