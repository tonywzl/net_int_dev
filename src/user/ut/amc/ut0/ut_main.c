#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "amc_if.h"

static struct amc_interface ut_amc;

int
main()
{
	struct hs_interface;
	struct amc_setup setup;
	
	nid_log_open();
	nid_log_info("amc start ...");

	memset(&setup, 0, sizeof(setup));
	amc_initialization(&ut_amc, &setup);

	nid_log_info("amc end...");
	nid_log_close();

	return 0;
}
