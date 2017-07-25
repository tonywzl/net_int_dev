#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "reptc_if.h"
#include "reptcg_if.h"

int
main()
{
	nid_log_open();
	nid_log_info("reptcg ut module start ...");
	struct reptcg_interface ut_reptcg;
	struct reptcg_setup ut_setup;

	reptcg_initialization(&ut_reptcg, &ut_setup);

	nid_log_info("reptcg ut module end...");
	nid_log_close();

	return 0;
}
