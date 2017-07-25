#include "nid_log.h" 
#include "rex_if.h"
#include "drg_if.h"

struct drg_setup drg_setup;
struct drg_interface ut_drg;
int nid_log_level = 7;

int
rex_initialization(struct rex_interface *rex_p, struct rex_setup *setup)
{
	return 0;
}

static int
ut_mod_init(void)
{
	nid_log_info("this is drg ut module");
	drg_initialization(&ut_drg, &drg_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("drg ut module exit");
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
