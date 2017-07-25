#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Regular Device Module."
#define DESCRIPTION_NAME	"put your description here."

struct ini_key_desc rgdev_keys[] = {
	{"type",	KEY_TYPE_STRING,	"dev",		1,	DESCRIPTION_TYPE},
	{"name",	KEY_TYPE_STRING,	NULL,		0,	DESCRIPTION_NAME},
	{NULL,		KEY_TYPE_WRONG,		NULL,		0,	NULL}
};

void
conf_rgdev_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)rgdev_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
