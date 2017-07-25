#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ppg_if.h"

static struct ppg_setup ppg_setup;
static struct ppg_interface ut_ppg;

struct pp_interface;
struct pp_setup;
int
pp_initialization(struct pp_interface *pp_p, struct pp_setup *setup)
{
	assert(pp_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("ppg ut module start ...");

	ppg_initialization(&ut_ppg, &ppg_setup);

	nid_log_info("ppg ut module end...");
	nid_log_close();

	return 0;
}
