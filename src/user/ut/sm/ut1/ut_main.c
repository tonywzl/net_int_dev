#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ini_if.h"
#include "sm_if.h"

#define AGENT_CONF_FILE "/etc/ilio/nidagent.conf"

static struct ini_setup ini_setup;
static struct ini_interface sm_ini;
static struct sm_setup sw_setup;
static struct sm_interface ut_sm;

int d_retry = 5;	// default number of retry before io_error 

struct ini_key_desc _global_keys[] = {
	{"retry",	KEY_TYPE_INT, 	&d_retry, 0},
	{NULL,		KEY_TYPE_WRONG,	NULL,		0}
};

struct ini_key_desc _channel_keys[] = {
	{"exportname",	KEY_TYPE_STRING,	NULL,		1},
	{"ip",		KEY_TYPE_STRING,	NULL,		1},
	{"postrun",	KEY_TYPE_STRING,	NULL,		0},
	{NULL,		KEY_TYPE_WRONG,		NULL,		0}
};

struct ini_key_desc* _keys_ary[] = {
	_global_keys,
	_channel_keys,
	NULL,
};

struct ini_section_desc _sections[] = {
	{"global",	0,	NULL},
	{NULL,		2000,	NULL},
	{NULL,		-1,	NULL},
};

int
config_parser(struct ini_interface *ini_p)
{
	struct ini_section_desc **sections;
	sections = ini_p->i_op->i_generate_description(ini_p, _keys_ary, _sections);
	ini_p->i_op->i_parse(ini_p, sections);
	ini_p->i_op->i_display(ini_p);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("sm ut module start ...");

	ini_setup.path = AGENT_CONF_FILE;
	ini_initialization(&sm_ini, &ini_setup);
	config_parser(&sm_ini);

	sw_setup.ini = &sm_ini;
	sm_initialization(&ut_sm, &sw_setup);
	ut_sm.s_op->s_display(&ut_sm);

	nid_log_info("sm ut module end...");
	nid_log_close();

	return 0;
}
