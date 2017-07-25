/*
 * manager-cx.c
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
#include "mgrcx_if.h"
#include "mac_if.h"
#include "manager.h"


#define CMD_DISPLAY		1
#define CMD_HELLO		2
#define CMD_MAX			3

#define CODE_NONE		0

#define CODE_HELLO		1

/* code of display */
#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_W_DIS		3
#define CODE_W_DIS_ALL		4

static void usage_cx();


static int
__cx_display_func(int argc, char* const argv[], char* cxa_name, char* cxp_name, struct mgrcx_interface *mgrcx_p)
{
	char *log_header = "__cx_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	assert(!cxp_name);

	if (optind >= argc) {
		warn_str = "got empty sub command!";
		usage_cx();
		goto out;
	}

	cmd_str = argv[optind++];

	if (cxa_name != NULL && cmd_str != NULL) {
		if (strlen(cxa_name) > NID_MAX_UUID) {
			warn_str = "wrong cxa_name!";
			usage_cx();
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
	else if (cxa_name == NULL && cmd_str != NULL) {
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
		usage_cx();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, cxa_name:%s: display",
			log_header, cmd_code, optind, cxa_name);
		rc = mgrcx_p->cx_op->cx_display(mgrcx_p, cxa_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, cxa_name:%s: display_all",
			log_header, cmd_code, optind, cxa_name);
		rc = mgrcx_p->cx_op->cx_display_all(mgrcx_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, cxa_name:%s: display",
			log_header, cmd_code, optind, cxa_name);
		rc = mgrcx_p->cx_op->cx_display(mgrcx_p, cxa_name, is_setup);
		if (rc)
		warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, cxa_name:%s: display_all",
			log_header, cmd_code, optind, cxa_name);
		rc = mgrcx_p->cx_op->cx_display_all(mgrcx_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind:%d, cxa_name:%s: display_all",
			log_header, cmd_code, optind, cxa_name);
	}

out:
	if (rc) {
		nid_log_warning("%s: cxa_name:%s: %s",
			log_header, cxa_name, warn_str);
	}
	return rc;
}

static int
__cx_hello_func(int argc, char* const argv[], char* cxa_name, char* cxp_name, struct mgrcx_interface *mgrcx_p)
{
	char *log_header = "__cx_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);
	assert(&cxa_name);
	assert(&cxp_name);

	if (optind > argc) {
		usage_cx();
		goto out;
	}

	cmd_code = CODE_HELLO;
	nid_log_warning("%s: cmd_code=%d, optind=%d: hello",
		log_header, cmd_code, optind);
	rc = mgrcx_p->cx_op->cx_hello(mgrcx_p);

out:
	return rc;
}

typedef int (*cx_operation_func)(int, char* const *, char *, char *, struct mgrcx_interface *);
struct manager_cx_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	cx_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_cx_task cx_task_table[] = {
	{"display",	"disp",			CMD_DISPLAY,		__cx_display_func,	NULL,	NULL},
	{"hello",	"hello",		CMD_HELLO,		__cx_hello_func,	NULL,	NULL},
	{NULL,		NULL,			CMD_MAX,		NULL,			NULL,	NULL},
};


static void
__setup_display_setup_description(struct manager_cx_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of cxa\n";

	disp_mtask->detail = malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show cxa configuration\n"
		first_intend"			sample:nidmanager -r s -m cx display setup\n"
		first_intend"			sample:nidmanager -r s -m cx -i cxa_name display setup\n"
		first_intend"working:		show working cxa configuration\n"
		first_intend"			sample:nidmanager -r s -m cx display working\n"
		first_intend"			sample:nidmanager -r s -m cx -i cxa_name display working\n");
}

static void
__setup_hello_setup_description(struct manager_cx_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not:\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s", 
		first_intend"			sample: nidmanager -r s -m cx hello");
}

static void
setup_descriptions()
{
	struct manager_cx_task *mtask;

	mtask = &cx_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_DISPLAY)
			__setup_display_setup_description(mtask);
		else if (mtask->cmd_index == CMD_HELLO)
			__setup_hello_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_cx()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_cx_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m cx ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &cx_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (cx_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &cx_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s\n",task->description );
		printf("%s\n",task->detail);

	}
	printf ("\n");

	return;
}

static char *optstr_ss = "a:i:p:";

static struct option long_options_ss[] = {
	{0, 0, 0, 0},
};

int
manager_server_cx(int argc, char * const argv[])
{
	char *log_header = "manager_server_mcx";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL, *cxa_name = NULL, *cxp_name = NULL;
	char *ipstr = "127.0.0.1";	// by default
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrcx_interface *mgrcx_p;
	struct mgrcx_setup setup;
	struct manager_cx_task *mtask = &cx_task_table[0];
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
			cxa_name = optarg;
			break;

		case 'p':
			cxp_name = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_cx();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrcx_p = x_calloc(1, sizeof(*mgrcx_p));
	mgrcx_initialization(mgrcx_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_cx();
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
		usage_cx();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, cxa_name, cxp_name, mgrcx_p);

out:
	return rc;
}
