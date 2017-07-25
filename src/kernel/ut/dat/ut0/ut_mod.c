#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>

#include "nid_log.h" 
#include "dat_if.h"

struct dat_setup dat_setup;
struct dat_interface ut_dat;
static int dat_inited = 0;

static int
ut_mod_init(void)
{
	nid_log_info("this is mn ut module");
	if (!dat_initialization(&ut_dat, NULL))
		dat_inited = 1;
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("mn ut module exit");
	if (dat_inited)
		ut_dat.d_op->d_exit(&ut_dat);
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
