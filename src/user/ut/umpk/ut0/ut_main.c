#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "nid_log.h"
#include "umpk_if.h"

static struct umpk_setup sw_setup;
static struct umpk_interface ut_nw;

int
main()
{
	nid_log_open();
	nid_log_info("umpk ut module start ...");

	umpk_initialization(&ut_nw, &sw_setup);

	nid_log_info("umpk ut module end...");
	nid_log_close();

	return 0;
}
