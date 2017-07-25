#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "doa_if.h"

static struct doa_setup doa_setup;
static struct doa_interface ut_doa;

int
main()
{
	nid_log_open();
	nid_log_info("doa ut module start ...");

	doa_initialization(&ut_doa, &doa_setup);

	nid_log_info("doa ut module end...");
	nid_log_close();

	return 0;
}
