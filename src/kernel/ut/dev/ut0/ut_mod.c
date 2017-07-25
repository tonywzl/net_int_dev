#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>

#include "nid.h"
#include "nid_log.h" 
#include "nl_if.h" 
#include "mpk_if.h"
#include "mq_if.h"
#include "dev_if.h"

int nid_log_level = LOG_DEBUG;
int part_shift = 4;
struct dev_interface *nid_dev_p;

const struct block_device_operations nid_fops;

void nid_end_io(struct request *req)
{
	return;
}

static int
ut_mod_init(void)
{
	struct dev_setup dev_setup;
	nid_log_notice("this is dev ut module");
	nid_dev_p = kcalloc(1, sizeof(*nid_dev_p), GFP_KERNEL);
	dev_initialization(nid_dev_p, &dev_setup);
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_notice("dev ut module exit");
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)

MODULE_DESCRIPTION("ut deriver module");
MODULE_LICENSE("GPL");
