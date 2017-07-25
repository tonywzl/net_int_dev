#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPR			"put your description here."
#define DESCRIPTION_DEV_LIST			"put your description here."
#define DESCRIPTION_SIMULATE_ASYNC		"put your description here."
#define DESCRIPTION_SIMULATE_DELAY		"put your description here."
#define DESCRIPTION_SIMULATE_DELAY_MIN_GAP	"put your description here."
#define DESCRIPTION_SIMULATE_DELAY_MAX_GAP	"put your description here."
#define DESCRIPTION_SIMULATE_DELAY_TIME_US	"put your description here."

static int d_simulate_async = 0;
static int d_simulate_delay = 0;
static int d_simulate_delay_min_gap = 1;
static int d_simulate_delay_max_gap = 10;
static int d_simulate_delay_time_us = 100000;

/*
 * simulate_delay: 0 for non-delay, 1 for read delay, 2 for write delay, 3 for read-write delay
 * dev_list: list of drw name delimited by ';'	
 */
struct ini_key_desc sarw_keys[] = {
	{"type",			KEY_TYPE_STRING,	"sarw",				1,	DESCRIPTION_TYPR},
	{"dev_list",			KEY_TYPE_STRING,	NULL,				0,	DESCRIPTION_DEV_LIST},
	{"simulate_async",		KEY_TYPE_INT,		&d_simulate_async,		0,	DESCRIPTION_SIMULATE_ASYNC},
	{"simulate_delay",		KEY_TYPE_INT,		&d_simulate_delay,		0,	DESCRIPTION_SIMULATE_DELAY},
	{"simulate_delay_min_gap",	KEY_TYPE_INT,		&d_simulate_delay_min_gap,	0,	DESCRIPTION_SIMULATE_DELAY_MIN_GAP},
	{"simulate_delay_max_gap",	KEY_TYPE_INT,		&d_simulate_delay_max_gap,	0,	DESCRIPTION_SIMULATE_DELAY_MAX_GAP},
	{"simulate_delay_time_us",	KEY_TYPE_INT,		&d_simulate_delay_time_us,	0,	DESCRIPTION_SIMULATE_DELAY_TIME_US},
	{NULL,				KEY_TYPE_WRONG,		NULL,				0,	NULL}
};

void
conf_sarw_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)sarw_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
