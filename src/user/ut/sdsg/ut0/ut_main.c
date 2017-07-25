#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "sdsg_if.h"

static struct sdsg_setup sdsg_setup;
static struct sdsg_interface ut_sdsg;

struct sds_interface;
struct sds_setup;
int
sds_initialization(struct sds_interface *sds_p, struct sds_setup *setup)
{
	assert(sds_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("sdsg ut module start ...");

	sdsg_initialization(&ut_sdsg, &sdsg_setup);

	nid_log_info("sdsg ut module end...");
	nid_log_close();

	return 0;
}
