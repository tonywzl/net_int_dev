#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "cxac_if.h"

static struct cxac_interface ut_cxac;

int
main()
{
	struct hs_interface;
	struct cxac_setup setup;
	
	nid_log_open();
	nid_log_info("cxac start ...");

	memset(&setup, 0, sizeof(setup));
	cxac_initialization(&ut_cxac, &setup);

	nid_log_info("cxac end...");
	nid_log_close();

	return 0;
}
