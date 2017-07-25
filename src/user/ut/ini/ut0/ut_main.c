#include <stdlib.h>
#include <stdio.h>

#include "nid_log.h"
#include "ini_if.h"

static struct ini_interface ut_ini;

int
main()
{
	nid_log_open();
	nid_log_info("ini start ...");
	
	ini_initialization(&ut_ini, NULL);

	nid_log_info("ini end...");
	nid_log_close();
	return 0;
}
