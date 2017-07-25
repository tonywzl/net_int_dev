#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Control Exchange Active Module."
#define DESCRIPTION_CXA_TYPE	"put your description here."
#define DESCRIPTION_IP		"Ip address of cxp side."
#define DESCRIPTION_CXT_NAME	"Name of the cxt."
#define DESCRIPTION_UUID	"UUID of cxp side."

struct ini_key_desc cxa_keys[] = {
	{"type",		KEY_TYPE_STRING,	"cxa",			1,	DESCRIPTION_TYPE},
	{"cxa_type",            KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_CXA_TYPE}, // type coulde be: mreps
	{"ip",                  KEY_TYPE_STRING,        "0.0.0.0",              1,	DESCRIPTION_IP},
	{"cxt_name",            KEY_TYPE_STRING,        "wrong_name",           1,	DESCRIPTION_CXT_NAME},
	{"peer_uuid",		KEY_TYPE_STRING,	"wrong_name",		1,	DESCRIPTION_UUID},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_cxa_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)cxa_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
