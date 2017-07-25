#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>

#include "nid_log.h" 
#include "mpk_if.h"

struct mpk_setup mpk_setup = {.type = 0,};
struct mpk_interface ut_mpk;

static int
ut_mod_init(void)
{
	nid_log_info("mpk module ut start");
	mpk_initialization(&ut_mpk, &mpk_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("mpk module ut exit");
	ut_mpk.m_op->m_cleanup(&ut_mpk);
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
