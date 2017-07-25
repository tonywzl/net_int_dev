#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ash_if.h"

static struct ash_interface ut_ash;

int
main()
{
	struct ash_setup setup;
	
	nid_log_open();
	nid_log_info("Hand Shake start ...");

	memset(&setup, 0, sizeof(setup));
	ash_initialization(&ut_ash, &setup);

	nid_log_info("Hand Shake end...");
	nid_log_close();

	return 0;
}
