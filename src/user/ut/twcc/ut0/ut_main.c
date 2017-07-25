#include <string.h>

#include "nid_log.h"
#include "twcc_if.h"

static struct twcc_interface ut_twcc;

int
main()
{
	struct twcc_setup setup;
	
	nid_log_open();
	nid_log_info("twcc start ...");

	memset(&setup, 0, sizeof(setup));
	twcc_initialization(&ut_twcc, &setup);

	nid_log_info("twcc end...");
	nid_log_close();

	return 0;
}
