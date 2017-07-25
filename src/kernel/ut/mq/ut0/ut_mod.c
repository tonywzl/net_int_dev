#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>

#include "nid_log.h" 
#include "mq_if.h"

struct mq_setup mq_setup = {.size = 1024,};
struct mq_interface ut_mq;

static int
ut_mod_init(void)
{
	nid_log_info("this is mn ut module");
	mq_initialization(&ut_mq, &mq_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("mn ut module exit");
	ut_mq.m_op->m_exit(&ut_mq);
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)
