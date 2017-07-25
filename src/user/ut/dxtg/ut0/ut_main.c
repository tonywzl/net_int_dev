#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dxtg_if.h"

static struct dxtg_setup dxtg_setup;
static struct dxtg_interface ut_dxtg;

int
main()
{
	nid_log_open();
	nid_log_info("dxtg ut module start ...");

	dxtg_initialization(&ut_dxtg, &dxtg_setup);

	nid_log_info("dxtg ut module end...");
	nid_log_close();

	return 0;
}
