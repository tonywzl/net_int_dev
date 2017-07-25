#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>

#include "nid_log.h" 
#include "nl_if.h" 
#include "mpk_if.h"
#include "dat_if.h"
#include "mq_if.h"
#include "ddg_if.h"
#include "dcg_if.h"
#include "drg_if.h"
#include "drv_if.h"

int nid_log_level = LOG_DEBUG;
int part_shift = 4;
struct drv_interface *nid_drv_p;

int ddg_initialization(struct ddg_interface *ddg_p, struct ddg_setup *setup)
{
	return 0;
}

int drg_initialization(struct drg_interface *drg_p, struct drg_setup *setup)
{
	return 0;
}

int dcg_initialization(struct dcg_interface *dcg_p, struct dcg_setup *setup)
{
	return 0;
}

int mq_initialization(struct mq_interface *mq_p, struct mq_setup *setup)
{
	return 0;
}

int dat_initialization(struct dat_interface *dat_p, struct dat_setup *setup)
{
	return 0;
}

int mpk_initialization(struct mpk_interface *mpk_p, struct mpk_setup *setup)
{
	return 0;
}

int nl_initialization(struct nl_interface *nl_p, struct nl_setup *setup)
{
	return 0;
}

static int
ut_mod_init(void)
{
	struct drv_setup drv_setup;
	nid_log_notice("this is dev ut module");
	drv_setup.size = 1024;
	nid_drv_p = kcalloc(1, sizeof(*nid_drv_p), GFP_KERNEL);
	drv_initialization(nid_drv_p, &drv_setup);
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
MODULE_LICENSE("Non-GPL");
