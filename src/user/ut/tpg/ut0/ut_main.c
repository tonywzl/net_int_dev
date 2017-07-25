#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "tpg_if.h"

static struct tpg_setup tpg_setup;
static struct tpg_interface ut_tpg;

int
main()
{
	nid_log_open();
	nid_log_info("tpg ut module start ...");

	tpg_initialization(&ut_tpg, &tpg_setup);

	nid_log_info("tpg ut module end...");
	nid_log_close();

	return 0;
}
