#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Data Exchange Active Module."
#define DESCRIPTION_DXA_TYPE	"put your description here."
#define DESCRIPTION_IP		"IP address of dxp side."
#define DESCRIPTION_UUID	"UUID of dxp side."
#define DESCRIPTION_DXT_NAME	"UUID of the dxt"

struct ini_key_desc dxa_keys[] = {
	{"type",		KEY_TYPE_STRING,	"dxa",			1,	DESCRIPTION_TYPE},
	{"dxa_type",            KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_DXA_TYPE}, // type coulde be: mreps
	{"ip",                  KEY_TYPE_STRING,        "0.0.0.0",              0,	DESCRIPTION_IP},
	{"peer_uuid",		KEY_TYPE_STRING,	"wrong_name",		1,	DESCRIPTION_UUID},
	{"dxt_name",		KEY_TYPE_STRING,	"wrong_name",		1,	DESCRIPTION_DXT_NAME},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_dxa_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)dxa_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
