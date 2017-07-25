/*
 * nw-agent.c
 * 	Implementation of Network Module -- for Agent
 */

#include "nw.h"
#include "acg_if.h"

void*
module_accept_new_channel(void *module, int sfd, char *cip)
{
	struct acg_interface *acg_p = (struct acg_interface *)module;
	return (void *)acg_p->ag_op->ag_accept_new_channel(acg_p, sfd, cip);
}

void
module_do_channel(void *module, void *data)
{
	struct acg_interface *acg_p = (struct acg_interface *)module;
	acg_p->ag_op->ag_do_channel(acg_p, data);
}
