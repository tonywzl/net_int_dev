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
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_doa_if.h"
#include "mgrdoa_if.h"
#include "mac_if.h"
#include "manager.h"

#define	CMD_LCK			1
#define CMD_HELLO		2
#define	CMD_MAX			3

#define	CODE_NONE		0

#define	CODE_LCK_REQ		1
#define CODE_LCK_CHK		2
#define CODE_LCK_REL		3

#define CODE_HELLO		1

static void usage_doa();

static int
__doa_lck_func(int argc, char * const argv[], struct mgrdoa_interface *mgrdoa_p, char *vid, char *lid, uint32_t time_out)
{
	char *log_header = "__doa_lck_func";
	char *cmd_code_str = NULL;
	struct umessage_doa_information res;
	int rc = 1, cmd_code = CODE_NONE;

	if (vid == NULL || lid == NULL) {
		usage_doa();
		goto out;
	}

	memset(&res, 0, sizeof(res));
	res.um_doa_vid_len = strlen(vid);
	memcpy(res.um_doa_vid, vid, res.um_doa_vid_len);

	res.um_doa_lid_len = strlen(lid);
	memcpy(res.um_doa_lid, lid, res.um_doa_lid_len);

	res.um_doa_time_out = time_out;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "request"))
			cmd_code = CODE_LCK_REQ;
		else if (!strcmp(cmd_code_str, "check"))
			cmd_code = CODE_LCK_CHK;
		else if (!strcmp(cmd_code_str, "release"))
			cmd_code = CODE_LCK_REL;
	}

	if (cmd_code == CODE_LCK_REQ) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, node_id=%s, lock_id=%s, timeout=%d: lock_request",
			log_header, cmd_code, optind, res.um_doa_vid, res.um_doa_lid, res.um_doa_time_out);
		rc = mgrdoa_p->md_op->md_doa_request(mgrdoa_p, &res);
	}
	else if (cmd_code == CODE_LCK_CHK) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, node_id=%s, lock_id=%s, timeout=%d: lock_check",
			log_header, cmd_code, optind, res.um_doa_vid, res.um_doa_lid, res.um_doa_time_out);
		rc = mgrdoa_p->md_op->md_doa_check(mgrdoa_p, &res);
	}
	else if (cmd_code == CODE_LCK_REL) {
		nid_log_warning("%s: cmd_code=%d, optind=%d, node_id=%s, lock_id=%s, timeout=%d: lock_release",
			log_header, cmd_code, optind, res.um_doa_vid, res.um_doa_lid, res.um_doa_time_out);
		rc = mgrdoa_p->md_op->md_doa_release(mgrdoa_p, &res);
	}

out:
	if (rc < 0)
		printf("network error\n");
	return rc;
}

static int
__doa_hello_func(int argc, char * const argv[], struct mgrdoa_interface *mgrdoa_p, char *vid, char *lid, uint32_t time_out)
{
	char *log_header = "__doa_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);
	assert(&vid);
	assert(&lid);
	assert(&time_out);

	nid_log_warning("this is hello.");

	if (optind > argc) {
		usage_doa();
		goto out;
	}

	cmd_code = CODE_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind=%d: hello",
		log_header, cmd_code, optind);
	rc = mgrdoa_p->md_op->md_doa_hello(mgrdoa_p);

out:
	return rc;
}

typedef int (*doa_operation_func)(int, char *const *, struct mgrdoa_interface *, char *, char *, uint32_t);

struct manager_doa_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	doa_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_doa_task doa_task_table [] = {
	{"lock",	"lck",		CMD_LCK,	__doa_lck_func,		NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,	__doa_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,	NULL,			NULL,	NULL},
};

static void
__setup_lck_setup_description(struct manager_doa_task *lck_mtask)
{
	lck_mtask->description = "lck:	volume lock command\n";

	lck_mtask->detail = x_malloc(4096);
	sprintf(lck_mtask->detail, "%s",
		first_intend"request:		request a lock, default timeout is 5\n"
		first_intend"			sample:nidmanager -r s -m doa -n node_id -l lock_id -t timeout lck request\n"
		first_intend"check:			check the information of a lock\n"
		first_intend"			sample:nidmanager -r s -m doa -n node_id -l lock_id lck check\n"
		first_intend"release:		release a lock\n"
		first_intend"			sample:nidmanager -r s -m doa -n node_id -l lock_id lck release\n");
}

static void
__setup_hello_setup_description(struct manager_doa_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r -s -m doa hello\n");
}

static void
setup_description()
{
	struct manager_doa_task *mtask;

	mtask = &doa_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_LCK)
			__setup_lck_setup_description(mtask);
		else if (mtask->cmd_index == CMD_HELLO)
			__setup_hello_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_doa()
{
	char *log_header = "usage_doa";
	struct manager_doa_task *task;
	int i;

	setup_description();
	
	printf("%s: nidmanager		-r s -m doa ", log_header);
	for (i = 0; i < CMD_MAX; i++) {
		task = &doa_task_table[i];
		if (task->cmd_long_name == NULL) {
			break;
		}
		printf("%s", task->cmd_short_name);
		if (doa_task_table[i + 1].cmd_long_name != NULL) {
			printf("|");
		}
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i ++) {
		task = &doa_task_table[i];
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

char *optstr_ss = "a:n:l:t:";

struct option long_options_ss[] = {
			{0, 0, 0, 0},
	};
int manager_server_doa (int argc, char * const argv[])
{

	char *log_header = "manager_doa";
	char c;
	int i;
	char *cmd_str;
	char *ipstr = "127.0.0.1";
	int rc = 0, cmd_index;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrdoa_interface *mgrdoa_p;
	struct mgrdoa_setup setup;
	struct manager_doa_task *mtask = doa_task_table;
	char *vid = NULL, *lid = NULL;
	uint32_t time_out = 5;

	umpk_p = x_calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);


	nid_log_debug("%s: start ...\n", log_header);
	while ((c=getopt_long(argc, argv, optstr_ss, long_options_ss, &i)) >= 0){
		switch (c){

			case 'a':
				ipstr = optarg;
				nid_log_debug("got ipaddr: %s", ipstr);
				break;
			case 'n':
				vid = optarg;
				nid_log_debug("request volume ID: %s", vid);
				break;
			case 'l':
				lid = optarg;
				nid_log_debug("lock ID: %s", lid);
				break;
			case 't':
				time_out = atoi (optarg);
				nid_log_debug("time out :%u", time_out);
				break;
			default:
				printf("the option has not been create\n");
				usage_doa();
				goto out;

		}
	}

	if (optind >= argc) {
		usage_doa();
		goto out;
	}
	
	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;

	mgrdoa_p = x_calloc(1, sizeof(*mgrdoa_p));
	mgrdoa_initialization(mgrdoa_p, &setup);	

	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_doa();
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
		usage_doa();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, mgrdoa_p, vid, lid, time_out);

out:
	return rc;
}
