#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "d1an_if.h"

static struct d1an_setup d1an_setup;
static struct d1an_interface ut_d1an;

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
	nid_log_info("d1an ut module start ...");

	d1an_initialization(&ut_d1an, &d1an_setup);

	nid_log_info("d1an ut module end...");
	nid_log_close();

	return 0;
}
