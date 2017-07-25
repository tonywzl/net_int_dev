#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dxp_if.h"

static struct dxp_setup dxp_setup;
static struct dxp_interface ut_dxp;

int
main()
{
	nid_log_open();
	nid_log_info("dxp ut module start ...");

	dxp_initialization(&ut_dxp, &dxp_setup);

	nid_log_info("dxp ut module end...");
	nid_log_close();

	return 0;
}
