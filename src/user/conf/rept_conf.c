#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Replication Target Module"
#define DESCRIPTION_DXP_NAME	"Name of dxt"
#define DESCRIPTION_TP_NAME	"Name of the thread pool"
#define DESCRIPTION_VOL_UUID	"UUID of the volume"
#define DESCRIPTION_VOL_LEN	"Length of the volume, unit byte"

struct ini_key_desc rept_keys[] = {
	{"type",	KEY_TYPE_STRING,	"rept",		1,	DESCRIPTION_TYPE},
	{"dxp_name",	KEY_TYPE_STRING,	NULL,           1,	DESCRIPTION_DXP_NAME},
	{"tp_name",	KEY_TYPE_STRING,	NULL,           1,	DESCRIPTION_TP_NAME},
	{"voluuid",	KEY_TYPE_STRING,	NULL,		1,	DESCRIPTION_VOL_UUID},
	{"vol_len",	KEY_TYPE_INT64,		NULL,		1,	DESCRIPTION_VOL_LEN},
	{NULL,		KEY_TYPE_WRONG,		NULL,		0,	NULL}
};

void
conf_rept_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)rept_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
