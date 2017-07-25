#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rsun_if.h"

static struct rsun_setup rsun_setup;
static struct rsun_interface ut_rsun;

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
	nid_log_info("rsun ut module start ...");

	rsun_initialization(&ut_rsun, &rsun_setup);

	nid_log_info("rsun ut module end...");
	nid_log_close();

	return 0;
}
