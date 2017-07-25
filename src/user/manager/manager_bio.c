/*
 * debug_Ser_bio.c
 *
 *  Created on: Jun 19, 2015
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "version.h"
#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "msc_if.h"
#include "mac_if.h"
#include "manager.h"


#define CMD_RELEASE_WCACHE	1
#define CMD_VEC_INFO		2
#define CMD_MAX			3

#define CODE_NONE		0

#define CODE_REL_START		1
#define CODE_REL_STOP		2

#define CODE_VEC_START		1
#define CODE_VEC_STOP		2
#define CODE_VEC_STATUS		3
static void usage_cache();

static int
__bio_release_wcache_func(int argc, char * const argv[], struct msc_interface *msc_p )
{
	char *log_header = "__bio_release_wcache_func";
	char *cmd_code_str = NULL;
	char *chan_uuid = NULL;
	int rc = -1, cmd_code = CODE_NONE;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start"))
			cmd_code = CODE_REL_START;
		else if (!strcmp(cmd_code_str, "stop"))
			cmd_code = CODE_REL_STOP;
	}

	if (optind < argc) {
		chan_uuid = argv[optind++];
	}
	else {
		usage_cache();
		rc = -1;
		goto out;
	}

	if (cmd_code == CODE_REL_START) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, chan_uuid=%s: release_start",
			log_header, cmd_code, optind, chan_uuid);
			rc = msc_p->m_op->m_bio_release_start(msc_p, chan_uuid);
	}
	else if (cmd_code == CODE_REL_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, chan_uuid=%s: release_stop",
			log_header, cmd_code, optind, chan_uuid);
		rc = msc_p->m_op->m_bio_release_stop(msc_p, chan_uuid);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, chan_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, chan_uuid);
		usage_cache();
	}

out:
	return rc;
}

static int
__bio_vec_info_func(int argc, char * const argv[], struct msc_interface *msc_p )
{
	char *log_header = "__bio_vec_info_func";
	char *cmd_code_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;

	if (optind <argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start"))
			cmd_code = CODE_VEC_START;
		else if (!strcmp(cmd_code_str, "stop"))
			cmd_code = CODE_VEC_STOP;
		else if (!strcmp(cmd_code_str, "status"))
			cmd_code = CODE_VEC_STATUS;
	}

	if (cmd_code == CODE_VEC_START) {
		nid_log_warning("%s: cmd_code=%d, optind=%d: vec_info_start",
			log_header, cmd_code, optind);
		rc = msc_p->m_op->m_bio_vec_start(msc_p);
		printf("Starts to counte vec info!\n");
	}
	else if (cmd_code == CODE_VEC_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind=%d: vec_info_stop",
			log_header, cmd_code, optind);
		rc = msc_p->m_op->m_bio_vec_stop(msc_p);
		rc = msc_p->m_op->m_bio_vec_stat(msc_p);
		printf("Stop to counte vec info!\n");

	}
	else if (cmd_code == CODE_VEC_STATUS) {
		nid_log_warning("%s: cmd_code=%d, optind=%d: vec_info_status",
			log_header, cmd_code, optind);
		rc = msc_p->m_op->m_bio_vec_stat(msc_p);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d: got wrong cmd_code",
			log_header, cmd_code, optind);
		usage_cache();
	}

	return rc;
}

typedef int (*bio_operation_func)(int, char * const *, struct msc_interface *);

struct manager_bio_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	bio_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_bio_task bio_task_table[] = {
	{"release",	"rel",	CMD_RELEASE_WCACHE,	__bio_release_wcache_func,	NULL,	NULL},
	{"vector",	"vec",	CMD_VEC_INFO,		__bio_vec_info_func,		NULL,	NULL},
	{NULL,		NULL,	CMD_MAX,		NULL,				NULL,	NULL},
};

static void
__setup_rel_setup_description(struct manager_bio_task *rel_mtask)
{
	rel_mtask->description = "release:	bio release command\n";

	rel_mtask->detail = x_malloc(4096);
	sprintf(rel_mtask->detail, "%s",
		first_intend"start:			bio release start\n"
		first_intend"			sample: nidmanager -r s -m cache rel start channel_uuid\n"
		first_intend"stop:			bio release stop\n"
		first_intend"			sample: nidmanager -r s -m cache rel stop channel_uuid\n");
}

static void
__setup_vec_setup_description(struct manager_bio_task *vec_mtask)
{
	vec_mtask->description = "vec:		vector information command\n";

	vec_mtask->detail = x_malloc(4096);
	sprintf(vec_mtask->detail, "%s",
		first_intend"start:			start to count vec info\n"
		first_intend"			sample: nidmanager -r s -m cache vec start\n"
		first_intend"stop:			stop count vec info\n"
		first_intend"			sample: nidmanager -r s -m cache vec stop\n"
		first_intend"status:			show vec status\n"
		first_intend"			sample: nidmanager -r s -m cache vec status\n");
}

static void
setup_description()
{
	struct manager_bio_task *mtask;

	mtask = &bio_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_RELEASE_WCACHE)
			__setup_rel_setup_description(mtask);
		else if (mtask->cmd_index == CMD_VEC_INFO)
			__setup_vec_setup_description(mtask);
		else
			assert(0);

		mtask++;
	}
}

void
usage_cache()
{
/*
	printf("usage_cache: cache module command [options]\n");
	printf("Options:\n");
	printf("  -a    IP address of server/agent/driver\n");
	printf("  --release|-r start|stop channel_uuid\n");
	printf("  --vec|-v    iovec info: start/stop/status \n");
*/
	char *log_header = "usage_cache";
	struct manager_bio_task *task;
	int i;

	setup_description();

	printf("%s: nidmanager		-r s -m cache ", log_header);
	for (i = 0; i < CMD_MAX; i++) {
		task = &bio_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s", task->cmd_short_name);
		if (bio_task_table[i + 1].cmd_long_name != NULL)
			printf("|");
	}

	printf(" ...\n");

	for (i = 0; i < CMD_MAX; i++) {
		task = &bio_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		if (task->description && task->detail){
			printf("%s\n", task->description);
			printf("%s\n", task->detail);
		}
	}
	printf("\n");

	return;
}

