/*
 * manager_twc.c
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
#include "mgrtwc_if.h"
#include "mac_if.h"
#include "manager.h"


#define CMD_INFORMATION		1
#define CMD_RESET		2
#define CMD_DISPLAY		3
#define CMD_HELLO		4
#define CMD_MAX			5

#define CODE_NONE		0

/* code of info */
#define CODE_INFO_FLUSHING	1
#define CODE_INFO_STAT		2
#define CODE_INFO_THROUGHPUT	3
#define CODE_INFO_RW		4

/* code of throughput */
#define CODE_THPT_RESET		1

/* code of display */
#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_W_DIS		3
#define CODE_W_DIS_ALL		4

/* code of hello */
#define CODE_HELLO		1

void usage_twc ();

static int
__twc_information_func(int argc, char* const argv[], char *twc_uuid, char *chan_uuid, struct mgrtwc_interface *mgrtwc_p)
{
	char *log_header = "__twc_information_func";
	char *cmd_code_str = NULL;
	int rc  = -1, cmd_code = CODE_NONE;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "flushing"))
			cmd_code = CODE_INFO_FLUSHING;
		else if (!strcmp(cmd_code_str, "stat"))
			cmd_code = CODE_INFO_STAT;
		else if (!strcmp(cmd_code_str, "throughput")|| !strcmp(cmd_code_str, "tpt"))
			cmd_code = CODE_INFO_THROUGHPUT;
		else if (!strcmp(cmd_code_str, "rw"))
			cmd_code = CODE_INFO_RW;
	}

	if (cmd_code == CODE_INFO_FLUSHING) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mtwc_uuid:%s: iformation_flushing",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_information_flushing(mgrtwc_p, twc_uuid);

	} else if (cmd_code == CODE_INFO_STAT) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mtwc_uuid:%s: information_stat",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_information_stat(mgrtwc_p, twc_uuid);

	} else if (cmd_code == CODE_INFO_THROUGHPUT) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mtwc_uuid:%s: information_stat",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_information_throughput_stat(mgrtwc_p, twc_uuid);

	} else if (cmd_code == CODE_INFO_RW) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mtwc_uuid:%s: information_stat",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_information_rw_stat(mgrtwc_p, twc_uuid, chan_uuid);

	} else {
		nid_log_warning("%s: cmd_code=%d, optind:%d mtwc_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, twc_uuid);
		usage_twc();
	}

	return rc;
}

static int
__twc_reset_func(int argc, char* const argv[], char *twc_uuid, char *chan_uuid, struct mgrtwc_interface *mgrtwc_p)
{
	char *log_header = "__twc_reset_func";
	char *cmd_code_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;

	assert(!chan_uuid);
	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "reset"))
			cmd_code = CODE_THPT_RESET;
	}

	if (cmd_code == CODE_THPT_RESET) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mtwc_uuid:%s: iformation_flushing",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_throughput_stat_reset(mgrtwc_p, twc_uuid);

	} else {
		nid_log_warning("%s: cmd_code=%d, optind:%d mtwc_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, twc_uuid);
		usage_twc ();
	}

	return rc;
}

