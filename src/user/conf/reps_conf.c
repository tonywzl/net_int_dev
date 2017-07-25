#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Replication Src Module"
#define DESCRIPTION_DXA_NAME	"Name of dx active part"
#define DESCRIPTION_REPT_NAME	"Name of the coressponding rept"
#define DESCRIPTION_TP_NAME	"Name of the thread pool"
#define DESCRIPTION_VOL_UUID	"UUID of the volume"
#define DESCRIPTION_VOL_LEN	"Length of the volume, unit byte."

struct ini_key_desc reps_keys[] = {
	{"type",	KEY_TYPE_STRING,	"reps",		1,	DESCRIPTION_TYPE},
	{"dxa_name",	KEY_TYPE_STRING,	NULL,           1,	DESCRIPTION_DXA_NAME},
	{"rept_name",	KEY_TYPE_STRING,	NULL,		1,	DESCRIPTION_REPT_NAME},
	{"tp_name",	KEY_TYPE_STRING,	NULL,		1,	DESCRIPTION_TP_NAME},
	{"voluuid",	KEY_TYPE_STRING,	NULL,		1,	DESCRIPTION_VOL_UUID},
	{"vol_len",	KEY_TYPE_INT64,		NULL,		1,	DESCRIPTION_VOL_LEN},
	{NULL,		KEY_TYPE_WRONG,		NULL,		0,	NULL}
};

void
conf_reps_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)reps_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
