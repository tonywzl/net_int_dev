#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ash_if.h"
#include "asc_if.h"
#include "dsm_if.h"

static struct asc_setup asc_setup;
static struct asc_interface ut_asc;

int
ash_initialization(struct ash_interface *ash_p, struct ash_setup *setup)
{
	assert(setup);
	assert(ash_p);
	return 0;
}

int
dsm_initialization(struct dsm_interface *dsm_p, struct dsm_setup *setup)
{
	assert(dsm_p);
	assert(setup);
	return 0;
}


int
main()
{
	nid_log_open();
	nid_log_info("asc start ...");

	asc_initialization(&ut_asc, &asc_setup);

	nid_log_info("asc end...");
	nid_log_close();

	return 0;
}
