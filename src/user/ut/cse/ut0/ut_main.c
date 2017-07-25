#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "cdn_if.h"
#include "cse_if.h"

static struct cse_setup cse_setup;
static struct cse_interface ut_cse;

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
cdn_initialization(struct cdn_interface *cdn_p, struct cdn_setup *setup)
{
	assert(cdn_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("cse ut module start ...");

	cse_initialization(&ut_cse, &cse_setup);

	nid_log_info("cse ut module end...");
	nid_log_close();

	return 0;
}
