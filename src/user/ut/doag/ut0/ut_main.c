#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "doag_if.h"

static struct doag_setup doag_setup;
static struct doag_interface ut_doag;

struct doa_interface;
struct doa_setup;
int
doa_initialization(struct doa_interface *doa_p, struct doa_setup *setup)
{
	assert(doa_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("doag ut module start ...");

	doag_initialization(&ut_doag, &doag_setup);

	nid_log_info("doag ut module end...");
	nid_log_close();

	return 0;
}
