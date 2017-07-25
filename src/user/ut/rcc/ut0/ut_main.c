#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rcc_if.h"

static struct rcc_interface ut_rcc;

int
main()
{
	struct hs_interface;
	struct rcc_setup setup;
	
	nid_log_open();
	nid_log_info("rcc start ...");

	memset(&setup, 0, sizeof(setup));
	rcc_initialization(&ut_rcc, &setup);

	nid_log_info("rcc end...");
	nid_log_close();

	return 0;
}
