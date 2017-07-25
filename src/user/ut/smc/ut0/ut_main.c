#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "smc_if.h"

static struct smc_interface ut_smc;

int
main()
{
	struct hs_interface;
	struct smc_setup setup;
	
	nid_log_open();
	nid_log_info("smc start ...");

	memset(&setup, 0, sizeof(setup));
	smc_initialization(&ut_smc, &setup);

	nid_log_info("smc end...");
	nid_log_close();

	return 0;
}
