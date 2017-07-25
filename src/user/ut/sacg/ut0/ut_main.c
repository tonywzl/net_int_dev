#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ini_if.h"
#include "sac_if.h"

static struct sac_setup sac_setup;
static struct sac_interface ut_sac;

struct ini_key_desc* service_keys_ary[] = {
	NULL,
};

struct ini_section_desc service_sections[] = {
	{NULL,          -1,     NULL},
};

int
ini_initialization(struct ini_interface *ini_p, struct ini_setup *setup)
{
	assert(ini_p && setup);
	return 0;
}

int
sac_initialization(struct sac_interface *sac_p, struct sac_setup *setup)
{
	assert(sac_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("sac ut module start ...");

	sac_initialization(&ut_sac, &sac_setup);

	nid_log_info("sac ut module end...");
	nid_log_close();

	return 0;
}
