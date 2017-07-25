#include "nid.h"
#include "agent.h"
#include "ini_if.h"
#include "lstn_if.h"

#define DESCRIPTION_TYPE	"Agent Global Module."
#define	DESCRIPTION_RETRY	"number of retry before io_error."
#define	DESCRIPTION_AG_LOG_LEVEL	"agent log level."

static int d_retry = 5;                 // default number of retry before io_error

#ifdef DEBUG_LEVEL
static int d_ag_log_level = DEBUG_LEVEL;
#else
static int d_ag_log_level = 4;
#endif

struct ini_key_desc global_keys[] = {
	{"type",		KEY_TYPE_STRING,	"global",		1,	DESCRIPTION_TYPE},
	{"retry",               KEY_TYPE_INT,           &d_retry,               0,	DESCRIPTION_RETRY},
    {"log_level",    KEY_TYPE_INT, &d_ag_log_level, 0, DESCRIPTION_AG_LOG_LEVEL},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_global_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)global_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
