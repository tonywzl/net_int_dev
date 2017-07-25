/*
 * manager-wc.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "version.h"
#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrmrw_if.h"
#include "mac_if.h"
#include "manager.h"



#define CMD_INFORMATION		1
#define CMD_ADD			2
#define CMD_DISPLAY		3
#define CMD_HELLO		4
#define CMD_MAX			5


#define CODE_NONE		0

/* code of info */
#define CODE_INFO_STAT		1

/* code of display */
#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_W_DIS		3
#define CODE_W_DIS_ALL		4

/* code of hello */
#define CODE_HELLO		1
static void usage_mrw();

static int
__mrw_information_func(int argc, char* const argv[], char *mrw_name, struct mgrmrw_interface *mgrmrw_p)
{
	char *log_header = "__mrw_information_func";
	char *cmd_code_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	if (!mrw_name) {
		goto out;
	}

	if (strlen(mrw_name) >= NID_MAX_DEVNAME) {
		warn_str = "wrong mrw_name!";
		goto out;
	}

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "stat"))
			cmd_code = CODE_INFO_STAT;
		else {
			warn_str = "wrong sub operation!";
			goto out;
		}
	}

	if (cmd_code == CODE_INFO_STAT) {
		nid_log_warning("%s: cmd_code=%d, optind:%d : information_stat",
			log_header, cmd_code, optind);
		rc = mgrmrw_p->mr_op->mr_information_stat(mgrmrw_p, mrw_name);
		return rc;
	} else {
		warn_str = "no sub operation specified!";
		goto out;
	}

out:
	if (rc) {
		usage_mrw();
		nid_log_warning("%s: mrw_name:%s: %s",
			log_header, mrw_name, warn_str);
	}
	return rc;
}

static int
__mrw_add_func(int argc, char* const argv[], char *mrw_name, struct mgrmrw_interface *mgrmrw_p)
{
	char *log_header = "__mrw_add_func";
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	if (!mrw_name) {
		goto out;
	}

	if (strlen(mrw_name) >= NID_MAX_DEVNAME) {
		warn_str = "wrong mrw_name!";
		goto out;
	}

	if (optind < argc) {
		warn_str = "got too many arguments!";
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d mrw_name:%s",
				log_header, cmd_code, optind, mrw_name);
	rc = mgrmrw_p->mr_op->mr_add(mgrmrw_p, mrw_name);
	return rc;
out:
	if (rc) {
		usage_mrw();
		nid_log_warning("%s: mrw_name:%s: %s",
			log_header, mrw_name, warn_str);
	}
	return rc;
}

static int
__mrw_display_func(int argc, char* const argv[], char *mrw_name, struct mgrmrw_interface *mgrmrw_p)
{
	char *log_header = "__mrw_display_func";
	char *cmd_str = "";
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	if (optind >= argc) {
		warn_str = "got empty sub command!";
		usage_mrw();
		goto out;
	}

	cmd_str = argv[optind++];

	if (mrw_name != NULL && cmd_str != NULL) {
		if (strlen(mrw_name) > NID_MAX_DEVNAME) {
			warn_str = "wrong mrw_name";
			usage_mrw();
			goto out;
		}
		if (!strcmp(cmd_str, "setup")) {
			cmd_code = CODE_S_DIS;
			is_setup = 1;
		}
		else if (!strcmp(cmd_str, "working")) {
			cmd_code = CODE_W_DIS;
			is_setup = 0;
		}
	}
	else if (mrw_name == NULL && cmd_str != NULL) {
		if (!strcmp(cmd_str, "setup")) {
			cmd_code = CODE_S_DIS_ALL;
			is_setup = 1;
		}
		else if (!strcmp(cmd_str, "working")) {
			cmd_code = CODE_W_DIS_ALL;
			is_setup = 0;
		}
	}
	else if (cmd_str == NULL) {
		usage_mrw();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, mrw_name:%s: display",
			log_header, cmd_code, optind, mrw_name);
		rc = mgrmrw_p->mr_op->mr_display(mgrmrw_p, mrw_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, mrw_name:%s: display all",
			log_header, cmd_code, optind, mrw_name);
		rc = mgrmrw_p->mr_op->mr_display_all(mgrmrw_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, mrw_name:%s: display",
			log_header, cmd_code, optind, mrw_name);
		rc = mgrmrw_p->mr_op->mr_display(mgrmrw_p, mrw_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, mrw_name:%s: display",
			log_header, cmd_code, optind, mrw_name);
		rc = mgrmrw_p->mr_op->mr_display_all(mgrmrw_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, mrw_name=%s: got wrong cmd_code",
			log_header, cmd_code, optind, mrw_name);
	}

