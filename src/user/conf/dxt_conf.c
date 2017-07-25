#include "lstn_if.h"
#include "ini_if.h"
#include "tx_shared.h"

#define DESCRIPTION_TYPE	"Data Exchange Transfer Module."
#define DESCRIPTION_REQ_SIZE	"Size of the control message request."
#define DESCRIPTION_DS_NAME	"Name of the data stream."

static int d_dxt_reqsz = UMSG_TX_HEADER_LEN;

struct ini_key_desc dxt_keys[] = {
	{"type",		KEY_TYPE_STRING,	"dxt",			1,	DESCRIPTION_TYPE},
	{"req_size",            KEY_TYPE_INT,           &d_dxt_reqsz,           0,	DESCRIPTION_REQ_SIZE},
	{"ds_name",             KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_DS_NAME},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_dxt_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)dxt_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
