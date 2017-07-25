#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "d3an_if.h"

static struct d3an_setup d3an_setup;
static struct d3an_interface ut_d3an;

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
	nid_log_info("d3an ut module start ...");

	d3an_initialization(&ut_d3an, &d3an_setup);

	nid_log_info("d3an ut module end...");
	nid_log_close();

	return 0;
}
