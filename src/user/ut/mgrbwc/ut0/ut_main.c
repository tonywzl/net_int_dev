#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid.h"
#include "nid_log.h"
#include "mgrbwc_if.h"

static struct mgrbwc_interface ut_cc;

int
main()
{
	struct mgrbwc_setup setup = {"127.0.0.1", NID_SERVICE_PORT, NULL};
	
	nid_log_open();
	nid_log_info("ut_mgrbwc start ...");

	mgrbwc_initialization(&ut_cc, &setup);

	nid_log_info("ut_mgrbwc end...");
	nid_log_close();

	return 0;
}
