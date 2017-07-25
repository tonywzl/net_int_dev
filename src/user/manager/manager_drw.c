/*
 * manager-drw.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrdrw_if.h"
#include "manager.h"


#define CMD_ADD			1
#define CMD_DELETE		2
#define CMD_READY		3
#define CMD_DISPLAY		4
#define CMD_HELLO		5
#define	CMD_MAX			6

#define CODE_NONE		0

/* for cmd display */
#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_W_DIS		3
#define CODE_W_DIS_ALL		4

static void usage_drw();

static int
__drw_add_func(int argc, char* const argv[], char *drw_name, struct mgrdrw_interface *mgrdrw_p)
{
	char *log_header = "__drw_add_func";
	char *drw_conf;
	char args[3][1024];
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";
	int n;
	char *exportname;
	int device_provision = 0;

	if (!drw_name) {
		goto out;
	}

	if (strlen(drw_name) >= NID_MAX_DEVNAME) {
		warn_str = "wrong drw_name!";
		goto out;
	}

	if (optind >= argc) {
		warn_str = "got empty drw configuration!";
		goto out;
	}

	drw_conf = argv[optind++];
	n = sscanf(drw_conf, "%[^:]:%[^:]:%[^:]",
			args[0], args[1], args[2]);

	if (n != 1 && n != 2) {
		warn_str = "wrong number of drw configuration items!";
		goto out;
	}

	exportname = args[0];
	if (strlen(exportname) >= NID_MAX_PATH) {
		warn_str = "wrong exportname configuration!";
		goto out;
	}

	if (n == 2) {
		device_provision = atoi(args[1]);
		if (device_provision != 0 && device_provision != 1) {
			warn_str = "wrong device_provision configuration!";
			goto out;
		}
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d drw_name:%s",
				log_header, cmd_code, optind, drw_name);
	rc = mgrdrw_p->drw_op->drw_add(mgrdrw_p, drw_name, exportname, (uint8_t)device_provision);
	return rc;

out:
	if (rc) {
		usage_drw();
		nid_log_warning("%s: drw_name:%s: %s",
			log_header, drw_name, warn_str);
	}
	return rc;
}

static int
__drw_del_func(int argc, char * const argv[], char *drw_name, struct mgrdrw_interface *mgrdrw_p)
{
	char *log_header = "__drw_del_func";
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	if (!drw_name) {
		warn_str = "drw name cannot be null!";
		goto out;
	}

	if (optind < argc) {
		warn_str = "got too many arguments!";
		goto out;

	}

	if (strlen(drw_name) >= NID_MAX_DEVNAME) {
		warn_str = "wrong drw name!";
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d, drw_name:%s: del",
		log_header, cmd_code, optind, drw_name);
	rc = mgrdrw_p->drw_op->drw_delete(mgrdrw_p, drw_name);

out:
	if (rc) {
		usage_drw();
		nid_log_warning("%s: drw_name:%s: %s",
			log_header, drw_name, warn_str);
	}
	return rc;
}

static int
__drw_ready_func(int argc, char* const argv[], char *drw_name, struct mgrdrw_interface *mgrdrw_p)
{
	char *log_header = "__drw_ready_func";
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	if (optind < argc) {
		warn_str = "got too many arguments!";
		goto out;
	}

	if (!drw_name) {
		goto out;
	}

	if (strlen(drw_name) >= NID_MAX_DEVNAME) {
		warn_str = "wrong drw_name!";
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d drw_name:%s",
				log_header, cmd_code, optind, drw_name);
	rc = mgrdrw_p->drw_op->drw_ready(mgrdrw_p, drw_name);
	return rc;

out:
	if (rc) {
		usage_drw();
		nid_log_warning("%s: drw_name:%s: %s",
			log_header, drw_name, warn_str);
	}
	return rc;
}

static int
__drw_display_func(int argc, char* const argv[], char *drw_name, struct mgrdrw_interface *mgrdrw_p)
{
	char *log_header = "__drw_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	if (optind >= argc) {
		warn_str = "got empty sub command!";
		usage_drw();
		goto out;
	}

	cmd_str = argv[optind++];

	if (drw_name != NULL && cmd_str != NULL) {
		if (strlen(drw_name) > NID_MAX_DEVNAME) {
			warn_str = "wrong drw_name!";
			usage_drw();
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
	else if (drw_name == NULL && cmd_str != NULL){
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
		usage_drw();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, drw_name:%s: display",
			log_header, cmd_code, optind, drw_name);
		rc = mgrdrw_p->drw_op->drw_display(mgrdrw_p, drw_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, drw_name:%s: display all",
			log_header, cmd_code, optind, drw_name);
		rc = mgrdrw_p->drw_op->drw_display_all(mgrdrw_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, drw_name:%s: display",
			log_header, cmd_code, optind, drw_name);
		rc = mgrdrw_p->drw_op->drw_display(mgrdrw_p, drw_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, drw_name:%s: display",
			log_header, cmd_code, optind, drw_name);
		rc = mgrdrw_p->drw_op->drw_display_all(mgrdrw_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, drw_name=%s: got wrong cmd_code",
			log_header, cmd_code, optind, drw_name);
	}

out:
	if (rc) {
		nid_log_warning("%s: drw_name:%s: %s",
			log_header, drw_name, warn_str);
	}
	return rc;
}

