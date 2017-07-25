#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "nid_log.h"
#include "ini_if.h"

static struct ini_interface ut_ini;
int d_num_workers = 0;

struct ini_key_desc _keys0[] = {
	{"num_workers", KEY_TYPE_INT, 	&d_num_workers, 0},
	{NULL,		KEY_TYPE_WRONG,	NULL,		0}
};

int d_sync = 1;
struct ini_key_desc _keys1[] = {
	{"exportname",	KEY_TYPE_STRING,	NULL,		1},
	{"sync",	KEY_TYPE_BOOL,		&d_sync,	0},
	{NULL,		KEY_TYPE_WRONG,		NULL,		0}
};

struct ini_key_desc* _keys_ary[] = {
	_keys0,
	_keys1,
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
	struct ini_setup setup;

	nid_log_open();
	nid_log_info("ini start ...");

	if (argc < 2) {
		nid_log_error("missing path");
		printf("missing path\n");
		exit(1);
	} else {
		setup.path = argv[1];
	}
	setup.keys_desc = _keys_ary;
	setup.sections_desc = _sections;
	ini_initialization(&ut_ini, &setup);
	ut_ini.i_op->i_parse(&ut_ini);  
	ut_ini.i_op->i_display(&ut_ini);  

	nid_log_info("ini end...");
	nid_log_close();
	return 0;
}
