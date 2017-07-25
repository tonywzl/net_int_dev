#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "tp_if.h"
#include "ini_if.h"
#include "dcn_if.h"
#include "rdg_if.h"
#include "dit_if.h"

static int d_num_threads = 1;
static int d_maxlen = 128;	// in K
static int d_time_to_run = 10;	// in sec

static struct ini_key_desc dit_global_keys[] = {
	{"num_threads",	KEY_TYPE_INT,		&d_num_threads, 0},
	{"devname",	KEY_TYPE_STRING,	NULL,		1},
	{"bck_devname",	KEY_TYPE_STRING,	NULL,		1},
	{"devsize",	KEY_TYPE_INT,		NULL,		1},	// in G
	{"maxlen",	KEY_TYPE_INT,		&d_maxlen,	0},	// in M
	{"time_to_run",	KEY_TYPE_INT,		&d_time_to_run,	0},	// in sec
	{NULL,		KEY_TYPE_WRONG,		NULL,		0}	
};

static struct ini_key_desc* dit_keys_ary[] = {
	dit_global_keys,
	NULL,
};

static struct ini_section_desc dit_sections[] = {
	{"global",	0,	NULL},
	{NULL,		-1,	NULL},
};

int
main()
{
	struct ini_setup ini_setup;
	struct ini_interface *ini_p;
	struct tp_setup tp_setup ={0, 0, 0, 1};
	struct tp_interface *tp_p;
	struct dcn_setup dcn_setup;
	struct dcn_interface *dcn_p;
	struct rdg_setup rdg_setup;
	struct rdg_interface *rdg_p;
	struct dit_setup dit_setup;
	struct dit_interface *dit_p;
	struct ini_section_content *global_sect;
	struct ini_key_content *the_key;
	int num_threads;
	char *devname, *bck_devname;
	uint32_t devsize, maxlen, time_to_run;

	nid_log_open();
	nid_log_info("dit testing module start ...");

	ini_setup.path = "/etc/dit.conf";
	ini_setup.keys_desc = dit_keys_ary;
	ini_setup.sections_desc = dit_sections;
	ini_p = calloc(1, sizeof(*ini_p));
	ini_initialization(ini_p, &ini_setup);
	ini_p->i_op->i_parse(ini_p);
	global_sect = ini_p->i_op->i_search_section(ini_p, "global");
	assert(global_sect);

	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "num_threads");
	num_threads = *((int32_t *)the_key->k_value);
	nid_log_notice("num_threads is :%d", num_threads);

	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "devname");
	devname = (char *)(the_key->k_value);
	nid_log_notice("devname is :%s", devname);

	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "bck_devname");
	bck_devname = (char *)(the_key->k_value);
	nid_log_notice("bck_devname is :%s", bck_devname);

	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "devsize");
	devsize = *((int32_t *)the_key->k_value);
	nid_log_notice("devsize is: %uG", devsize);

	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "maxlen");
	maxlen = *((int32_t *)the_key->k_value);
	nid_log_notice("maxlen is: %uK", maxlen);

	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "time_to_run");
	time_to_run = *((int32_t *)the_key->k_value);
	nid_log_notice("time_to_run is: %u sec", time_to_run);

	tp_p = calloc(1, sizeof(*tp_p));
	tp_initialization(tp_p, &tp_setup);

	dcn_p = calloc(1, sizeof(*dcn_p));
	dcn_initialization(dcn_p, &dcn_setup);

	rdg_setup.start_off = 0;
	rdg_setup.range = devsize;
	rdg_setup.maxlen = maxlen;
	rdg_p = calloc(1, sizeof(*rdg_p));
	rdg_initialization(rdg_p, &rdg_setup);

	dit_setup.ini = ini_p;
	dit_setup.tp = tp_p;
	dit_setup.dcn = dcn_p;
	dit_setup.rdg = rdg_p;
	dit_setup.nr_threads = num_threads;
	dit_setup.devname = devname;
	dit_setup.bck_devname = bck_devname;
	dit_setup.time_to_run = time_to_run;
	dit_p = calloc(1, sizeof(*dit_p));
	dit_initialization(dit_p, &dit_setup);

	dit_p->d_op->d_run(dit_p);
	sleep(1);

	nid_log_info("dit testing module end...");
	nid_log_close();

	return 0;
}
