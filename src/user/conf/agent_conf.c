/*
 * agent_conf.c
 */

#include "nid.h"
#include "agent.h"
#include "ini_if.h"
#include "assert.h"

void
agent_generate_conf_tmplate(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	assert(key_set_head);
	assert(lstn_p);

	conf_global_registration(key_set_head, lstn_p);
	conf_asc_registration(key_set_head, lstn_p);
}

