#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "adt_if.h"

static struct adt_setup adt_setup;
static struct adt_interface ut_adt;

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
	nid_log_info("adt start ...");

	adt_initialization(&ut_adt, &adt_setup);

	nid_log_info("adt end...");
	nid_log_close();

	return 0;
}
