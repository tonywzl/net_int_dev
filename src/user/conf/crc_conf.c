#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE		"CAS (Content-Addressed Storage) Read Cache Module."
#define DESCRIPTION_CACHE_DEVICE	"Read cache directory."
#define DESCRIPTION_CACHE_SIZE		"Size of the read cache."
#define DESCRIPTION_PP_NAME		"Name of the page pool."

static int d_wrong_size = -1;           // default page size for buf page size, 8M

struct ini_key_desc crc_keys[] = {
	{"type",		KEY_TYPE_STRING,	"crc",			1,	DESCRIPTION_TYPE},
	{"cache_device",        KEY_TYPE_STRING,        "Wrong-cache-device",   0,	DESCRIPTION_CACHE_DEVICE},
        {"cache_size",          KEY_TYPE_INT,           &d_wrong_size,          0,	DESCRIPTION_CACHE_SIZE},
	{"pp_name",		KEY_TYPE_STRING,	NULL,			1,	DESCRIPTION_PP_NAME},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_crc_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)crc_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
