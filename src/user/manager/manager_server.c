/*
 * manager_server.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrserver_if.h"
#include "manager.h"

#define CMD_VERSION		1
#define	CMD_MAX			2

#define CODE_NONE		0

#define CODE_VERSION		1

static void usage_server();

static int
__server_version_func(int argc, char * const argv[], struct mgrserver_interface *mgrserver_p)
{
	char *log_header = "__server_version_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_server();
		goto out;
	}

	cmd_code = CMD_VERSION;

	nid_log_warning("%s: cmd_code=%d, optind=%d: version",
		log_header, cmd_code, optind);
	rc = mgrserver_p->ser_op->server_version(mgrserver_p);

out:
	return rc;
}

typedef int (*server_operation_func)(int, char * const *, struct mgrserver_interface *);

struct manager_server_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	server_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_server_task server_task_table[] = {
	{"version",	"ver",		CMD_VERSION,		__server_version_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_version_setup_description(struct manager_server_task *version_mtask)
{
	version_mtask->description = "version:	show version of server\n";

	version_mtask->detail = x_malloc(1024);
	sprintf(version_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m server version\n");
}

static void
setup_description()
{
	struct manager_server_task *mtask;

	mtask = &server_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_VERSION)
			__setup_version_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_server()
{
	char *log_header = "usage_server";
	struct manager_server_task *task;
	int i;

	setup_description();
	
	printf("%s: nidmanager		-r s -m server ", log_header);
	for (i = 0; i < CMD_MAX; i++) {
		task = &server_task_table[i];
		if (task->cmd_long_name == NULL) {
			break;
		}
		printf("%s", task->cmd_short_name);
		if (server_task_table[i + 1].cmd_long_name != NULL) {
			printf("|");
		}
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i ++) {
		task = &server_task_table[i];
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

static char *opstr_server = "a:i:";
static struct option long_options_server[] = {
	{0, 0, 0, 0},
};

int
manager_server_server(int argc, char * const argv[])
{
	char *log_header = "manager_server_server";
	char c;
	int i;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrserver_setup setup;
	struct mgrserver_interface *mgrserver_p;
	struct manager_server_task *mtask = &server_task_table[0];
	int rc = 0, cmd_index;

	umpk_p = x_calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	nid_log_debug("%s: start ...", log_header);
	while ((c = getopt_long(argc, argv, opstr_server, long_options_server, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		default:
			printf("the option has not been created \n");
			usage_server();

			goto out;
		}
	}

	if (optind >= argc) {
		usage_server();

		goto out;
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;

	mgrserver_p = x_calloc(1, sizeof(*mgrserver_p));
	mgrserver_initialization(mgrserver_p, &setup);

	/*
	 * get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_server();
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
		usage_server();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, mgrserver_p);

out:
	return rc;
}
