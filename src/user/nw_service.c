/*
 * nw-service.c
 * 	Implementation of Network Module -- for Service
 */

#include "nw.h"
#include "scg_if.h"

void*
module_accept_new_channel(void *module, int sfd, char *cip)
{
	struct scg_interface *scg_p = (struct scg_interface *)module;
	return (void *)scg_p->sg_op->sg_accept_new_channel(scg_p, sfd, cip);
}

void
module_do_channel(void *module, void *data)
{
	struct scg_interface *scg_p = (struct scg_interface *)module;
	scg_p->sg_op->sg_do_channel(scg_p, data);
}
