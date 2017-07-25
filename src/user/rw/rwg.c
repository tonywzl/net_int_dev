/*
 * rwg.c
 *	Implementation of Read Write Guardian Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "rwg_if.h"
#include "mrwg_if.h"
#include "drwg_if.h"

struct rwg_private {
	struct mrwg_interface	*p_mrwg;
	struct drwg_interface	*p_drwg;
};

static struct mrwg_interface *
rwg_get_mrwg(struct rwg_interface *rwg_p)
{
	struct rwg_private *priv_p = (struct rwg_private *)rwg_p->rwg_private;

	return priv_p->p_mrwg;
}

static struct drwg_interface *
rwg_get_drwg(struct rwg_interface *rwg_p)
{
	struct rwg_private *priv_p = (struct rwg_private *)rwg_p->rwg_private;

	return priv_p->p_drwg;
}

static struct rw_interface *
rwg_search(struct rwg_interface *rwg_p, char *rw_name)
{
	struct rwg_private *priv_p = (struct rwg_private *)rwg_p->rwg_private;
	struct mrwg_interface *mrwg_p = priv_p->p_mrwg;
	struct drwg_interface *drwg_p = priv_p->p_drwg;
	struct rw_interface *rw_p = NULL;

	if (mrwg_p)
		rw_p = mrwg_p->rwg_op->rwg_search_mrw(mrwg_p, rw_name);

	if (!rw_p && drwg_p)
		rw_p = drwg_p->rwg_op->rwg_search_drw(drwg_p, rw_name);

	return rw_p;
}

static struct rw_interface *
rwg_search_and_create(struct rwg_interface *rwg_p, char *rw_name)
{
	struct rwg_private *priv_p = (struct rwg_private *)rwg_p->rwg_private;
	struct mrwg_interface *mrwg_p = priv_p->p_mrwg;
	struct drwg_interface *drwg_p = priv_p->p_drwg;
	struct rw_interface *rw_p = NULL;

	if (mrwg_p)
		rw_p = mrwg_p->rwg_op->rwg_search_and_create_mrw(mrwg_p, rw_name);

	if (!rw_p && drwg_p)
		rw_p = drwg_p->rwg_op->rwg_search_and_create_drw(drwg_p, rw_name);

	return rw_p;
}

struct rwg_operations rwg_op = {
	.rwg_get_mrwg = rwg_get_mrwg,
	.rwg_get_drwg = rwg_get_drwg,
	.rwg_search = rwg_search,
	.rwg_search_and_create = rwg_search_and_create,
};

int
rwg_initialization(struct rwg_interface *rwg_p, struct rwg_setup *setup)
{
	char *log_header = "rwg_initialization";
	struct rwg_private *priv_p;

	nid_log_warning("%s: start ...", log_header);
	rwg_p->rwg_op = &rwg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	rwg_p->rwg_private = priv_p;

	if (setup->support_mrw) {
		struct mrwg_setup mrwg_setup;
		priv_p->p_mrwg = x_calloc(1, sizeof(*priv_p->p_mrwg));
		mrwg_setup.ini = setup->ini;
		mrwg_setup.allocator = setup->allocator;
		mrwg_setup.fpn = setup->fpn;
		mrwg_initialization(priv_p->p_mrwg, &mrwg_setup);
	}

	if (setup->support_drw) {
		struct drwg_setup drwg_setup;
		priv_p->p_drwg = x_calloc(1, sizeof(*priv_p->p_drwg));
		drwg_setup.ini = setup->ini;
		drwg_setup.allocator = setup->allocator;
		drwg_setup.fpn = setup->fpn;
		drwg_initialization(priv_p->p_drwg, &drwg_setup);
	}

	return 0;
}
