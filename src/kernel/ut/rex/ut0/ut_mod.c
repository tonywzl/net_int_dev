#include "nid_log.h" 
#include "rex_if.h"

struct rex_setup rex_setup;
struct rex_interface ut_rex;
int nid_log_level = 7;

static int
ut_mod_init(void)
{
	nid_log_info("this is rex ut module");
	rex_initialization(&ut_rex, &rex_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("rex ut module exit");
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
