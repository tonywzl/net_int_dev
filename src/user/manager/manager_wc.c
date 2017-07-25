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
#include "mgrwc_if.h"
#include "mac_if.h"
#include "manager.h"


#define CMD_INFORMATION		1
#define CMD_HELLO		2
#define CMD_MAX			3

#define CODE_NONE		0

/* code of info */
#define CODE_INFO_FLUSHING	1
#define CODE_INFO_STAT		2

/* code of hello */
#define CODE_HELLO		1

static void usage_wc();

static int
__wc_information_func(int argc, char * const argv[], char *c_uuid, struct mgrwc_interface *mgrwc_p)
{
	char *log_header = "__wc_information_func";
	char *cmd_code_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "stat")) {
			cmd_code = CODE_INFO_STAT;
		}
		else if (!strcmp(cmd_code_str, "flushing")) {
			cmd_code = CODE_INFO_FLUSHING;
		}
	}

	if (cmd_code == CODE_INFO_STAT) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, c_uuid=%s: information_stat",
			log_header, cmd_code, optind, c_uuid);
		rc = mgrwc_p->wc_op->wc_information_stat(mgrwc_p, c_uuid);
	}
	else if (cmd_code == CODE_INFO_FLUSHING) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, c_uuid=%s: information_flushing",
			log_header, cmd_code, optind, c_uuid);
		rc = mgrwc_p->wc_op->wc_information_flushing(mgrwc_p, c_uuid);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: got wrong cmd_code",
				log_header, cmd_code, optind, c_uuid);
		usage_wc();
	}

	return rc;
}

static int
__wc_hello_func(int argc, char * const argv[], char *c_uuid, struct mgrwc_interface *mgrwc_p)
{
	char *log_header = "__wc_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);
	assert(&c_uuid);

	if (optind > argc) {
		usage_wc();
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind=%d: hello",
		log_header, cmd_code, optind);
	rc = mgrwc_p->wc_op->wc_hello(mgrwc_p);

out:
	return rc;
}

typedef int (*wc_operation_func)(int, char * const*, char *, struct mgrwc_interface *);

struct manager_wc_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	wc_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_wc_task wc_task_table[] = {
	{"informtaion",	"info",		CMD_INFORMATION,	__wc_information_func,	NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,		__wc_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_info_setup_description(struct manager_wc_task *info_mtask)
{
	info_mtask->description = "info:	information desplay command\n";

	info_mtask->detail = x_malloc(4096);
	sprintf(info_mtask->detail, "%s",
		first_intend"stat:			display stat\n"
		first_intend"			sample: nidmanager -r s -m wc -c c_uuid info stat\n\n"
		first_intend"flusing:		display flusing stat\n"
		first_intend"			sample: nidmanager -r s -m wc -c c_uuid info flushing\n");
}

static void
__setup_hello_setup_description(struct manager_wc_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m wc hello\n");
}

static void
setup_description()
{
	struct manager_wc_task *mtask;

	mtask = &wc_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_INFORMATION)
			__setup_info_setup_description(mtask);
		else if (mtask->cmd_index == CMD_HELLO)
			__setup_hello_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_wc()
{
	char *log_header = "usage_wc";
	struct manager_wc_task *task;
	int i;

	setup_description();
	
	printf("%s: nidmanager		-r s -m wc ",log_header);
	for (i = 0; i < CMD_MAX; i++) {
		task = &wc_task_table[i];
		if (task->cmd_long_name == NULL) {
			break;
		}
		printf("%s", task->cmd_short_name);
		if (wc_task_table[i + 1].cmd_long_name != NULL) {
			printf("|");
		}
	}

	printf("...\n");

	for (i = 0; i < CMD_MAX; i++) {
		task = &wc_task_table[i];
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

static char *optstr_ss = "a:c:";

static struct option long_options_ss[] = {
	{"vec", required_argument, NULL, 'v'},
	{0, 0, 0, 0},
};

int
manager_server_wc(int argc, char * const argv[])
{
	char *log_header = "manager_server_wc";
	char c;
	int i;
	int cmd_index;//, cmd_code = CODE_NONE;
	char *cmd_str = NULL;//, *cmd_code_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	char *c_uuid = NULL;		// all channels
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrwc_interface *mgrwc_p;
	struct mgrwc_setup setup;
	struct manager_wc_task *mtask = &wc_task_table[0];
	int rc = 0;

	umpk_p = x_calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	nid_log_debug("%s: start ...\n", log_header);
	while ((c = getopt_long(argc, argv, optstr_ss, long_options_ss, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		case 'c':
			c_uuid = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_wc();
			return 0;
		}
	}

	if (optind >= argc) {
		usage_wc();
		
		goto out;
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;

	mgrwc_p = x_calloc(1, sizeof(*mgrwc_p));
	mgrwc_initialization(mgrwc_p, &setup);

	/* get command */

	cmd_str = argv[optind++];

	if (cmd_str == NULL) {
		usage_wc();
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
		usage_wc();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, c_uuid, mgrwc_p);

/*
	if ( cmd_str != NULL && (!strcmp(cmd_str, "information") || !strcmp(cmd_str, "info"))) {
		cmd_index = CMD_INFORMATION;
	} else {
		usage_wc();
		rc = -1;
		goto out;
	}

	switch (cmd_index) {
	case CMD_INFORMATION:
		if (optind < argc) {
			cmd_code_str = argv[optind++];
			if (!strcmp(cmd_code_str, "flushing"))
				cmd_code = CODE_INFO_FLUSHING;
			else if (!strcmp(cmd_code_str, "stat"))
				cmd_code = CODE_INFO_STAT;
		}

		if (cmd_code == CODE_INFO_FLUSHING) {
			nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: iformation_flushing",
				log_header, cmd_code, optind, c_uuid);
			rc = mgrwc_p->wc_op->wc_information_flushing(mgrwc_p, c_uuid);
		} else if (cmd_code == CODE_INFO_STAT) {
			nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: information_stat",
				log_header, cmd_code, optind, c_uuid);
			rc = mgrwc_p->wc_op->wc_information_stat(mgrwc_p, c_uuid);
		} else {
			nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: got wrong cmd_code",
				log_header, cmd_code, optind, c_uuid);
		}
		break;

	default:
		rc = -1;
		goto out;
	}
*/
out:
	return rc;
}
