#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Thread Pool Module."
#define DESCRIPTION_NUM_WORKERS	"Maxinum number of the thread."

static int d_num_workers = 8;           // default num_workers

struct ini_key_desc tp_keys[] = {
	{"type",		KEY_TYPE_STRING,	"tp",			1,	DESCRIPTION_TYPE},
        {"num_workers",         KEY_TYPE_INT,           &d_num_workers,         0,	DESCRIPTION_NUM_WORKERS},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_tp_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)tp_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
