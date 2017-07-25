#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Service Channel Guardian Module."
#define DESCRIPTION_WTP_NAME	"Name of the thread pool support for scg ."
#define DESCRIPTION_DRC_PP_NAME	"put your description here."

struct ini_key_desc scg_keys[] = {
	{"type",		KEY_TYPE_STRING,	"scg",			1,	DESCRIPTION_TYPE},
	{"wtp_name",            KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_WTP_NAME},
        {"drc_pp_name",         KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_DRC_PP_NAME},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_scg_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)scg_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