out:
	if (rc) {
		nid_log_warning("%s: mrw_name:%s: %s",
			log_header, mrw_name, warn_str);
	}
	return rc;
}

static int
__mrw_hello_func(int argc, char* const argv[], char *mrw_name, struct mgrmrw_interface *mgrmrw_p)
{
	char *log_header = "__mrw_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);
	assert(&mrw_name);

	if (optind > argc) {
		usage_mrw();
		goto out;
	}

	cmd_code = CODE_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind:%d: hello",
		log_header, cmd_code, optind);
	rc = mgrmrw_p->mr_op->mr_hello(mgrmrw_p);
out:
	return rc;
}

typedef int (*mrw_operation_func)(int, char* const *, char *, struct mgrmrw_interface *);
struct manager_mrw_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	mrw_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_mrw_task mrw_task_table[] = {
	{"info",	"information",	CMD_INFORMATION,	__mrw_information_func,	NULL,	NULL},
	{"add",		"add",		CMD_ADD,		__mrw_add_func,		NULL,	NULL},
	{"display",	"disp",		CMD_DISPLAY,		__mrw_display_func,	NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,		__mrw_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_info_setup_description(struct manager_mrw_task *info_mtask)
{
	info_mtask->description = "info:	information display command\n";

	info_mtask->detail = x_malloc(1024*1024);
	sprintf(info_mtask->detail, "%s",
		first_intend"stat:			display mrw gerenal status\n"
		first_intend"			sample:nidmanager -r s -m mrw -i your_mrw_name info stat \n");
}

static void
__setup_add_setup_description(struct manager_mrw_task *add_mtask)
{
	add_mtask->description = "add:	add operation command\n";

	add_mtask->detail = x_malloc (1024*1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"[mrw_name] add"
			"		mrw configurations\n"
		first_intend"			sample:nidmanager -r s -m mrw -i your_mrw_name add \n");
}

static void
__setup_display_setup_description(struct manager_mrw_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of mrw";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show mrw configuration\n"
		first_intend"			sample:nidmanager -r s -m mrw display setup\n"
		first_intend"			sample:nidmanager -r s -m mrw -i mrw_name display setup\n"
		first_intend"working:		show working mrw configuration\n"
		first_intend"			sample:nidmanager -r s -m mrw display working\n"
		first_intend"			sample:nidmanager -r s -m mrw -i mrw_name display working\n");
}

static void
__setup_hello_setup_description(struct manager_mrw_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m mrw hello\n");
}

static void
setup_descriptions()
{
	struct manager_mrw_task *mtask;

	mtask = &mrw_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_INFORMATION)
			__setup_info_setup_description(mtask);
		else if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DISPLAY)
			__setup_display_setup_description(mtask);
		else if (mtask->cmd_index == CMD_HELLO)
			__setup_hello_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_mrw()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_mrw_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m mrw ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &mrw_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (mrw_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &mrw_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s\n",task->description );
		printf("%s\n",task->detail);

	}
	printf ("\n");

	return;
}

static char *optstr_ss = "a:i:";

static struct option long_options_ss[] = {
	{0, 0, 0, 0},
};

int
manager_server_mrw(int argc, char * const argv[])
{
	char *log_header = "manager_server_mrw";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL, *mrw_name = NULL;
	char *ipstr = "127.0.0.1";	// by default
//	char *mrw_uuid = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrmrw_interface *mgrmrw_p;
	struct mgrmrw_setup setup;
	struct manager_mrw_task *mtask = &mrw_task_table[0];
	int rc = 0;

	umpk_p = x_calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	nid_log_debug("%s: start ...\n", log_header);
	while ((c = getopt_long(argc, argv, optstr_ss, long_options_ss, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		case 'i':
			mrw_name = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_mrw();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrmrw_p = x_calloc(1, sizeof(*mgrmrw_p));
	mgrmrw_initialization(mgrmrw_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_mrw();
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
		usage_mrw();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, mrw_name, mgrmrw_p);

out:
	return rc;
}
