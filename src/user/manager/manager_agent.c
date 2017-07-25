/*
 * manager_agent.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpka_if.h"
#include "mgragent_if.h"
#include "manager.h"

#define CMD_VERSION		1
#define	CMD_MAX			2

#define CODE_NONE		0

#define CODE_VERSION		1

static void usage_agent();

static int
__agent_version_func(int argc, char * const argv[], struct mgragent_interface *mgragent_p)
{
	char *log_header = "__agent_version_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_agent();
		goto out;
	}

	cmd_code = CMD_VERSION;

	nid_log_warning("%s: cmd_code=%d, optind=%d: version",
		log_header, cmd_code, optind);
	rc = mgragent_p->agt_op->agent_version(mgragent_p);

out:
	return rc;
}

typedef int (*agent_operation_func)(int, char * const *, struct mgragent_interface *);

struct manager_agent_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	agent_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_agent_task agent_task_table[] = {
	{"version",	"ver",		CMD_VERSION,		__agent_version_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_version_setup_description(struct manager_agent_task *version_mtask)
{
	version_mtask->description = "version:	show version of agent\n";

	version_mtask->detail = x_malloc(1024);
	sprintf(version_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r a -m agent version\n");
}

static void
setup_description()
{
	struct manager_agent_task *mtask;

	mtask = &agent_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_VERSION)
			__setup_version_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_agent()
{
	char *log_header = "usage_agent";
	struct manager_agent_task *task;
	int i;

	setup_description();
	
	printf("%s: nidmanager		-r a -m agent ", log_header);
	for (i = 0; i < CMD_MAX; i++) {
		task = &agent_task_table[i];
		if (task->cmd_long_name == NULL) {
			break;
		}
		printf("%s", task->cmd_short_name);
		if (agent_task_table[i + 1].cmd_long_name != NULL) {
			printf("|");
		}
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i ++) {
		task = &agent_task_table[i];
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

static char *opstr_agent = "a:i:";
static struct option long_options_agent[] = {
	{0, 0, 0, 0},
};

int
manager_agent_agent(int argc, char * const argv[])
{
	char *log_header = "manager_agent_agent";
	char c;
	int i;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";
	struct umpka_setup umpka_setup;
	struct umpka_interface *umpka_p;
	struct mgragent_setup setup;
	struct mgragent_interface *mgragent_p;
	struct manager_agent_task *mtask = &agent_task_table[0];
	int rc = 0, cmd_index;

	umpka_p = x_calloc(1, sizeof(*umpka_p));
	umpka_initialization(umpka_p, &umpka_setup);

	nid_log_debug("%s: start ...", log_header);
	while ((c = getopt_long(argc, argv, opstr_agent, long_options_agent, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		default:
			printf("the option has not been created \n");
			usage_agent();

			goto out;
		}
	}

	if (optind >= argc) {
		usage_agent();

		goto out;
	}

	setup.ipstr = ipstr;
	setup.port = NID_AGENT_PORT;
	setup.umpka = umpka_p;

	mgragent_p = x_calloc(1, sizeof(*mgragent_p));
	mgragent_initialization(mgragent_p, &setup);

	/*
	 * get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_agent();
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
		usage_agent();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, mgragent_p);

out:
	return rc;
}
