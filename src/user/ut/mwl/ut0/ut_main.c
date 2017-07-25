#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "mwl_if.h"

static struct mwl_setup mwl_setup;
static struct mwl_interface ut_nw;
int nid_log_level = 7;

int
main()
{
	nid_log_open();
	nid_log_info("mwl ut module start ...");

	mwl_setup.size = 100;
	mwl_initialization(&ut_nw, &mwl_setup);

	nid_log_info("mwl ut module end...");
	nid_log_close();

	return 0;
}
