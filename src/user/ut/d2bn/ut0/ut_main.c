#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "d2bn_if.h"

static struct d2bn_setup d2bn_setup;
static struct d2bn_interface ut_d2bn;

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
	nid_log_info("d2bn ut module start ...");

	d2bn_initialization(&ut_d2bn, &d2bn_setup);

	nid_log_info("d2bn ut module end...");
	nid_log_close();

	return 0;
}
