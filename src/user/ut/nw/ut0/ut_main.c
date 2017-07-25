#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "nw_if.h"

static struct nw_interface ut_nw;

void*
module_create_channel(void *module, int sfd)
{
	assert(module);
	assert(sfd);
	return NULL;
}

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
	nid_log_info("network start ...");

	nw_initialization(&ut_nw, NULL);

	nid_log_info("network end...");
	nid_log_close();

	return 0;
}
