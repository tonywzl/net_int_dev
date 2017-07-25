#include "nid_log.h" 
#include "XXX_if.h"

struct XXX_setup XXX_setup;
struct XXX_interface ut_XXX;
int nid_log_level = 7;

static int
ut_mod_init(void)
{
	nid_log_info("this is XXX ut module");
	XXX_initialization(&ut_XXX, &XXX_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("XXX ut module exit");
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
