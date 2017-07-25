#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "nid_log.h"
#include "mpk_if.h"

static struct mpk_setup sw_setup;
static struct mpk_interface ut_nw;

int
main()
{
	nid_log_open();
	nid_log_info("mpk ut module start ...");

	mpk_initialization(&ut_nw, &sw_setup);

	nid_log_info("mpk ut module end...");
	nid_log_close();

	return 0;
}
