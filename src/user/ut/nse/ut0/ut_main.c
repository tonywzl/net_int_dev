#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "brn_if.h"
#include "lck_if.h"
#include "rtree_if.h"
#include "nse_if.h"

static struct nse_setup nse_setup;
static struct nse_interface ut_nse;

int
brn_initialization(struct brn_interface *brn_p, struct brn_setup *setup)
{
	assert(brn_p && setup);
	return 0;
}

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
rtree_initialization(struct rtree_interface *rtree_p, struct rtree_setup *setup)
{
	assert(rtree_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("nse ut module start ...");

	nse_initialization(&ut_nse, &nse_setup);

	nid_log_info("nse ut module end...");
	nid_log_close();

	return 0;
}
