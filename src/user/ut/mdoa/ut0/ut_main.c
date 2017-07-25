#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid.h"
#include "nid_log.h"
#include "mdoa_if.h"

static struct mdoa_interface ut_cc;

int
main()
{
	struct mdoa_setup setup = {"127.0.0.1", NID_SERVICE_PORT, NULL};

	nid_log_open();
	nid_log_info("ut_mdoa start ...");

	mdoa_initialization(&ut_cc, &setup);

	nid_log_info("ut_mdoa end...");
	nid_log_close();

	return 0;
}
