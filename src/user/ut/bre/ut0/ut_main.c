#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bse_if.h"
#include "bre_if.h"

static struct bre_setup bre_setup;
static struct bre_interface ut_bre;

int
bse_initialization(struct bse_interface *bse_p, struct bse_setup *setup)
{
	assert(bse_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("bre ut module start ...");

	bre_initialization(&ut_bre, &bre_setup);

	nid_log_info("bre ut module end...");
	nid_log_close();

	return 0;
}
