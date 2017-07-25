#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rsm_if.h"

static struct rsm_setup rsm_setup;
static struct rsm_interface ut_rsm;

struct rs_interface;
struct rs_setup;
int
rs_initialization(struct rs_interface *rs_p, struct rs_setup *setup)
{
	assert(rs_p && setup);
	return 0;
}

struct rsn_interface;
struct rsn_setup;
int
rsn_initialization(struct rsn_interface *rsn_p, struct rsn_setup *setup)
{
	assert(rsn_p && setup);
	return 0;
}

struct rsun_interface;
struct rsun_setup;
int
rsun_initialization(struct rsun_interface *rsun_p, struct rsun_setup *setup)
{
	assert(rsun_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("rsm ut module start ...");

	rsm_initialization(&ut_rsm, &rsm_setup);

	nid_log_info("rsm ut module end...");
	nid_log_close();

	return 0;
}
