#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "cdn_if.h"

static struct cdn_setup cdn_setup;
static struct cdn_interface ut_cdn;

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
	nid_log_info("cdn ut module start ...");

	cdn_initialization(&ut_cdn, &cdn_setup);

	nid_log_info("cdn ut module end...");
	nid_log_close();

	return 0;
}
