/*
 * manager_ini.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrini_if.h"
#include "manager.h"

#define CONFIGURATION	0
#define TEMPLATE	1

#define CMD_DISPLAY	1
#define CMD_HELLO	2
#define CMD_MAX		3

#define CODE_NONE	0

#define CODE_DIS_T_SECTION		1
#define CODE_DIS_T_ALL			2
#define CODE_DIS_T_SECTION_DETAIL	3
#define CODE_DIS_C_SECTION		4
#define CODE_DIS_C_ALL			5
#define CODE_DIS_C_SECTION_DETAIL	6

#define CODE_HELLO	1

static void usage_ini();

static int
__ini_display_func(int argc, char * const argv[], int is_template, int is_module, struct mgrini_interface *mgrini_p)
{
	char *log_header = "__ini_display_func";
	char *s_name = NULL;
	int rc = -1, cmd_code = CODE_NONE;

	if (is_template == 1 && is_module == 1) {
		cmd_code = CODE_DIS_T_SECTION;
	}
	else if (is_template == 1 && is_module == 0) {
		if (optind < argc) {
			s_name = argv[optind++];
			cmd_code = CODE_DIS_T_SECTION_DETAIL;
		}
		else {
			cmd_code = CODE_DIS_T_ALL;
		}
	}
	else if	(is_template == 0 && is_module == 1) {
		cmd_code = CODE_DIS_C_SECTION;
	}
	else if (is_template == 0 && is_module == 0) {
		if (optind < argc) {
			s_name = argv[optind++];
			cmd_code = CODE_DIS_C_SECTION_DETAIL;
		}
		else {
			cmd_code = CODE_DIS_C_ALL;
		}
	}
	else{
		usage_ini();
		return rc;
	}

	if (cmd_code == CODE_DIS_T_SECTION_DETAIL) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, s_name=%s: display_t_section_detail",
			log_header, cmd_code, optind, s_name);
		if (s_name != NULL) {
			rc = mgrini_p->i_op->ini_display_section_detail(mgrini_p, s_name, TEMPLATE);
		}
		else {
			usage_ini();
		}
	}
	else if (cmd_code == CODE_DIS_T_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, s_name=%s: display_t_all",
                        log_header, cmd_code, optind, s_name);
		rc = mgrini_p->i_op->ini_display_all(mgrini_p, TEMPLATE);
	}
	else if (cmd_code == CODE_DIS_T_SECTION) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, s_name=%s: display_t_module",
                        log_header, cmd_code, optind, s_name);
		rc = mgrini_p->i_op->ini_display_section(mgrini_p, TEMPLATE);
	}
	else if (cmd_code == CODE_DIS_C_SECTION_DETAIL) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, s_name=%s: display_c_section_detail",
			log_header, cmd_code, optind, s_name);
		if (s_name != NULL) {
			rc = mgrini_p->i_op->ini_display_section_detail(mgrini_p, s_name, CONFIGURATION);
		}
		else {
			usage_ini();
		}
	}
	else if (cmd_code == CODE_DIS_C_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, s_name=%s: display_c_all",
			log_header, cmd_code, optind, s_name);
			rc = mgrini_p->i_op->ini_display_all(mgrini_p, CONFIGURATION);
	}
	else if (cmd_code == CODE_DIS_C_SECTION) {

		nid_log_warning("%s: cmd_code=%d, optind=%d, s_name=%s: display_c_section",
			log_header, cmd_code, optind, s_name);
			rc = mgrini_p->i_op->ini_display_section(mgrini_p, CONFIGURATION);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, s_name=%s: got wrong cmd_code",
			log_header, cmd_code, optind, s_name);
		usage_ini();
	}

	return rc;

}

static int
__ini_hello_func(int argc, char * const argv[], int is_template, int is_module, struct mgrini_interface *mgrini_p)
{
	char *log_header = "__ini_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(&is_template);
	assert(&is_module);
	assert(argv);

	if (optind > argc) {
		usage_ini();
		goto out;
	}

	cmd_code = CODE_HELLO;
	nid_log_warning("%s: cmd_code=%d, optind=%d: hello",
		log_header, cmd_code, optind);
	rc = mgrini_p->i_op->ini_hello(mgrini_p);

out:
	return rc;
}

typedef int (*ini_operation_func)(int, char *const *, int, int, struct mgrini_interface *);

struct manager_ini_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	ini_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_ini_task ini_task_table[] = {
	{"display",	"dis",		CMD_DISPLAY,		__ini_display_func,	NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,		__ini_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_display_setup_description(struct manager_ini_task *dis_mtask)
{
	dis_mtask->description = "display:	information display command\n";

	dis_mtask->detail = x_malloc(1024*1024);
	sprintf(dis_mtask->detail, "%s",
		first_intend"-T			show template information\n"
		first_intend"-M			show module names\n\n"
		first_intend"			display all template module names\n"
		first_intend"			sample: nidmanager -r s -m ini -T -M display\n\n"
		first_intend"			display all template keys\n"
		first_intend"			sample: nidmanager -r s -m ini -T display\n\n"
		first_intend"			display tmeplate keys of given module\n"
		first_intend"			[module_name]	the module name in template, use -M to show them\n"
		first_intend"			sample: nidmanager -r s -m ini -T display module_name\n\n"
		first_intend"			display all server config section names\n"
		first_intend"			sample: nidmanager -r s -m ini -M display\n\n"
		first_intend"			display all server config keys\n"
		first_intend"			sample: nidmanager -r s -m ini display\n\n"
		first_intend"			display server config keys of the given section\n"
		first_intend"			[section_name]	the section name in nidserver.conf, use -M to show them\n"
		first_intend"			sample: nidmanager -r s -m ini display section_name\n\n");
}

static void
__setup_hello_setup_description(struct manager_ini_task *hello_mtask)
{
	hello_mtask->description = "hello:		show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s", 
		first_intend"			sample: nidmanager -r s -m ini hello");
}

static void
setup_description()
{
	struct manager_ini_task *mtask;

	mtask = &ini_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_DISPLAY) {
			__setup_display_setup_description(mtask);
		}
		else if (mtask->cmd_index == CMD_HELLO) {
			__setup_hello_setup_description(mtask);
		}
		else {
			assert(0);
		}
		mtask++;
	}
}

void
usage_ini()
{
	char *log_header = "usage_ini";
	struct manager_ini_task *task;
	int i;

	setup_description();
	
	printf("%s: nidmanager		-r s -m ini ", log_header);
	for (i = 0; i < CMD_MAX; i++) {
		task = &ini_task_table[i];
		if (task->cmd_long_name == NULL) {
			break;
		}
		printf("%s", task->cmd_short_name);
		if (ini_task_table[i + 1].cmd_long_name != NULL) {
			printf("|");
		}
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i ++) {
		task = &ini_task_table[i];
		if (task->cmd_long_name == NULL) {
			break;
		}
		if (task->description && task->detail) {
			printf("%s\n", task->description);
			printf("%s\n", task->detail);
		}
	}
	printf("\n");

	return;
}

static char *opstr_ini = "a:TMC";
static struct option long_options_ini[] = {
	{0, 0, 0, 0},
};

int
manager_server_ini(int argc, char * const argv[])
{
	char *log_header = "manager_server_ini";
	char c;
	int i;
	int is_template = 0, is_module = 0;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrini_setup setup;
	struct mgrini_interface *mgrini_p;
	struct manager_ini_task *mtask = &ini_task_table[0];
	int rc = 0, cmd_index;

	umpk_p = x_calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	nid_log_debug("%s: start ...", log_header);
	while ((c = getopt_long(argc, argv, opstr_ini, long_options_ini, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		case 'T':
			is_template = 1;
			break;

		case 'M':
			is_module = 1;
			break;

		default:
			printf("the option has not been created \n");
			usage_ini();

			goto out;
		}
	}

	if (optind >= argc) {
		usage_ini();

		goto out;
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;

	mgrini_p = x_calloc(1, sizeof(*mgrini_p));
	mgrini_initialization(mgrini_p, &setup);

	/*
	 * get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_ini();
		rc = -1;
		goto out;
	}

	cmd_index = CMD_MAX;
	while (mtask->cmd_index < CMD_MAX) {
		if (!strcmp(cmd_str, mtask->cmd_long_name) || !strcmp(cmd_str, mtask->cmd_short_name)) {
			cmd_index = mtask->cmd_index;
			break;
		}
		mtask++;
	}

	if (cmd_index == CMD_MAX) {
		usage_ini();
		rc = -1;
		goto out;
	}

		mtask->func(argc, argv, is_template, is_module, mgrini_p);
out:
	return rc;

}
