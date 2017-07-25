/*
 * manager_tp.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrtp_if.h"
#include "manager.h"

#define CMD_INFORMATION		1
#define CMD_ADD			2
#define CMD_DELETE		3
#define CMD_DISPLAY		4
#define CMD_HELLO		5
#define	CMD_MAX			6

#define CODE_NONE		0

#define CODE_INFO_STAT		1
#define CODE_INFO_ALL		2

#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_W_DIS		3
#define CODE_W_DIS_ALL		4

#define CODE_ADD		1

#define CODE_HELLO		1

static void usage_tp();

static int
__tp_information_func(int argc, char * const argv[], char *tp_uuid, struct mgrtp_interface *mgrtp_p)
{
	char *log_header = "__tp_information_func";
	char *cmd_code_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "stat")) {
			cmd_code = CODE_INFO_STAT;
		}
		else if(!strcmp(cmd_code_str, "all")) {
			cmd_code = CODE_INFO_ALL;
		}
	}

	if (cmd_code == CODE_INFO_STAT) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, tp_uuid=%s: information_stat",
			log_header, cmd_code, optind, tp_uuid);
		if (tp_uuid != NULL) {
			rc = mgrtp_p->t_op->tp_information_stat(mgrtp_p, tp_uuid);
		}
		else {
			usage_tp();
		}
	}
	else if (cmd_code == CODE_INFO_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, tp_uuid=%s: information_all",
                        log_header, cmd_code, optind, tp_uuid);
		rc = mgrtp_p->t_op->tp_information_all(mgrtp_p);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, tp_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, tp_uuid);
		usage_tp();
	}

	return rc;
}

static int
__tp_add_func(int argc, char * const argv[], char *tp_uuid, struct mgrtp_interface *mgrtp_p)
{
	char *log_header = "__tp_add_func";
	char *str;
	int rc = -1, num_workers, cmd_code = CODE_NONE;

	assert(argv);

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_tp();
		goto out;
	}
	if (tp_uuid == NULL) {
		usage_tp();
		goto out;
	}
	if (optind < argc) {
		str = argv[optind++];
		num_workers = atoi(str);
		if (!num_workers)
			goto out;
	}

	cmd_code = CMD_ADD;

	nid_log_warning("%s: cmd_code=%d, tp_uuid=%s, num_workers=%d, optind=%d: add",
		log_header, cmd_code, tp_uuid, num_workers, optind);
	rc = mgrtp_p->t_op->tp_add(mgrtp_p, tp_uuid, num_workers);

out:
	return rc;
}

static int
__tp_del_func(int argc, char * const argv[], char *tp_uuid, struct mgrtp_interface *mgrtp_p)
{
	char *log_header = "__tp_del_func";
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	if (!tp_uuid) {
		warn_str = "tp uuid cannot be null!";
		goto out;
	}

	if (optind < argc) {
		warn_str = "got too many arguments!";
		goto out;

	}

	if (strlen(tp_uuid) >= NID_MAX_UUID) {
		warn_str = "wrong tp uuid!";
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d, tp_uuid:%s: del",
		log_header, cmd_code, optind, tp_uuid);
	rc = mgrtp_p->t_op->tp_delete(mgrtp_p, tp_uuid);

out:
	if (rc) {
		usage_tp();
		nid_log_warning("%s: tp_uuid:%s: %s",
			log_header, tp_uuid, warn_str);
	}
	return rc;
}

static int
__tp_display_func(int argc, char* const argv[], char *tp_uuid, struct mgrtp_interface *mgrtp_p)
{
	char *log_header = "__tp_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_tp();
		goto out;
	}
	if (optind < argc) {
		cmd_str = argv[optind++];
	}
	if (tp_uuid != NULL && cmd_str != NULL) {
		if (strlen(tp_uuid) > NID_MAX_UUID) {
			nid_log_warning("%s: wrong tp uuid(%s)",
				log_header, tp_uuid);
			usage_tp();
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
	else if (tp_uuid == NULL && cmd_str != NULL){
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
		usage_tp();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, tp_uuid:%s: display",
			log_header, cmd_code, optind, tp_uuid);
		rc = mgrtp_p->t_op->tp_display(mgrtp_p, tp_uuid, is_setup);
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, tp_uuid:%s: display all",
			log_header, cmd_code, optind, tp_uuid);
		rc = mgrtp_p->t_op->tp_display_all(mgrtp_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, tp_uuid:%s: display",
			log_header, cmd_code, optind, tp_uuid);
		rc = mgrtp_p->t_op->tp_display(mgrtp_p, tp_uuid, is_setup);
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, tp_uuid:%s: display",
			log_header, cmd_code, optind, tp_uuid);
		rc = mgrtp_p->t_op->tp_display_all(mgrtp_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, tp_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, tp_uuid);
	}
out:
	return rc;
}

static int
__tp_hello_func(int argc, char * const argv[], char *tp_uuid, struct mgrtp_interface *mgrtp_p)
{
	char *log_header = "__tp_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(&tp_uuid);
	assert(argv);

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_tp();
		goto out;
	}

	cmd_code = CMD_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind=%d: hello",
		log_header, cmd_code, optind);
	rc = mgrtp_p->t_op->tp_hello(mgrtp_p);

out:
	return rc;
}

typedef int (*tp_operation_func)(int, char * const *, char *, struct mgrtp_interface *);

struct manager_tp_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	tp_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_tp_task tp_task_table[] = {
	{"information",	"info",		CMD_INFORMATION,	__tp_information_func,	NULL,	NULL},
	{"display",	"disp",		CMD_DISPLAY,		__tp_display_func,	NULL,	NULL},
	{"add",		"add",		CMD_ADD,		__tp_add_func,		NULL,	NULL},
	{"delete",	"del",		CMD_DELETE,		__tp_del_func,		NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,		__tp_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_info_setup_description(struct manager_tp_task *info_mtask)
{
	info_mtask->description = "info:	information display command\n";

	info_mtask->detail = x_malloc(4*1024);
	sprintf(info_mtask->detail, "%s",
		first_intend"stat:			display tp general status\n"
		first_intend"			sample:nidmanager -r s -m tp -i tp_uuid info stat\n"
		first_intend"all:			display all tp general status\n"
		first_intend"			sample:nidmanager -r s -m tp info all");
}

static void
__setup_display_setup_description(struct manager_tp_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of tp\n";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show tp configuration\n"
		first_intend"			sample:nidmanager -r s -m tp display setup\n"
		first_intend"			sample:nidmanager -r s -m tp -i tp_uuid display setup\n"
		first_intend"working:		show working sac configuration\n"
		first_intend"			sample:nidmanager -r s -m tp display working\n"
		first_intend"			sample:nidmanager -r s -m tp -i tp_uuid display working\n");
}

static void
__setup_add_setup_description(struct manager_tp_task *add_mtask)
{
	add_mtask->description = "add:	add a thread pool\n";

	add_mtask->detail = x_malloc(1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m tp -i tp_uuid add worker_number\n");
}

static void
__setup_del_setup_description(struct manager_tp_task *del_mtask)
{
	del_mtask->description = "del:	delete a thread pool\n";

	del_mtask->detail = x_malloc(1024);
	sprintf(del_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m tp -i tp_uuid del\n");
}

static void
__setup_hello_setup_description(struct manager_tp_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m tp hello\n");
}

static void
setup_description()
{
	struct manager_tp_task *mtask;

	mtask = &tp_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_INFORMATION)
			__setup_info_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DISPLAY)
			__setup_display_setup_description(mtask);
		else if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DELETE)
			__setup_del_setup_description(mtask);
		else if (mtask->cmd_index == CMD_HELLO)
			__setup_hello_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_tp()
{
	char *log_header = "usage_tp";
	struct manager_tp_task *task;
	int i;

	setup_description();
	
	printf("%s: nidmanager		-r s -m tp ", log_header);
	for (i = 0; i < CMD_MAX; i++) {
		task = &tp_task_table[i];
		if (task->cmd_long_name == NULL) {
			break;
		}
		printf("%s", task->cmd_short_name);
		if (tp_task_table[i + 1].cmd_long_name != NULL) {
			printf("|");
		}
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i ++) {
		task = &tp_task_table[i];
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

static char *opstr_tp = "a:i:";
static struct option long_options_tp[] = {
	{0, 0, 0, 0},
};

int
manager_server_tp(int argc, char * const argv[])
{
	char *log_header = "manager_server_tp";
	char c;
	int i;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";
	char *tp_uuid = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrtp_interface *mgrtp_p;
	struct mgrtp_setup setup;
	struct manager_tp_task *mtask = &tp_task_table[0];
	int rc = 0, cmd_index;

	umpk_p = x_calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	nid_log_debug("%s: start ...", log_header);
	while ((c = getopt_long(argc, argv, opstr_tp, long_options_tp, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		case 'i':
			tp_uuid = optarg;
			break;

		default:
			printf("the option has not been created \n");
			usage_tp();

			goto out;
		}
	}

	if (optind >= argc) {
		usage_tp();

		goto out;
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;

	mgrtp_p = x_calloc(1, sizeof(*mgrtp_p));
	mgrtp_initialization(mgrtp_p, &setup);

	/*
	 * get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_tp();
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
		usage_tp();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, tp_uuid, mgrtp_p);

out:
	return rc;
}
