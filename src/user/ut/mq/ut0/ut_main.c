#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "mq_if.h"

static struct mq_setup mq_setup;
static struct mq_interface ut_mq;

void
module_do_channel(void *module, void *data)
{
	assert(module);
	assert(data);
}

int
main()
{
	nid_log_open();
	nid_log_info("mq start ...");

	mq_setup.size = 1024;
	mq_initialization(&ut_mq, &mq_setup);

	nid_log_info("mq end...");
	nid_log_close();

	return 0;
}
