#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "nid.h"
#include "cds_if.h"
#include "sds_if.h"
#include "ds_if.h"

static struct ds_interface ut_ds;
static struct ds_setup ut_setup = {DS_TYPE_SPLIT, NULL}; 

int
cds_initialization(struct cds_interface *cds_p, struct cds_setup *setup)
{
	assert(cds_p && setup);
	return 0;
}

int
sds_initialization(struct sds_interface *sds_p, struct sds_setup *setup)
{
	assert(sds_p && setup);
	return 0;
}

int
main()
{
	struct hs_interface;
	
	nid_log_open();
	nid_log_info("ds ut start ...");

	ds_initialization(&ut_ds, &ut_setup);

	nid_log_info("ds ut end...");
	nid_log_close();

	return 0;
}
