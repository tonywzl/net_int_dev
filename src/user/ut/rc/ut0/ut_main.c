#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "crc_if.h"
#include "rc_if.h"

static struct rc_setup rc_setup;
static struct rc_interface ut_rc;

int
crc_initialization(struct crc_interface *crc_p, struct crc_setup *setup)
{
	assert(crc_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("rc ut module start ...");

	rc_initialization(&ut_rc, &rc_setup);

	nid_log_info("rc ut module end...");
	nid_log_close();

	return 0;
}
