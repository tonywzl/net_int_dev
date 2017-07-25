/*
 * manager_driver.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpka_if.h"
#include "mgrdriver_if.h"
#include "manager.h"

#define CMD_VERSION		1
#define	CMD_MAX			2

#define CODE_NONE		0

#define CODE_VERSION		1

static void usage_driver();

static int
__driver_version_func(int argc, char * const argv[], struct mgrdriver_interface *mgrdriver_p)
{
	char *log_header = "__driver_version_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_driver();
		goto out;
	}

	cmd_code = CMD_VERSION;

	nid_log_warning("%s: cmd_code=%d, optind=%d: version",
		log_header, cmd_code, optind);
	rc = mgrdriver_p->drv_op->driver_version(mgrdriver_p);

out:
	return rc;
}

typedef int (*driver_operation_func)(int, char * const *, struct mgrdriver_interface *);

struct manager_driver_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	driver_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_driver_task driver_task_table[] = {
	{"version",	"ver",		CMD_VERSION,		__driver_version_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_version_setup_description(struct manager_driver_task *version_mtask)
{
	version_mtask->description = "version:	show version of driver\n";

	version_mtask->detail = x_malloc(1024);
	sprintf(version_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r d -m driver version\n");
}

static void
setup_description()
{
	struct manager_driver_task *mtask;

	mtask = &driver_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_VERSION)
			__setup_version_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_driver()
{
	char *log_header = "usage_driver";
	struct manager_driver_task *task;
	int i;

	setup_description();
	
	printf("%s: nidmanager		-r d -m driver ", log_header);
	for (i = 0; i < CMD_MAX; i++) {
		task = &driver_task_table[i];
		if (task->cmd_long_name == NULL) {
			break;
		}
		printf("%s", task->cmd_short_name);
		if (driver_task_table[i + 1].cmd_long_name != NULL) {
			printf("|");
		}
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i ++) {
		task = &driver_task_table[i];
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

static char *opstr_driver = "a:i:";
static struct option long_options_driver[] = {
	{0, 0, 0, 0},
};

int
manager_driver_driver(int argc, char * const argv[])
{
	char *log_header = "manager_driver_driver";
	char c;
	int i;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";
	struct umpka_setup umpka_setup;
	struct umpka_interface *umpka_p;
	struct mgrdriver_setup setup;
	struct mgrdriver_interface *mgrdriver_p;
	struct manager_driver_task *mtask = &driver_task_table[0];
	int rc = 0, cmd_index;

	umpka_p = x_calloc(1, sizeof(*umpka_p));
	umpka_initialization(umpka_p, &umpka_setup);

	nid_log_debug("%s: start ...", log_header);
	while ((c = getopt_long(argc, argv, opstr_driver, long_options_driver, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		default:
			printf("the option has not been created \n");
			usage_driver();

			goto out;
		}
	}

	if (optind >= argc) {
		usage_driver();

		goto out;
	}

	setup.ipstr = ipstr;
	setup.port = NID_AGENT_PORT;
	setup.umpka = umpka_p;

	mgrdriver_p = x_calloc(1, sizeof(*mgrdriver_p));
	mgrdriver_initialization(mgrdriver_p, &setup);

	/*
	 * get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_driver();
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
		usage_driver();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, mgrdriver_p);

out:
	return rc;
}
