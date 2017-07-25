#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "sah_if.h"

static struct sah_interface ut_sah;

int
main()
{
	struct sah_setup setup;
	
	nid_log_open();
	nid_log_info("Hand Shake start ...");

	memset(&setup, 0, sizeof(setup));
	sah_initialization(&ut_sah, &setup);

	nid_log_info("Hand Shake end...");
	nid_log_close();

	return 0;
}
