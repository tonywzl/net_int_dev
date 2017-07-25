/*
 * XXX.c
 * 	Implementation of  XXX Module
 */

#include <stdlib.h>
#include "nid_log.h"
#include "XXX_if.h"

struct XXX_private {
};

struct XXX_operations XXX_op = {

};

int
XXX_initialization(struct XXX_interface *XXX_p, struct XXX_setup *setup)
{
	char *log_header = "XXX_initialization";
	struct XXX_private *priv_p;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);
	XXX_p->s_op = &XXX_op;
	priv_p = calloc(1, sizeof(*priv_p));
	XXX_p->s_private = priv_p;

	return 0;
}
