#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "cxtg_if.h"

static struct cxtg_setup cxtg_setup;
static struct cxtg_interface ut_cxtg;

int
main()
{
	nid_log_open();
	nid_log_info("cxtg ut module start ...");

	cxtg_initialization(&ut_cxtg, &cxtg_setup);

	nid_log_info("cxtg ut module end...");
	nid_log_close();

	return 0;
}
