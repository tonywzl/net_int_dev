/*
 * manager-dx.c
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
#include "mgrdx_if.h"
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

static void usage_dx();


static int
__dx_display_func(int argc, char * const argv[], char *dxa_name, char* dxp_name, struct mgrdx_interface *mgrdx_p)
{
	char *log_header = "__dx_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	assert(!dxp_name);

	if (optind >= argc){
		warn_str = "got empty sub command!";
		usage_dx();
		goto out;
	}

	cmd_str = argv[optind++];

	if (dxa_name != NULL && cmd_str != NULL) {
		if (strlen(dxa_name) > NID_MAX_UUID) {
			warn_str = "wrong dxa_name!";
			usage_dx();
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
	else if (dxa_name == NULL && cmd_str != NULL) {
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
		usage_dx();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, dxa_name:%s: display",
			log_header, cmd_code, optind, dxa_name);
		rc = mgrdx_p->dx_op->dx_display(mgrdx_p, dxa_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, dxa_name:%s: display_all",
			log_header, cmd_code, optind, dxa_name);
		rc = mgrdx_p->dx_op->dx_display_all(mgrdx_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, dxa_name:%s: display",
			log_header, cmd_code, optind, dxa_name);
		rc = mgrdx_p->dx_op->dx_display(mgrdx_p, dxa_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, dxa_name:%s: display_all",
			log_header, cmd_code, optind, dxa_name);
		rc = mgrdx_p->dx_op->dx_display_all(mgrdx_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind:%d, dxa_name:%s: got wrong cmd_code",
			log_header, cmd_code, optind, dxa_name);
	}

out:
	if (rc) {
		nid_log_warning("%s: dxa_name:%s: %s",
			log_header, dxa_name, warn_str);
	}
	return rc;
}

static int
__dx_hello_func(int argc, char* const argv[], char* dxa_name,char* dxp_name, struct mgrdx_interface *mgrdx_p)
{
	char *log_header = "__dx_create_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);
	assert(&dxa_name);
	assert(&dxp_name);

	if (optind > argc) {
		usage_dx();
		goto out;
	}

	cmd_code = CODE_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind:%d",
				log_header, cmd_code, optind);
	rc = mgrdx_p->dx_op->dx_hello(mgrdx_p);
out:
	return rc;
}

typedef int (*dx_operation_func)(int, char* const *, char *, char *, struct mgrdx_interface *);
struct manager_dx_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	dx_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_dx_task dx_task_table[] = {
	{"display",	"disp",			CMD_DISPLAY,		__dx_display_func,	NULL,	NULL},
	{"hello",	"hello",		CMD_HELLO,		__dx_hello_func,	NULL,	NULL},
	{NULL,		NULL,			CMD_MAX,		NULL,			NULL,	NULL},
};


static void
__setup_display_setup_description(struct manager_dx_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of dxa\n";

	disp_mtask->detail = malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show dxa configuration\n"
		first_intend"			sample:nidmanager -r s -m dx display setup\n"
		first_intend"			sample:nidmanager -r s -m dx -i dxa_name display setup\n"
		first_intend"working:		show working dxa configuration\n"
		first_intend"			sample:nidmanager -r s -m dx display working\n"
		first_intend"			sample:nidmanager -r s -m dx -i dxa_name display working\n");
}

static void
__setup_hello_setup_description(struct manager_dx_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this moduel is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"hello:		sample:nidmanager -r -s -m dx hello\n");
}

static void
setup_descriptions()
{
	struct manager_dx_task *mtask;

	mtask = &dx_task_table[0];
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
usage_dx()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_dx_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m dx ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &dx_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (dx_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &dx_task_table[i];
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
manager_server_dx(int argc, char * const argv[])
{
	char *log_header = "manager_server_mdx";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL, *dxa_name = NULL, *dxp_name = NULL;
	char *ipstr = "127.0.0.1";	// by default
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrdx_interface *mgrdx_p;
	struct mgrdx_setup setup;
	struct manager_dx_task *mtask = &dx_task_table[0];
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
			dxa_name = optarg;
			break;

		case 'p':
			dxp_name = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_dx();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrdx_p = x_calloc(1, sizeof(*mgrdx_p));
	mgrdx_initialization(mgrdx_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_dx();
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
		usage_dx();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, dxa_name, dxp_name, mgrdx_p);

out:
	return rc;
}
