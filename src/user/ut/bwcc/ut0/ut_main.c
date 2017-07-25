#include <string.h>

#include "nid_log.h"
#include "bwcc_if.h"

static struct bwcc_interface ut_bwcc;

int
main()
{
	struct bwcc_setup setup;
	
	nid_log_open();
	nid_log_info("bwcc start ...");

	memset(&setup, 0, sizeof(setup));
	bwcc_initialization(&ut_bwcc, &setup);

	nid_log_info("bwcc end...");
	nid_log_close();

	return 0;
}
