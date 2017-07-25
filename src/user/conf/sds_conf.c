#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Split Data Stream Module."
#define DESCRIPTION_PP_NAME	"Name of the page pool."
#define DESCRIPTION_WC_UUID	"UUID of the write cache."
#define DESCRIPTION_RC_UUID	"UUID of the read cache."

struct ini_key_desc sds_keys[] = {
	{"type",		KEY_TYPE_STRING,	"sds",			1,	DESCRIPTION_TYPE},
	{"pp_name",             KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_PP_NAME},
        {"wc_uuid",             KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_WC_UUID},
        {"rc_uuid",             KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_RC_UUID},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_sds_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)sds_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
