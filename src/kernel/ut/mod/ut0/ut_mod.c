#include <linux/module.h>
#include "dev_if.h"
#include "drv_if.h"

int
drv_initialization(struct drv_interface *drv_p, struct drv_setup *setup)
{
	return 0;
}

int
dev_initialization(struct dev_interface *dev_p, struct dev_setup *setup)
{
	return 0;
}
