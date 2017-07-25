#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lstn_if.h"

static struct lstn_setup lstn_setup;
static struct lstn_interface ut_lstn;

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
	nid_log_info("lstn ut module start ...");

	lstn_initialization(&ut_lstn, &lstn_setup);

	nid_log_info("lstn ut module end...");
	nid_log_close();

	return 0;
}
