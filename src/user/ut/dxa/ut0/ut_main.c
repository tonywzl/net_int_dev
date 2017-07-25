#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dxa_if.h"

static struct dxa_setup dxa_setup;
static struct dxa_interface ut_dxa;

int
main()
{
	nid_log_open();
	nid_log_info("dxa ut module start ...");

	strcpy(dxa_setup.uuid, "dxa_ut_uuid");
	dxa_initialization(&ut_dxa, &dxa_setup);

	nid_log_info("dxa ut module end...");
	nid_log_close();

	return 0;
}
