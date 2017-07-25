#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "server.h"
#include "sac_if.h"

struct sac_interface ut_sac;

int
main()
{
	nid_log_open();
	nid_log_info("sac ut start ...");

	sac_initialization(&ut_sac, NULL);

	nid_log_info("sac ut end...");
	nid_log_close();

	return 0;
}
