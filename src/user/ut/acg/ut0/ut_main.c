#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ini_if.h"
#include "acg_if.h"
#include "asc_if.h"
#include "amc_if.h"

struct acg_setup acg_setup;
struct acg_interface ut_acg;

struct ini_key_desc* agent_keys_ary[] = {
	NULL,
};

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

struct mpk_interface;
struct mpk_setup;
int
mpk_initialization(struct mpk_interface *mpk_p, struct mpk_setup *setup)
{
	assert(mpk_p && setup);
	return 0;
}

struct drv_interface;
struct drv_setup;
int
drv_initialization(struct drv_interface *drv_p, struct drv_setup *setup)
{
	assert(drv_p && setup);
	return 0;
}

struct devg_interface;
struct devg_setup;
int
devg_initialization(struct devg_interface *devg_p, struct devg_setup *setup)
{
	assert(devg_p && setup);
	return 0;
}

struct ascg_interface;
struct ascg_setup;
int
ascg_initialization(struct ascg_interface *ascg_p, struct ascg_setup *setup)
{
	assert(ascg_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("acg start ...");

	acg_setup.ini = NULL;
	acg_initialization(&ut_acg, &acg_setup);

	nid_log_info("acg end...");
	nid_log_close();

	return 0;
}
