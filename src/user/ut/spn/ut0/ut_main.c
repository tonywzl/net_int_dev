#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "spn_if.h"

static struct spn_setup spn_setup;
static struct spn_interface ut_spn;

int
main()
{
	nid_log_open();
	nid_log_info("spn ut module start ...");

	spn_initialization(&ut_spn, &spn_setup);

	nid_log_info("spn ut module end...");
	nid_log_close();

	return 0;
}

