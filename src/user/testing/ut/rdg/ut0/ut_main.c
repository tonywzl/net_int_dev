#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rdg_if.h"

static struct rdg_setup rdg_setup;
static struct rdg_interface ut_rdg;

int
main()
{
	nid_log_open();
	nid_log_info("rdg ut module start ...");

	rdg_initialization(&ut_rdg, &rdg_setup);

	nid_log_info("rdg ut module end...");
	nid_log_close();

	return 0;
}
