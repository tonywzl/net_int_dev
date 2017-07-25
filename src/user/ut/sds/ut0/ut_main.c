#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "sds_if.h"

static struct sds_setup sds_setup;
static struct sds_interface ut_sds;

int
lck_initialization(struct lck_interface *lck_p, struct lck_setup *setup)
{
	assert(lck_p && setup);
	return 0;
}

int
lck_node_init(struct lck_node *ln_p)
{
	assert(ln_p);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("sds ut module start ...");

	sds_initialization(&ut_sds, &sds_setup);

	nid_log_info("sds ut module end...");
	nid_log_close();

	return 0;
}
