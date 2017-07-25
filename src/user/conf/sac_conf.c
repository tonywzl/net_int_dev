#include "lstn_if.h"
#include "ini_if.h"
#include "nid.h"

#define DESCRIPTION_TYPE	"Server Agent Channel Module."
#define DESCRIPTION_SYNC	"Whether enable sync mode."
#define DESCRIPTION_DIRECT_IO	"Whether enable direct io mode.."
#define DESCRIPTION_ALIGNMENT	"Size of request aligment."
#define DESCRIPTION_DS_NAME	"Name of the data Stream."
#define DESCRIPTION_DEV_NAME	"Name of the device."
#define DESCRIPTION_TP_NAME	"Name of the thread pool."
#define DESCRIPTION_DEV_SIZE	"Size of the device."
#define DESCRIPTION_VOLUME_UUID	"UUID of the volume."
#define DESCRIPTION_ENABLE_KILL_MYSELF	"Enable reboot system when encounter errors that cannot be handled"

static int d_sync = 1;                  // default sync mode
static int d_direct_io = 0;             // default direct_io mode
static int d_alignment = 0;             // default align size, 0 for no align
static int d_mrw_dev_size = 4;          // default mrw device size in G
static int d_enable_kill_myself = 0;	// default not enable kill_myself

struct ini_key_desc sac_keys[] = {
	{"type",		KEY_TYPE_STRING,	"sac",			1,	DESCRIPTION_TYPE},
	{"sync",                KEY_TYPE_INT,           &d_sync,                0,	DESCRIPTION_SYNC},
        {"direct_io",           KEY_TYPE_INT,           &d_direct_io,           0,	DESCRIPTION_DIRECT_IO},
        {"alignment",           KEY_TYPE_INT,           &d_alignment,           0,	DESCRIPTION_ALIGNMENT},
        {"ds_name",             KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_DS_NAME},
        {"dev_name",            KEY_TYPE_STRING,        "wrong_name",           0,	DESCRIPTION_DEV_NAME},
        {"tp_name",            	KEY_TYPE_STRING,        NID_GLOBAL_TP,          0,	DESCRIPTION_TP_NAME},
        {"dev_size",            KEY_TYPE_INT,           &d_mrw_dev_size,        0,	DESCRIPTION_DEV_SIZE},
        {"volume_uuid",         KEY_TYPE_STRING,        "0",                    0,	DESCRIPTION_VOLUME_UUID},     // For mrw, set volumn id
	{"enable_kill_myself",	KEY_TYPE_INT,		&d_enable_kill_myself,	0,	DESCRIPTION_ENABLE_KILL_MYSELF},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_sac_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)sac_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
