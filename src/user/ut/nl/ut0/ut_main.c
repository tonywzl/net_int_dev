#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "nl_if.h"

static struct nl_setup nl_setup;
static struct nl_interface ut_nl;

void
module_do_channel(void *module, void *data)
{
	assert(module);
	assert(data);
}

int
main()
{
	nid_log_open();
	nid_log_info("network start ...");

	nl_initialization(&ut_nl, &nl_setup);

	nid_log_info("network end...");
	nid_log_close();

	return 0;
}
