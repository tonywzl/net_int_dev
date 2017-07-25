#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "nid.h"
#include "agent.h"
#include "tp_if.h"
#include "asc_if.h"
#include "acg_if.h"

struct ini_setup ini_setup;
struct ini_interface acg_ini;
struct tp_setup tp_setup = {1, 10, 1, 1};
static struct tp_interface ut_tp;
struct acg_setup acg_setup;
struct acg_interface ut_acg;


static int d_retry = 5;	// default number of retry before io_error 

static struct ini_key_desc _global_keys[] = {
	{"retry",	KEY_TYPE_INT, 	&d_retry, 0},
	{NULL,		KEY_TYPE_WRONG,	NULL,		0}
};

static struct ini_key_desc _channel_keys[] = {
	{"exportname",	KEY_TYPE_STRING,	NULL,		1},
	{"ip",		KEY_TYPE_STRING,	NULL,		1},
	{"postrun",	KEY_TYPE_STRING,	NULL,		0},
	{NULL,		KEY_TYPE_WRONG,		NULL,		0}
};

struct ini_key_desc* agent_keys_ary[] = {
	_global_keys,
	_channel_keys,
	NULL,
};

struct ini_section_desc agent_sections[] = {
	{"global",	0,	NULL},
	{NULL,		2000,	NULL},
	{NULL,		-1,	NULL},
};

int
main()
{
	nid_log_open();
	nid_log_info("acg start ...");

	ini_setup.path = NID_CONF_AGENT;
	ini_setup.keys_desc = agent_keys_ary;
	ini_setup.sections_desc = agent_sections;
	ini_initialization(&acg_ini, &ini_setup);
	acg_ini.i_op->i_parse(&acg_ini);

	tp_initialization(&ut_tp, &tp_setup);
	acg_setup.tp = &ut_tp;
	acg_setup.ini = &acg_ini;
	acg_initialization(&ut_acg, &acg_setup);
	ut_acg.a_op->a_display(&ut_acg);
	sleep(10);

	nid_log_info("acg end...");
	nid_log_close();

	return 0;
}
