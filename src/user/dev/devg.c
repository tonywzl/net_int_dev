/*
 * devg.c
 * 	Implementation of Device Guardian  Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "rgdevg_if.h"
#include "devg_if.h"
#include "dev_if.h"

struct devg_private {
	struct ini_interface	*p_ini;
	struct rgdevg_interface	*p_rgdevg;
};

static struct dev_interface *
devg_search_and_create(struct devg_interface *devg_p, char *dev_name)
{
	struct devg_private *priv_p = (struct devg_private *)devg_p->dg_private;
	struct rgdevg_interface *rgdevg_p = priv_p->p_rgdevg;
	struct dev_interface *dev = NULL;
	struct dev_setup dev_setup;
	void *ret_handle;

	if (rgdevg_p) {
		ret_handle = rgdevg_p->dg_op->dg_search_and_create(rgdevg_p, dev_name);
		if (ret_handle){
			dev = calloc(1, sizeof(*dev));
			dev_setup.type = DEV_TYPE_RG;
			dev_setup.real_dev = ret_handle;
			dev_initialization(dev, &dev_setup);
			return dev;
		}
	}


	return NULL;
}

struct devg_operations devg_op = {
	.dg_search_and_create = devg_search_and_create,
};

int
devg_initialization(struct devg_interface *devg_p, struct devg_setup *setup)
{
	char *log_header = "devg_initialization";
	struct devg_private *priv_p;
	struct rgdevg_setup rgdevg_setup;
	struct rgdevg_interface *rgdevg_p;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);
	devg_p->dg_op = &devg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	devg_p->dg_private = priv_p;

	priv_p->p_ini = setup->ini;

	rgdevg_p = calloc(1, sizeof(*rgdevg_p));
	priv_p->p_rgdevg = rgdevg_p;
	rgdevg_setup.ini = priv_p->p_ini;
	rgdevg_initialization(rgdevg_p, &rgdevg_setup);

	return 0;
}