static char *optstr_ss = "a:";



static struct option long_options_ss[] = {
	{"vec", required_argument, NULL, 'v'},
	{"release", required_argument, NULL, 'r'},
	{0, 0, 0, 0},
};

int
manager_server_bio(int argc, char * const argv[])
{
	char *log_header = "manager_server_bio";
	char c;
	int i;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	struct mpk_setup mpk_setup;
	struct mpk_interface *mpk_p;
	struct msc_interface *msc_p;
	struct msc_setup setup;
	struct manager_bio_task *mtask = &bio_task_table[0];
	int rc = 0, cmd_index;

	mpk_setup.type = 0;
	mpk_p = x_calloc(1, sizeof(*mpk_p));
	mpk_initialization(mpk_p, &mpk_setup);

	nid_log_debug("%s: start ...\n", log_header);
	while ((c = getopt_long(argc, argv, optstr_ss, long_options_ss, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			nid_log_debug("got ipaddr: %s", ipstr);
			break;

		default:
			printf("the option has not been create\n");
			usage_cache();
			goto out;
		} 
	}

	if (optind >= argc) {
		usage_cache();

		goto out;
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.mpk = mpk_p;

	msc_p = x_calloc(1, sizeof(*msc_p));
	msc_initialization(msc_p, &setup);

	/*
	 * get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_cache();
		rc = -1;
		goto out;
	}

	cmd_index = CMD_MAX;
	while (mtask->cmd_index < CMD_MAX){
		if (!strcmp(cmd_str, mtask->cmd_long_name) || !strcmp(cmd_str, mtask->cmd_short_name)) {
			cmd_index = mtask->cmd_index;
			break;
		}
		mtask++;
	}

	if (cmd_index == CMD_MAX) {
		usage_cache();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, msc_p);
/*	switch (cmd_index) {
	case CMD_RELEASE_WCACHE:
		nid_log_warning("%s: cmd_code=%d, chan_uuid:%s", log_header, cmd_code, chan_uuid); 	
		if (cmd_code == CODE_START)
			rc = msc_p->m_op->m_bio_release_start(msc_p, chan_uuid);
		else
			rc = msc_p->m_op->m_bio_release_stop(msc_p, chan_uuid);

		return 0;
	}

	if (!strcmp(bio_cmd, "start")) {
		rc = msc_p->m_op->m_bio_vec_start(msc_p);
		printf("Starts to counte vec info!\n");
	} else if (!strcmp(bio_cmd, "stop")) {
		rc = msc_p->m_op->m_bio_vec_stop(msc_p);
		rc = msc_p->m_op->m_bio_vec_stat(msc_p);
		printf("Stop to counte vec info!\n");
	} else if(!strcmp(bio_cmd,"status")||!strcmp(bio_cmd, "stat")) {
		rc = msc_p->m_op->m_bio_vec_stat(msc_p);
	}
*/
out:
	return rc;
}
