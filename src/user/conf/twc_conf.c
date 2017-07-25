#include "lstn_if.h"
#include "ini_if.h"
#include "nid.h"

#define DESCRIPTION_TYPE	"Write Through Cache Mode."
#define DESCRIPTION_TP_NAME	"Name of the thread pool."
#define DESCRIPTION_DO_FP	"Whether enable finger point mode."

static int d_do_fp = 1;

struct ini_key_desc twc_keys[] = {
	{"type",		KEY_TYPE_STRING,	"twc",			1,	DESCRIPTION_TYPE},
	{"tp_name",		KEY_TYPE_STRING,	NID_GLOBAL_TP,		0,	DESCRIPTION_TP_NAME},
	{"do_fp",               KEY_TYPE_INT,           &d_do_fp,               0,	DESCRIPTION_DO_FP},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_twc_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)twc_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
