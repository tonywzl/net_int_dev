/*
 * XXX.c
 * 	Implementation of XXX Module 
 */

#include <linux/list.h>
#include <linux/slab.h>

#include "nid_log.h"
#include "XXX_if.h"

struct XXX_private {
};

static struct XXX_operations XXX_op = {
};

int
XXX_initialization(struct XXX_interface *XXX_p, struct XXX_setup *setup)
{
	char *log_header = "XXX_initialization";
	struct XXX_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	XXX_p->XXX_op = &XXX_op;
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
	XXX_p->XXX_private = priv_p;
	return 0;
}
