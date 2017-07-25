#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid.h"
#include "nid_log.h"
#include "mgrrc_if.h"

static struct mgrrc_interface ut_cc;

int
main()
{
	struct mgrrc_setup setup = {"127.0.0.1", NID_SERVICE_PORT, NULL};
	
	nid_log_open();
	nid_log_info("ut_mgrrc start ...");

	mgrrc_initialization(&ut_cc, &setup);

	nid_log_info("ut_mgrrc end...");
	nid_log_close();

	return 0;
}
