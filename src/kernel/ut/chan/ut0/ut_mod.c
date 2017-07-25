#include "nid_log.h" 
#include "chan_if.h"

struct chan_setup chan_setup;
struct chan_interface ut_chan;
int nid_log_level = 7;

static int
ut_mod_init(void)
{
	nid_log_info("this is chan ut module");
	chan_initialization(&ut_chan, &chan_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("chan ut module exit");
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
