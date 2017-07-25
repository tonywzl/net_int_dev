#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "node_if.h"

static struct node_setup node_setup;
static struct node_interface ut_node;

int
main()
{
	nid_log_open();
	nid_log_info("node ut module start ...");

	node_initialization(&ut_node, &node_setup);

	nid_log_info("node ut module end...");
	nid_log_close();

	return 0;
}
