#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "nid_log.h"
#include "ini_if.h"

static struct ini_interface ut_ini_1;
static struct ini_interface ut_ini_2;

int d_num_workers = 8;	// default num_workers
int d_sync = 1;		// default sync mode  

struct ini_key_desc _global_keys[] = {
	{"num_workers", KEY_TYPE_INT, 	&d_num_workers, 0},
	{NULL,		KEY_TYPE_WRONG,	NULL,		0}
};

struct ini_key_desc _export_keys[] = {
	{"exportname",	KEY_TYPE_STRING,	NULL,		1},
	{"sync",	KEY_TYPE_BOOL,		&d_sync,	0},
	{NULL,		KEY_TYPE_WRONG,		NULL,		0}
};

struct ini_key_desc* _keys_ary[] = {
	_global_keys,
	_export_keys,
	NULL,
};

struct ini_section_desc _sections[] = {
	{"global",	0,	NULL},
	{NULL,		2000,	NULL},
	{NULL,		-1,	NULL},
};

int
main(int argc, char* argv[])
{
	struct ini_setup setup_1, setup_2;
	struct ini_section_content *sc_p, **sc_pp, *added_sc[100], *rm_sc[100];

	nid_log_open();
	nid_log_info("ini start ...");

	if (argc < 3) {
		nid_log_error("missing path");
		printf("missing path\n");
		exit(1);
	} else {
		setup_1.path = argv[1];
		setup_2.path = argv[2];
	}
	setup_1.keys_desc = _keys_ary;
	setup_1.sections_desc = _sections;	
	ini_initialization(&ut_ini_1, &setup_1);
	setup_2.keys_desc = _keys_ary;
	setup_2.sections_desc = _sections;	
	ini_initialization(&ut_ini_2, &setup_2);

	ut_ini_1.i_op->i_parse(&ut_ini_1);
	ut_ini_2.i_op->i_parse(&ut_ini_2);

	ut_ini_1.i_op->i_compare(&ut_ini_1, &ut_ini_2, added_sc, rm_sc);

	sc_pp = rm_sc;
	nid_log_debug("display removed sections start:");
	while (*sc_pp) {
		ut_ini_1.i_op->i_remove_section(&ut_ini_1, (*sc_pp)->s_name);
		nid_log_debug("uuid:%s", (*sc_pp)->s_name);	
		sc_pp++;
	}
	nid_log_debug("display removed sections end");

	nid_log_debug("display added sections start:");
	sc_pp = added_sc;
	while (*sc_pp) {
		sc_p = ut_ini_2.i_op->i_remove_section(&ut_ini_2, (*sc_pp)->s_name);
		if (sc_p)
			ut_ini_1.i_op->i_add_section(&ut_ini_1, *sc_pp);
		nid_log_debug("uuid:%s", (*sc_pp)->s_name);
		sc_pp++;
	}
	nid_log_debug("display added sections end");

	ut_ini_1.i_op->i_display(&ut_ini_1);

	nid_log_info("ini end...");
	nid_log_close();
	return 0;
}
