#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "pp2_if.h"

static struct pp2_setup pp2_setup;
static struct pp2_interface ut_pp2;
int nid_log_level = LOG_WARNING;
char *log_header = "pp2 ut0";

int
lck_node_init(struct lck_node *ln_p)
{
	assert(ln_p);
	return 0;
}

int
lck_initialization(struct lck_interface *lck_p, struct lck_setup *setup)
{
	assert(lck_p && setup);
	return 0;
}

int
main()
{
#ifdef DEBUG_LEVEL
	nid_log_level = DEBUG_LEVEL;
#endif
	nid_log_open();
	nid_log_info("%s module start ...", log_header);

	pp2_initialization(&ut_pp2, &pp2_setup);

	nid_log_info("%s module end...", log_header);
	nid_log_close();

	return 0;
}
