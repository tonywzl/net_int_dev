#include "lstn_if.h"
#include "ini_if.h"
#include "nid.h"

#define DESCRIPTION_TYPE		"Writeback Write Cache Module."
#define DESCRIPTION_CACHE_DEVICE	"Write cache device directory."
#define DESCRIPTION_TP_NAME		"Name of the page pool."
#define DESCRIPTION_CACHE_SIZE		"Size of the write cache unit Megabyte."
#define DESCRIPTION_RW_SYNC		"Whether enable sync mode."
#define DESCRIPTION_TWO_STEP_READ	"Whether enable two step read mode."
#define DESCRIPTION_DO_FP		"Whether enable finger point mode."
#define DESCRIPTION_FLUSH_POLICY	"Name of the flush policy."
#define DESCRIPTION_COALESCE_RATIO	"Ratio of coalesce when flushing."
#define DESCRIPTION_LOAD_CTRL_LEVEL	"Load control level for some specific flush policy."
#define DESCRIPTION_LOAD_RATIO_MAX	"Load max ratio for some specific flush policy."
#define DESCRIPTION_LOAD_RATIO_MIN	"Load min ratio for some specific flush policy."
#define DESCRIPTION_FLUSH_DELAY_CTL	"Flush delay control for some specific flush policy."
#define DESCRIPTION_THROTTLE_RATIO	"Throttle ratio for some specific flush policy."
#define DESCRIPTION_SSD_MODE		"Whether enable ssd mode."
#define DESCRIPTION_MAX_FLUSH_SIZE	"Maxinum size of one flush operation."
#define DESCRIPTION_WRITE_DELAY_FIRST	"First level of the write delay."
#define DESCRIPTION_WRITE_DELAY_SECOND	"Second level of the write delay."
#define DESCRIPTION_WRITE_DELAY_FIRST_MAX_MS	"Time of the delay for first delay level."
#define DESCRIPTION_WRITE_DELAY_SECOND_MAX_MS	"Time of the delay for second delay level."
#define DESCRIPTION_HIGH_WATER_MARK	"High water mark for some specific flush policy."
#define DESCRIPTION_LOW_WATER_MARK	"Low water mark for some specific flush policy.."

static int d_wrong_size = -1;           // default page size for buf page size, 8M
static int d_rw_sync = 0;
static int d_two_step_read = 0;
static int d_do_fp = 1;
static int d_ssd_mode = 0;
static int d_max_flush_size = 8;
static int d_write_delay_first_level = 0;
static int d_write_delay_second_level = 0;
static int d_write_delay_first_level_max_us = 200000;
static int d_write_delay_second_level_max_us = 500000;

struct ini_key_desc bwc_keys[] = {
	{"type",			KEY_TYPE_STRING,	"bwc",				1,	DESCRIPTION_TYPE},
	{"cache_device",        	KEY_TYPE_STRING,        "Wrong-cache-device",   	0,	DESCRIPTION_CACHE_DEVICE},
	{"tp_name",        		KEY_TYPE_STRING,        NID_GLOBAL_TP, 			0,	DESCRIPTION_TP_NAME},
        {"cache_size",          	KEY_TYPE_INT,           &d_wrong_size,          	0,	DESCRIPTION_CACHE_SIZE},
        {"rw_sync",             	KEY_TYPE_INT,           &d_rw_sync,             	0,	DESCRIPTION_RW_SYNC},
        {"two_step_read",       	KEY_TYPE_INT,           &d_two_step_read,       	0,	DESCRIPTION_TWO_STEP_READ},
        {"do_fp",               	KEY_TYPE_INT,           &d_do_fp,               	0,	DESCRIPTION_DO_FP},
	{"write_delay_first_level",     KEY_TYPE_INT,           &d_write_delay_first_level,	0,	DESCRIPTION_WRITE_DELAY_FIRST},
	{"write_delay_second_level",    KEY_TYPE_INT,           &d_write_delay_second_level,	0,	DESCRIPTION_WRITE_DELAY_SECOND},
	{"write_delay_first_level_max_us", KEY_TYPE_INT,        &d_write_delay_first_level_max_us, 0,	DESCRIPTION_WRITE_DELAY_FIRST_MAX_MS},
	{"write_delay_second_level_max_us", KEY_TYPE_INT,       &d_write_delay_second_level_max_us, 0,	DESCRIPTION_WRITE_DELAY_SECOND_MAX_MS},
	{"ssd_mode",            	KEY_TYPE_INT,        	&d_ssd_mode,            	0,	DESCRIPTION_SSD_MODE},
	{"max_flush_size",      	KEY_TYPE_INT,           &d_max_flush_size,      	0,	DESCRIPTION_MAX_FLUSH_SIZE},
        {"flush_policy",        	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_FLUSH_POLICY},
        {"coalesce_ratio",      	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_COALESCE_RATIO},
        {"load_ctrl_level",     	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_LOAD_CTRL_LEVEL},
        {"load_ratio_max",      	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_LOAD_RATIO_MAX},
        {"load_ratio_min",      	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_LOAD_RATIO_MIN},
        {"flush_delay_ctl",     	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_FLUSH_DELAY_CTL},
        {"throttle_ratio",      	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_THROTTLE_RATIO},
        {"high_water_mark",     	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_HIGH_WATER_MARK},
        {"low_water_mark",      	KEY_TYPE_STRING,        NULL,                   	0,	DESCRIPTION_LOW_WATER_MARK},
	{NULL,				KEY_TYPE_WRONG,		NULL,				0,	NULL}
};

void
conf_bwc_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)bwc_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
