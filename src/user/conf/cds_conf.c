#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE	"Module type."
#define DESCRIPTION_PAGE_SIZE	"Size of per page."
#define DESCRIPTION_PAGE_NR	"Number of pages in page pool."

static int d_cds_page_size = 8;         // default page size (in unit M) of cds
static int d_cds_page_nr = 4;           // default number of page of cds

struct ini_key_desc cds_keys[] = {
	{"type",		KEY_TYPE_STRING,	"cds",			1,	DESCRIPTION_TYPE},
	{"page_size",           KEY_TYPE_INT,           &d_cds_page_size,       0,	DESCRIPTION_PAGE_SIZE},
        {"page_nr",             KEY_TYPE_INT,           &d_cds_page_nr,         0,	DESCRIPTION_PAGE_NR},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};


void
conf_cds_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)cds_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
