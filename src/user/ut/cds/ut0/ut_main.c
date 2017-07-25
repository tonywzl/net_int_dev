#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "nid.h"
#include "cds_if.h"

static struct cds_interface ut_cds;

int
main()
{
	struct hs_interface;
	struct cds_setup setup;
	
	nid_log_open();
	nid_log_info("ut cds start ...");

	memset(&setup, 0, sizeof(setup));
	cds_initialization(&ut_cds, &setup);

	nid_log_info("ut cds end...");
	nid_log_close();

	return 0;
}