static int
__twc_display_func(int argc, char* const argv[], char *twc_uuid, char *chan_uuid, struct mgrtwc_interface *mgrtwc_p)
{
	char *log_header = "__twc_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	assert(!chan_uuid);

	if (optind >= argc) {
		warn_str = "got empty sub command!";
		usage_twc();
		goto out;
	}

	cmd_str = argv[optind++];

	if (twc_uuid != NULL && cmd_str != NULL) {
		if (strlen(twc_uuid) > NID_MAX_UUID) {
			warn_str = "wrong twc_uuid!";
			usage_twc();
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
	else if (twc_uuid == NULL && cmd_str != NULL){
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
		usage_twc();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, twc_uuid:%s: display",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_display(mgrtwc_p, twc_uuid, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, twc_uuid:%s: display all",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_display_all(mgrtwc_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, twc_uuid:%s: display",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_display(mgrtwc_p, twc_uuid, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, twc_uuid:%s: display",
			log_header, cmd_code, optind, twc_uuid);
		rc = mgrtwc_p->tw_op->tw_display_all(mgrtwc_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, twc_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, twc_uuid);
	}

out:
	if (rc) {
		nid_log_warning("%s: twc_uuid:%s: %s",
			log_header, twc_uuid, warn_str);
	}
	return rc;
}

static int
__twc_hello_func(int argc, char* const argv[], char *twc_uuid, char *chan_uuid, struct mgrtwc_interface *mgrtwc_p)
{
	char *log_header = "__twc_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(&chan_uuid);
	assert(&twc_uuid);
	assert(argv);

	if (optind > argc) {
		usage_twc();
		goto out;
	}

	cmd_code = CODE_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind:%d mtwc_uuid:%s: iformation_flushing",
		log_header, cmd_code, optind, twc_uuid);
	rc = mgrtwc_p->tw_op->tw_hello(mgrtwc_p);

out:
	return rc;
}

typedef int (*twc_operation_func)(int, char* const * , char *, char *, struct mgrtwc_interface *);
struct manager_twc_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	twc_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_twc_task twc_task_table[] = {
	{"information",	"info",		CMD_INFORMATION,	__twc_information_func,	NULL,	NULL},
	{"reset",	"reset",	CMD_RESET,		__twc_reset_func,	NULL,	NULL},
	{"display",	"disp",		CMD_DISPLAY,		__twc_display_func,	NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,		__twc_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_twc_info_description(struct manager_twc_task *dc_mtask)
{
	dc_mtask->description = "info:	get twc work information\n";
	dc_mtask->detail = x_malloc(1024*16);
	sprintf(dc_mtask->detail, "%s",
		first_intend"stat:			display twc gerenal status\n"
		first_intend"			sample:nidmanager -r s -m twc -i twc_uuid info stat\n"
		first_intend"throughput|tpt:		display twc throughput information\n"
		first_intend"			sample:nidmanager -r s -m twc -i twc_uuid info throughput|tpt\n"
		first_intend"rw:			display twc flush related information (overwritten & coalesce)\n"
		first_intend"			sample:nidmanager -r s -m twc -i twc_uuid -c channel_uuid info rw\n");
}
/*
static void
__setup_twc_reset_description(struct manager_twc_task *dc_mtask)
{
	dc_mtask->description = "reset:	reset twc throughput info\n";

	dc_mtask->detail = x_malloc(1024*16);
	sprintf(dc_mtask->detail, "%s",
		first_intend"reset:			reset twc throughput counter\n"
		first_intend"			sample:nidmanager -r s -m twc tpt reset  -i twc_uuid\n");
}
*/
static void
__setup_twc_display_description(struct manager_twc_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of twc";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show twc configuration\n"
		first_intend"			sample:nidmanager -r s -m twc display setup\n"
		first_intend"			sample:nidmanager -r s -m twc -i twc_uuid display setup\n"
		first_intend"working:		show working twc configuration\n"
		first_intend"			sample:nidmanager -r s -m twc display working\n"
		first_intend"			sample:nidmanager -r s -m twc -i twc_uuid display working\n");
}

static void
__setup_twc_hello_description(struct manager_twc_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m twc hello\n");
}

static void
setup_descriptions()
{
	struct manager_twc_task *mtask;

	mtask = &twc_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_INFORMATION)
			__setup_twc_info_description(mtask);
		else if (mtask->cmd_index == CMD_RESET)
			assert(1);
//			__setup_twc_reset_description(mtask);
		else if (mtask->cmd_index == CMD_DISPLAY)
			__setup_twc_display_description(mtask);
		else if (mtask->cmd_index == CMD_HELLO)
			__setup_twc_hello_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_twc()
{
	struct manager_twc_task *task;
	int i;

	setup_descriptions();

	for (i = 0 ; i < CMD_MAX; i++){
		task = &twc_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (twc_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &twc_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		if(task->description && task->detail) {
			printf("%s\n",task->description );
			printf("%s\n",task->detail);
		}

	}
	printf ("\n");
}

static char *optstr_ss = "a:i:c:";

static struct option long_options_ss[] = {
	{"vec", required_argument, NULL, 'v'},
	{0, 0, 0, 0},
};

int
manager_server_twc(int argc, char * const argv[])
{
	char *log_header = "manager_server_twc";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	char *twc_uuid = NULL;
	char *chan_uuid = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrtwc_interface *mgrtwc_p;
	struct mgrtwc_setup setup;
	struct manager_twc_task *mtask = &twc_task_table[0];
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
			twc_uuid = optarg;
			break;

		case 'c':
			chan_uuid = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_twc();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrtwc_p = x_calloc(1, sizeof(*mgrtwc_p));
	mgrtwc_initialization(mgrtwc_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_twc();
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
		usage_twc();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, twc_uuid, chan_uuid, mgrtwc_p);

out:
	return rc;
}
