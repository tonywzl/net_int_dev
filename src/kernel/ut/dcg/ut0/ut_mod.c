#include "nid_log.h" 
#include "chan_if.h"
#include "dcg_if.h"

struct dcg_setup dcg_setup;
struct dcg_interface ut_dcg;
int nid_log_level = 7;

int
chan_initialization(struct chan_interface *chan_p, struct chan_setup *setup)
{
	return 0;
}

static int
ut_mod_init(void)
{
	nid_log_info("this is dcg ut module");
	dcg_initialization(&ut_dcg, &dcg_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("dcg ut module exit");
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
