#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"

static struct lck_setup lck_setup;
static struct lck_interface ut_lck;

int
main()
{
	nid_log_open();
	nid_log_info("lck ut module start ...");

	lck_initialization(&ut_lck, &lck_setup);

	nid_log_info("lck ut module end...");
	nid_log_close();

	return 0;
}
