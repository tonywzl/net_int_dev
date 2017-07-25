#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "srn_if.h"

static struct srn_setup srn_setup;
static struct srn_interface ut_srn;

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
	nid_log_info("srn ut module start ...");

	srn_initialization(&ut_srn, &srn_setup);

	nid_log_info("srn ut module end...");
	nid_log_close();

	return 0;
}
