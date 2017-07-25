#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "cxag_if.h"

static struct cxag_setup cxag_setup;
static struct cxag_interface ut_cxag;

struct cxa_interface;
struct cxa_setup;
int
cxa_initialization(struct cxa_interface *cxa_p, struct cxa_setup *setup)
{
	assert(cxa_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("cxag ut module start ...");

	cxag_initialization(&ut_cxag, &cxag_setup);

	nid_log_info("cxag ut module end...");
	nid_log_close();

	return 0;
}
