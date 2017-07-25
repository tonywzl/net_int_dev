#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bwccg_if.h"

static struct bwccg_setup bwccg_setup;
static struct bwccg_interface ut_bwccg;

struct bwcc_interface;
struct bwcc_setup;
int
bwcc_initialization(struct bwcc_interface *bwcc_p, struct bwcc_setup *setup)
{
	assert(bwcc_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("bwccg ut module start ...");

	bwccg_initialization(&ut_bwccg, &bwccg_setup);

	nid_log_info("bwccg ut module end...");
	nid_log_close();

	return 0;
}
