/*
 * manager-sds.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrsds_if.h"
#include "manager.h"


#define CMD_ADD			1
#define CMD_DELETE		2
#define CMD_DISPLAY		3
#define CMD_HELLO		4
#define	CMD_MAX			5

#define CODE_NONE		0

#define CODE_HELLO		1

/* for cmd display */
#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_W_DIS		3
#define CODE_W_DIS_ALL		4

#define EMPTY_CACHE	"NONE"

static void usage_sds();

static int
__sds_add_func(int argc, char* const argv[], char *sds_name, struct mgrsds_interface *mgrsds_p)
{
	char *log_header = "__sds_add_func";
	char *sds_conf;
	char args[4][1024];
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";
	int n;
	char *pp_name, *wc_uuid = NULL, *rc_uuid = NULL;

	if (!sds_name) {
		goto out;
	}

	if (strlen(sds_name) >= NID_MAX_DSNAME) {
		warn_str = "wrong sds_name!";
		goto out;
	}

	if (optind >= argc) {
		warn_str = "got empty sds configuration!";
		goto out;
	}

	sds_conf = argv[optind++];
	n = sscanf(sds_conf, "%[^:]:%[^:]:%[^:]:%[^:]",
			args[0], args[1], args[2], args[3]);

	/*
	 * n == 1: sds w/o read cache and write cache (non-buffer io)
	 * n == 3: sds with read cache and write cache (buffer io)
	 */
	if (n != 1 && n != 3) {
		warn_str = "wrong number of sds configuration items!";
		goto out;
	}

	pp_name = args[0];
	if (strlen(pp_name) >= NID_MAX_PPNAME) {
		warn_str = "wrong pp_name configuration!";
		goto out;
	}

	if (n == 3) {
		if (strcmp(args[1], EMPTY_CACHE)) {
			wc_uuid = args[1];
			if (strlen(wc_uuid) >= NID_MAX_UUID) {
				warn_str = "wrong wc_uuid configuration!";
				goto out;
			}
		}

		if (strcmp(args[2], EMPTY_CACHE)) {
			rc_uuid = args[2];
			if (strlen(rc_uuid) >= NID_MAX_UUID) {
				warn_str = "wrong rc_uuid configuration!";
				goto out;
			}
		}
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d sds_name:%s",
				log_header, cmd_code, optind, sds_name);
	rc = mgrsds_p->sds_op->sds_add(mgrsds_p, sds_name, pp_name, wc_uuid, rc_uuid);
	return rc;

out:
	if (rc) {
		usage_sds();
		nid_log_warning("%s: sds_name:%s: %s",
			log_header, sds_name, warn_str);
	}
	return rc;
}

static int
__sds_del_func(int argc, char * const argv[], char *sds_name, struct mgrsds_interface *mgrsds_p)
{
	char *log_header = "__sds_del_func";
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	if (!sds_name) {
		warn_str = "sds name cannot be null!";
		goto out;
	}

	if (optind < argc) {
		warn_str = "got too many arguments!";
		goto out;

	}

	if (strlen(sds_name) >= NID_MAX_DSNAME) {
		warn_str = "wrong sds name!";
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d, sds_name:%s: del",
		log_header, cmd_code, optind, sds_name);
	rc = mgrsds_p->sds_op->sds_delete(mgrsds_p, sds_name);

out:
	if (rc) {
		usage_sds();
		nid_log_warning("%s: sds_name:%s: %s",
			log_header, sds_name, warn_str);
	}
	return rc;
}