static int
__drw_hello_func(int argc, char* const argv[], char *drw_name, struct mgrdrw_interface *mgrdrw_p)
{
	char *log_header = "__drw_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);
	assert(&drw_name);

	if (optind > argc) {
		usage_drw();
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d",
				log_header, cmd_code, optind);
	rc = mgrdrw_p->drw_op->drw_hello(mgrdrw_p);

out:
	return rc;
}

typedef int (*drw_operation_func)(int, char* const *, char *, struct mgrdrw_interface *);
struct manager_drw_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	drw_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_drw_task drw_task_table[] = {
	{"add",		"add",		CMD_ADD,		__drw_add_func,		NULL,	NULL},
	{"delete",	"del",		CMD_DELETE,		__drw_del_func,		NULL,	NULL},
	{"ready",	"ready",	CMD_READY,		__drw_ready_func,	NULL,	NULL},
	{"display",	"disp",		CMD_DISPLAY,		__drw_display_func,	NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,		__drw_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_add_setup_description(struct manager_drw_task *add_mtask)
{
	add_mtask->description = "add:	add operation command\n";
	add_mtask->detail = x_malloc (1024*1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"[drw_name] add [exportname][:[device_provision]]"
			"		drw configurations\n"
		first_intend"			sample:nidmanager -r s -m drw -i drw_name add /dev/PRIMARY_FLASH_vsSpWzxuRcSizGfwjh3v6Q/1_OhtzjfD3Ta6pchjtsRiYfw \n"
		first_intend"			sample:nidmanager -r s -m drw -i drw_name add /dev/PRIMARY_FLASH_vsSpWzxuRcSizGfwjh3v6Q/1_OhtzjfD3Ta6pchjtsRiYfw:1 \n");
}

static void
__setup_del_setup_description(struct manager_drw_task *del_mtask)
{
	del_mtask->description = "del:	delete operation command\n";

	del_mtask->detail = x_malloc(1024);
	sprintf(del_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m drw -i drw_name del\n");
}

static void
__setup_ready_setup_description(struct manager_drw_task *ready_mtask)
{
	ready_mtask->description = "ready:	device ready notification command\n";
	ready_mtask->detail = x_malloc (1024*1024);
	sprintf(ready_mtask->detail, "%s",
		first_intend"[drw_name] ready"
		first_intend"			sample:nidmanager -r s -m drw -i drw_name ready \n");
}

static void
__setup_display_setup_description(struct manager_drw_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of drw";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s", 
		first_intend"setup:			show drw configuration\n"
		first_intend"			sample:nidmanager -r s -m drw display setup\n"
		first_intend"			sample:nidmanager -r s -m drw -i drw_name display setup\n"
		first_intend"working:		show working drw configuration\n"
		first_intend"			sample:nidmanager -r s -m drw display working\n"
		first_intend"			sample:nidmanager -r s -m drw -i drw_name display working\n");
}

static void
__setup_hello_setup_description(struct manager_drw_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			samepl:nidmanager -r s -m drw hello\n");
}

static void
setup_descriptions()
{
	struct manager_drw_task *mtask;

	mtask = &drw_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DELETE)	
			__setup_del_setup_description(mtask);
		else if (mtask->cmd_index == CMD_READY)
			__setup_ready_setup_description(mtask);
		else if (mtask->cmd_index == CMD_HELLO)
			__setup_hello_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DISPLAY)
			__setup_display_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_drw()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_drw_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m drw ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &drw_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (drw_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &drw_task_table[i];
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
	{"vec", required_argument, NULL, 'v'},
	{0, 0, 0, 0},
};

int
manager_server_drw(int argc, char * const argv[])
{
	char *log_header = "manager_server_drw";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	char *drw_name = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrdrw_interface *mgrdrw_p;
	struct mgrdrw_setup setup;
	struct manager_drw_task *mtask = &drw_task_table[0];
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
			drw_name = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_drw();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrdrw_p = x_calloc(1, sizeof(*mgrdrw_p));
	mgrdrw_initialization(mgrdrw_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_drw();
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
		usage_drw();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, drw_name, mgrdrw_p);

out:
	return rc;
}
