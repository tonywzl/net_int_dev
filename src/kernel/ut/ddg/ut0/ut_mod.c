#include "nid_log.h" 
#include "dev_if.h"
#include "ddg_if.h"

struct ddg_setup ddg_setup;
struct ddg_interface ut_ddg;
int nid_log_level = 7;

int
dev_initialization(struct dev_interface *dev_p, struct dev_setup *setup)
{
	return 0;
}

static int
ut_mod_init(void)
{
	nid_log_info("this is ddg ut module");
	ddg_initialization(&ut_ddg, &ddg_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("ddg ut module exit");
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