static int
__sds_display_func(int argc, char* const argv[], char *sds_name, struct mgrsds_interface *mgrsds_p)
{
	char *log_header = "__sds_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	if (optind >= argc) {
		warn_str = "got empty sub command!";
		usage_sds();
		goto out;
	}

	cmd_str = argv[optind++];

	if (sds_name != NULL && cmd_str != NULL) {
		if (strlen(sds_name) > NID_MAX_UUID) {
			warn_str = "wrong sds_name!";
			usage_sds();
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
	else if (sds_name == NULL && cmd_str != NULL){
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
		usage_sds();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sds_name:%s: display",
			log_header, cmd_code, optind, sds_name);
		rc = mgrsds_p->sds_op->sds_display(mgrsds_p, sds_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sds_name:%s: display all",
			log_header, cmd_code, optind, sds_name);
		rc = mgrsds_p->sds_op->sds_display_all(mgrsds_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sds_name:%s: display",
			log_header, cmd_code, optind, sds_name);
		rc = mgrsds_p->sds_op->sds_display(mgrsds_p, sds_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sds_name:%s: display",
			log_header, cmd_code, optind, sds_name);
		rc = mgrsds_p->sds_op->sds_display_all(mgrsds_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, sds_name=%s: got wrong cmd_code",
			log_header, cmd_code, optind, sds_name);
	}

out:
	if (rc) {
		nid_log_warning("%s: sds_name:%s: %s",
			log_header, sds_name, warn_str);
	}
	return rc;
}

static int
__sds_hello_func(int argc, char* const argv[], char *sds_name, struct mgrsds_interface *mgrsds_p)
{
	char *log_header = "__sds_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(&sds_name);
	assert(argv);

	if (optind > argc) {
		usage_sds();
		goto out;
	}

	cmd_code = CODE_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind:%d: hello",
				log_header, cmd_code, optind);
	rc = mgrsds_p->sds_op->sds_hello(mgrsds_p);
	return rc;

out:
	return rc;
}

typedef int (*sds_operation_func)(int, char* const *, char *, struct mgrsds_interface *);
struct manager_sds_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	sds_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_sds_task sds_task_table[] = {
	{"add",		"add",		CMD_ADD,		__sds_add_func,		NULL,	NULL},
	{"delete",	"del",		CMD_DELETE,		__sds_del_func,		NULL,	NULL},
	{"display",	"disp",		CMD_DISPLAY,		__sds_display_func,	NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,		__sds_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_add_setup_description(struct manager_sds_task *add_mtask)
{
	add_mtask->description = "add:	add operation command\n";
	
	add_mtask->detail = x_malloc (1024*1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"[sds_name] add [sds_pp_name][:[sds_wc_uuid]:[sds_rc_uuid]]"
			"		sds configurations\n"
		first_intend"			sample:nidmanager -r s -m sds -i sds_name add sds_pp_name \n"
		first_intend"			sample:nidmanager -r s -m sds -i sds_name add sds_pp_name:"EMPTY_CACHE":rc_uuid \n"
		first_intend"			sample:nidmanager -r s -m sds -i sds_name add sds_pp_name:wc_uuid:"EMPTY_CACHE"\n"
		first_intend"			sample:nidmanager -r s -m sds -i sds_name add sds_pp_name:wc_uuid:rc_uuid \n");
}

static void
__setup_del_setup_description(struct manager_sds_task *del_mtask)
{
	del_mtask->description = "del:	delete operation command\n";

	del_mtask->detail = x_malloc(1024);
	sprintf(del_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m sds -i sds_name del\n");
}

static void
__setup_display_setup_description(struct manager_sds_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of sds";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s", 
		first_intend"setup:			show sds configuration\n"
		first_intend"			sample:nidmanager -r s -m sds display setup\n"
		first_intend"			sample:nidmanager -r s -m sds -i sds_name display setup\n"
		first_intend"working:		show working sds configuration\n"
		first_intend"			sample:nidmanager -r s -m sds display working\n"
		first_intend"			sample:nidmanager -r s -m sds -i sds_name display working\n");
}

static void
__setup_hello_setup_description(struct manager_sds_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m sds hello\n");
};

static void
setup_descriptions()
{
	struct manager_sds_task *mtask;

	mtask = &sds_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DELETE)
			__setup_del_setup_description(mtask);
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
usage_sds()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_sds_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m sds ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &sds_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (sds_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &sds_task_table[i];
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
manager_server_sds(int argc, char * const argv[])
{
	char *log_header = "manager_server_sds";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	char *sds_name = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrsds_interface *mgrsds_p;
	struct mgrsds_setup setup;
	struct manager_sds_task *mtask = &sds_task_table[0];
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
			sds_name = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_sds();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrsds_p = x_calloc(1, sizeof(*mgrsds_p));
	mgrsds_initialization(mgrsds_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_sds();
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
		usage_sds();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, sds_name, mgrsds_p);

out:
	return rc;
}
