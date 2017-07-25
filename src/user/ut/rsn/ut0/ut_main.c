#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rsn_if.h"

static struct rsn_setup rsn_setup;
static struct rsn_interface ut_rsn;

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
	nid_log_info("rsn ut module start ...");

	rsn_initialization(&ut_rsn, &rsn_setup);

	nid_log_info("rsn ut module end...");
	nid_log_close();

	return 0;
}
