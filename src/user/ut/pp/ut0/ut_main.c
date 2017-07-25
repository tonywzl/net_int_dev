#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "pp_if.h"

static struct pp_setup pp_setup;
static struct pp_interface ut_pp;
int nid_log_level = LOG_WARNING;

int
main()
{
	struct pp_page_node *np;
#ifdef DEBUG_LEVEL
	nid_log_level = DEBUG_LEVEL;
#endif
	nid_log_open();
	nid_log_info("pp ut module start ...");

	pp_setup.page_size = 2;
	pp_setup.pool_size = 128;
	pp_initialization(&ut_pp, &pp_setup);
	np = ut_pp.pp_op->pp_get_node(&ut_pp);
	ut_pp.pp_op->pp_free_node(&ut_pp, np);

	nid_log_debug("poolsz: %u", ut_pp.pp_op->pp_get_poolsz(&ut_pp));
	nid_log_debug("pagesz: %u", ut_pp.pp_op->pp_get_pagesz(&ut_pp));
	nid_log_debug("pageszshift: %u", ut_pp.pp_op->pp_get_pageszshift(&ut_pp));
	nid_log_info("pp ut module end...");
	nid_log_close();

	return 0;
}
