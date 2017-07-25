#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "cxt_if.h"

static struct cxt_setup cxt_setup;
static struct cxt_interface ut_cxt;

int
main()
{
	nid_log_open();
	nid_log_info("cxt ut module start ...");

	cxt_initialization(&ut_cxt, &cxt_setup);

	nid_log_info("cxt ut module end...");
	nid_log_close();

	return 0;
}
