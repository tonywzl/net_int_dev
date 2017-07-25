#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rdwn_if.h"

static struct rdwn_setup rdwn_setup;
static struct rdwn_interface ut_rdwn;

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
	nid_log_info("rdwn ut module start ...");

	rdwn_initialization(&ut_rdwn, &rdwn_setup);

	nid_log_info("rdwn ut module end...");
	nid_log_close();

	return 0;
}
