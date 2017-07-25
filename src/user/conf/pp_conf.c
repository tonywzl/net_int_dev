#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Page Pool Module."
#define DESCRIPTION_POOL_SIZE	"Number of pages in Page pool."
#define DESCRIPTION_PAGE_SIZE	"Size of per page."

static int d_pp_poolsz = 512;           // default pool size for buf pool, number of pages
static int d_pp_pagesz = 8;             // default page size for buf page size, 8M

struct ini_key_desc pp_keys[] = {
	{"type",		KEY_TYPE_STRING,	"pp",			1,	DESCRIPTION_TYPE},
	{"pool_size",           KEY_TYPE_INT,           &d_pp_poolsz,           0,	DESCRIPTION_POOL_SIZE},
        {"page_size",           KEY_TYPE_INT,           &d_pp_pagesz,           0,	DESCRIPTION_PAGE_SIZE},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_pp_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)pp_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
