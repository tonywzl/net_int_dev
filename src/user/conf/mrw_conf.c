#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Meta Server Read Write Module."
#define DESCRIPTION_DEV_SIZE	"Size of the device."
#define DESCRIPTION_VOLUME_UUID	"UUID of the volume."

static int d_mrw_dev_size = 4;		// default mrw device size in G

struct ini_key_desc mrw_keys[] = {
	{"type",		KEY_TYPE_STRING,	"mrw",			1,	DESCRIPTION_TYPE},
	{"dev_size",            KEY_TYPE_INT,           &d_mrw_dev_size,	0,	DESCRIPTION_DEV_SIZE},
	{"volume_uuid",         KEY_TYPE_STRING,	"0",			0,	DESCRIPTION_VOLUME_UUID},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_mrw_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)mrw_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
