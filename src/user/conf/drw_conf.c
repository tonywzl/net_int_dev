#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE			"Device Read Write Module."
#define DESCRIPTION_EXPORTNAME			"Exportname of the target device."
#define DESCRIPTION_DEVICE_PROVISION		"put your description here."
#define DESCRIPTION_DEV_NAME			"Name of the Device."
#define DESCRIPTION_SIMULATE_ASYNC		"put your description here."
#define DESCRIPTION_SIMULATE_DELAY		"put your description here."
#define DESCRIPTION_SIMULATE_DELAY_MIN_GAP	"put your description here."
#define DESCRIPTION_SIMULATE_DELAY_MAX_GAP	"put your description here."
#define DESCRIPTION_SIMULATE_DELAY_TIME_US	"put your description here."

static int d_device_provision = 0;
static int d_simulate_async = 0;
static int d_simulate_delay = 0;
static int d_simulate_delay_min_gap = 1;
static int d_simulate_delay_max_gap = 10;
static int d_simulate_delay_time_us = 100000;

/*
 * "simulate_delay": 0 for non-delay, 1 for read delay, 2 for write delay, 3 for read-write delay
 * "dev_name": a section name of a device type
 */
struct ini_key_desc drw_keys[] = {
	{"type",			KEY_TYPE_STRING,	"drw",				1,	DESCRIPTION_TYPE},
	{"exportname",			KEY_TYPE_STRING,	NULL,				0,	DESCRIPTION_EXPORTNAME},
	{"dev_name",			KEY_TYPE_STRING,	NULL,				0,	DESCRIPTION_DEV_NAME},
	{"device_provision",		KEY_TYPE_INT,		&d_device_provision,		0,	DESCRIPTION_DEVICE_PROVISION},
	{"simulate_async",		KEY_TYPE_INT,		&d_simulate_async,		0,	DESCRIPTION_SIMULATE_ASYNC},
	{"simulate_delay",		KEY_TYPE_INT,		&d_simulate_delay,		0,	DESCRIPTION_SIMULATE_DELAY},
	{"simulate_delay_min_gap",	KEY_TYPE_INT,		&d_simulate_delay_min_gap,	0,	DESCRIPTION_SIMULATE_DELAY_MIN_GAP},
	{"simulate_delay_max_gap",	KEY_TYPE_INT,		&d_simulate_delay_max_gap,	0,	DESCRIPTION_SIMULATE_DELAY_MAX_GAP},
	{"simulate_delay_time_us",	KEY_TYPE_INT,		&d_simulate_delay_time_us,	0,	DESCRIPTION_SIMULATE_DELAY_TIME_US},
	{NULL,				KEY_TYPE_WRONG,		NULL,				0,	NULL}
};

void
conf_drw_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)drw_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
