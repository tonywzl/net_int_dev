#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ini_if.h"
#include "ascg_if.h"
#include "asc_if.h"
#include "amc_if.h"
#include "mwl_if.h"
#include "mq_if.h"

struct ascg_setup ascg_setup;
struct ascg_interface ut_ascg;

struct ini_key_desc* agent_keys_ary[] = {
	NULL,
};

struct ini_section_desc agent_sections[] = {
	{NULL,		-1,	NULL},
};

void
agent_stop()
{

}

int
amc_initialization(struct amc_interface *amc_p, struct amc_setup *setup)
{
	assert(amc_p);
	assert(setup);
	return 0;
}

int
asc_initialization(struct asc_interface *asc_p, struct asc_setup *setup)
{
	assert(asc_p);
	assert(setup);
	return 0;
}

int
ini_initialization(struct ini_interface *ini_p, struct ini_setup *setup)
{
	assert(ini_p);
	assert(setup);
	return 0;
}

int
mwl_initialization(struct mwl_interface *mwl_p, struct mwl_setup *setup)
{
	assert(mwl_p);
	assert(setup);
	return 0;
}

int
mq_initialization(struct mq_interface *mq_p, struct mq_setup *setup)
{
	assert(mq_p);
	assert(setup);
	return 0;
}

int nid_log_level = 7;

int
main()
{
	nid_log_open();
	nid_log_info("ascg start ...");

	ascg_setup.ini = NULL;
	ascg_initialization(&ut_ascg, &ascg_setup);

	nid_log_info("ascg end...");
	nid_log_close();

	return 0;
}
