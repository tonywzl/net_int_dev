#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "olap_if.h"

static struct olap_setup olap_setup;
static struct olap_interface ut_olap;

int
main()
{
	nid_log_open();
	nid_log_info("olap ut module start ...");

	olap_initialization(&ut_olap, &olap_setup);

	nid_log_info("olap ut module end...");
	nid_log_close();

	return 0;
}
