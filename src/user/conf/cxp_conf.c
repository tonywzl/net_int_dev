#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Control Exchange Passive Module."
#define DESCRIPTION_CXP_TYPE	"put your description here."
#define DESCRIPTION_CXT_NAME	"Name of the cxt."
#define DESCRIPTION_UUID	"UUID of cxa side."

struct ini_key_desc cxp_keys[] = {
	{"type",		KEY_TYPE_STRING,	"cxp",			1,	DESCRIPTION_TYPE},
	{"cxp_type",            KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_CXP_TYPE}, // type coulde be: mrept
	{"cxt_name",            KEY_TYPE_STRING,        "wrong_name",           1,	DESCRIPTION_CXT_NAME},
	{"peer_uuid",		KEY_TYPE_STRING,	"wrong_name",		1,	DESCRIPTION_UUID},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};


void
conf_cxp_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)cxp_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
