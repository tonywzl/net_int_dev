#include "nid.h"
#include "agent.h"
#include "ini_if.h"
#include "lstn_if.h"
#include "nid_log.h"

#define DESCRIPTION_TYPE	"Agent Service Channel Module."
#define DESCRIPTION_DSIZE	"Device size unit Gigbyte."
#define DESCRIPTION_DEVNAME	"Name of the device."
#define DESCRIPTION_MINOR	"Minor of the device."
#define DESCRIPTION_IP		"Ip of nidserver."
#define DESCRIPTION_ACCESS	"Access authority of the channel."
#define DESCRIPTION_WR_TIMEWAIT	"Wait time for housekeeping."
#define DESCRIPTION_IW_HOOK	"Hook after initialization."
#define DESCRIPTION_WR_HOOK	"Hook when switch to recover from working status."
#define DESCRIPTION_RW_HOOK	"Hook when switch to working from recover status."
#define DESCRIPTION_UP_HOOK	"Hook when do upgrade."

static int d_dsize = 0;
static int d_minor = 0;
static int d_wr_tw = 5;

struct ini_key_desc asc_keys[] = {
	{"type",		KEY_TYPE_STRING,	"asc",			1,	DESCRIPTION_TYPE},
	{"dsize",               KEY_TYPE_INT,           &d_dsize,               0,	DESCRIPTION_DSIZE},     // device size, unit Gigbyte
        {"devname",             KEY_TYPE_STRING,        AGENT_NONEXISTED_DEV,   0,	DESCRIPTION_DEVNAME},
        {"minor",               KEY_TYPE_INT,           &d_minor,               0,	DESCRIPTION_MINOR},
        {"ip",                  KEY_TYPE_STRING,        AGENT_NONEXISTED_IP,    0,	DESCRIPTION_IP},
        {"access",              KEY_TYPE_STRING,        "rw",                   0,	DESCRIPTION_ACCESS},
        {"wr_timewait",         KEY_TYPE_INT,           &d_wr_tw,               0,	DESCRIPTION_WR_TIMEWAIT},
        {"iw_hook",             KEY_TYPE_STRING,        NULL,                   0,	DESCRIPTION_IW_HOOK},
        {"wr_hook",             KEY_TYPE_STRING,        NULL,                   0,	DESCRIPTION_WR_HOOK},
        {"rw_hook",             KEY_TYPE_STRING,        NULL,                   0,	DESCRIPTION_RW_HOOK},
        {"up_hook",             KEY_TYPE_STRING,        NULL,                   0,	DESCRIPTION_UP_HOOK},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_asc_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)asc_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
