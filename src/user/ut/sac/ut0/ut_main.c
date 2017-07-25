#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "cds_if.h"
#include "ds_if.h"
#include "sac_if.h"
#include "sah_if.h"

static struct sac_interface ut_sac;

int
ds_initialization(struct ds_interface *ds_p, struct ds_setup *setup)
{
	nid_log_debug("this is dummy ds_initialization (%p, %p)",
		ds_p, setup);
	return 0;
}

int
sah_initialization(struct sah_interface *sah_p, struct sah_setup *setup)
{
	assert(sah_p);
	assert(setup);
	return 0; 
}

int
cds_initialization(struct cds_interface *cds_p, struct cds_setup *setup)
{
	assert(cds_p);
	assert(setup);
	return 0;
}

int
main()
{
	struct hs_interface;
	struct sac_setup setup;
	
	nid_log_open();
	nid_log_info("sac ut start ...");

	memset(&setup, 0, sizeof(setup));
	sac_initialization(&ut_sac, &setup);

	nid_log_info("sac ut end...");
	nid_log_close();

	return 0;
}
