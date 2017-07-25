#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "wcc_if.h"

static struct wcc_interface ut_wcc;

int
main()
{
	struct hs_interface;
	struct wcc_setup setup;
	
	nid_log_open();
	nid_log_info("wcc start ...");

	memset(&setup, 0, sizeof(setup));
	wcc_initialization(&ut_wcc, &setup);

	nid_log_info("wcc end...");
	nid_log_close();

	return 0;
}
