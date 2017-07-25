/*
 * manager-sac.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrsac_if.h"
#include "manager.h"

#define CMD_INFORMATION		1
#define CMD_ADD			2
#define CMD_DELETE		3
#define CMD_SWITCH		4
#define CMD_SET_KEEPALIVE	5
#define CMD_FAST_RELEASE	6
#define CMD_IOINFO		7
#define CMD_DISPLAY		8
#define CMD_HELLO		9
#define CMD_MAX			10

#define CODE_NONE		0

/* for cmd information */
#define CODE_STAT		1
#define CODE_STAT_ALL		2
#define CODE_LIST_STAT		3

/* for cmd fast release */
#define CODE_START		1
#define CODE_STOP		2

/* for cmd ioinfo */
#define CODE_IOINFO_START	1
#define CODE_IOINFO_START_ALL	2
#define CODE_IOINFO_STOP	3
#define CODE_IOINFO_STOP_ALL	4
#define CODE_IOINFO_CHECK	5
#define CODE_IOINFO_CHECK_ALL	6

/* for cmd display */
#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_C_DIS		3
#define CODE_C_DIS_ALL		4

/* for cmd hello */
#define CODE_HELLO		1

static void usage_sac();

static int
__sac_information_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_information_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_sac();
		goto out;
	}
	if (optind < argc) {
		cmd_str = argv[optind++];
	}
	if (chan_uuid != NULL && cmd_str != NULL) {
		if (strlen(chan_uuid) > NID_MAX_UUID) {
			nid_log_warning("%s: wrong sac uuid(%s)",
				log_header, chan_uuid);
			usage_sac();
			goto out;
		}
		if (!strcmp(cmd_str, "stat")) {
			cmd_code = CODE_STAT;
		} else if (!strcmp(cmd_str, "list")) {
			cmd_code = CODE_LIST_STAT;
		}
	}
	else if (chan_uuid == NULL && cmd_str != NULL) {
		if (!strcmp(cmd_str, "stat"))
			cmd_code = CODE_STAT_ALL;
	}
	else {
		usage_sac();
		goto out;
	}

	if (cmd_code == CODE_STAT) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sac_uuid:%s: stat",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_information(mgrsac_p, chan_uuid);
	} else if (cmd_code == CODE_STAT_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d: stat all",
			log_header, cmd_code, optind);
		rc = mgrsac_p->sa_op->sa_information_all(mgrsac_p);
	} else if (cmd_code == CODE_LIST_STAT) {
		nid_log_warning("%s: cmd_code=%d, optind:%d: list stat",
			log_header, cmd_code, optind);
		rc = mgrsac_p->sa_op->sa_list_stat(mgrsac_p, chan_uuid);
	} else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, sac_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, chan_uuid);
		usage_sac();
	}
out:
	return rc;
}

static int
__sac_add_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_add_func";
	char *chan_conf;
	char args[9][1024];
	char *ds_name, *dev_name, *exportname, *tp_name;
	int n, sync, direct_io, enable_kill_myself, alignment, dev_size = 0;
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	if (!chan_uuid) {
		goto out;
	}

	if (strlen(chan_uuid) >= NID_MAX_UUID) {
		warn_str = "wrong sac uuid!";
		goto out;
	}

	if (optind >= argc) {
		warn_str = "got empty sac configuration!";
		goto out;
	}

	chan_conf = argv[optind++];
	n = sscanf(chan_conf, "%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]",
			args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
	if (n != 8 && n != 9) {
		warn_str = "wrong number of sac configuration items!";
		goto out;
	}
	sync = atoi(args[0]);
	if (sync != 0 && sync != 1) {
		warn_str = "wrong sync configuration!";
		goto out;
	}

	direct_io = atoi(args[1]);
	if (direct_io != 0 && direct_io != 1) {
		warn_str = "wrong direct_io configuration!";
		goto out;
	}

	enable_kill_myself = atoi(args[2]);
	if (enable_kill_myself != 0 && enable_kill_myself != 1) {
		warn_str = "wrong enable_kill_myself configuration!";
		goto out;
	}

	alignment = atoi(args[3]);
	if (((~(alignment - 1)) & alignment) != alignment) {
		warn_str = "wrong alignment configuration!";
		goto out;
	}

	tp_name = args[4];
	if (strlen(tp_name) >= NID_MAX_TPNAME) {
		warn_str = "wrong tp_name configuration!";
		goto out;
	}

	ds_name = args[5];
	if (strlen(ds_name) >= NID_MAX_DSNAME) {
		warn_str = "wrong ds_name configuration!";
		goto out;
	}

	dev_name = args[6];
	if (strlen(dev_name) >= NID_MAX_DEVNAME) {
		warn_str = "wrong dev_name configuration!";
		goto out;
	}

	exportname = args[7];
	if (strlen(exportname) >= NID_MAX_PATH) {
		warn_str = "wrong exportname configuration!";
		goto out;
	}

	/* mrw case, need dev_size */
	if (n == 9) {
		dev_size = atoi(args[8]);
		if (dev_size <= 0) {
			warn_str = "wrong dev_size configuration!";
			goto out;
		}
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d sac_uuid:%s",
				log_header, cmd_code, optind, chan_uuid);
	rc = mgrsac_p->sa_op->sa_add(mgrsac_p, chan_uuid, (uint8_t)sync, (uint8_t)direct_io, (uint8_t)enable_kill_myself,
			(uint32_t)alignment, ds_name, dev_name, exportname, (uint32_t)dev_size, tp_name);
	return rc;

out:
	if (rc) {
		usage_sac();
		nid_log_warning("%s: sac_uuid:%s: %s",
			log_header, chan_uuid, warn_str);
	}
	return rc;
}

static int
__sac_del_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_del_func";
	int rc  = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	if (!chan_uuid) {
		warn_str = "sac uuid cannot be null!";
		goto out;
	}

	if (optind < argc) {
		warn_str = "got too many arguments!";
		goto out;

	}

	if (strlen(chan_uuid) >= NID_MAX_UUID) {
		warn_str = "wrong sac uuid!";
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d sac_uuid:%s",
				log_header, cmd_code, optind, chan_uuid);
	rc = mgrsac_p->sa_op->sa_delete(mgrsac_p, chan_uuid);
	return rc;

out:
	if (rc) {
		usage_sac();
		nid_log_warning("%s: sac_uuid:%s: %s",
			log_header, chan_uuid, warn_str);
	}
	return rc;
}

static int
__sac_swtich_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_swtich_func";
	char *bwc_uuid;
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	if (!chan_uuid) {
		goto out;
	}

	if (optind < argc) {
		if (strlen(chan_uuid) >= NID_MAX_UUID) {
			warn_str = "wrong sac uuid!";
			goto out;
		}
		bwc_uuid = argv[optind++];
		if (strlen(bwc_uuid) >= NID_MAX_UUID) {
			warn_str = "wrong bwc uuid!";
			goto out;
		}
		nid_log_warning("%s: cmd_code=%d, optind:%d sac_uuid:%s",
					log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_switch_bwc(mgrsac_p, chan_uuid, bwc_uuid);
		return rc;
	}

	warn_str = "got empty bwc uuid!";

out:
	if (rc) {
		usage_sac();
		nid_log_warning("%s: sac_uuid:%s: %s",
			log_header, chan_uuid, warn_str);
	}
	return rc;
}

static int
__sac_set_keepalive_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_stop_keepalive_func";
	int rc = -1, cmd_code = CODE_NONE, opt_idx = 0;
	char *warn_str = "";
	uint16_t keepalive_enable, max_keepalive;

	assert(argv);
	if (!chan_uuid) {
		goto out;
	}

	while(opt_idx < argc) {
		if(strcmp(argv[opt_idx], "skp")) {
			opt_idx++;
			continue;
		}

		keepalive_enable = atoi(argv[opt_idx + 1]);
		if(keepalive_enable) {
			if(argv[opt_idx + 2]) {
				max_keepalive = atoi(argv[opt_idx + 2]);
			} else {
				max_keepalive = 20;
			}

			if(max_keepalive <= 0) {
				warn_str = "invalid keepalive setting!";
				goto out;
			}
		}

		if (strlen(chan_uuid) >= NID_MAX_UUID) {
			warn_str = "wrong sac uuid!";
			goto out;
		}

		nid_log_warning("%s: cmd_code=%d, optind:%d sac_uuid:%s",
					log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_set_keepalive(mgrsac_p, chan_uuid, keepalive_enable, max_keepalive);
		return rc;
	}

out:
	if (rc) {
		usage_sac();
		nid_log_warning("%s: sac_uuid:%s: %s",
			log_header, chan_uuid, warn_str);
	}
	return rc;
}

static int
__sac_fast_release_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_fast_release_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_start = 0;

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_sac();
		goto out;
	}
	if (optind < argc) {
		cmd_str = argv[optind++];
	}
	if (chan_uuid != NULL && cmd_str != NULL) {
		if (strlen(chan_uuid) > NID_MAX_UUID) {
			nid_log_warning("%s: wrong sac uuid(%s)",
				log_header, chan_uuid);
			usage_sac();
			goto out;
		}
		if (!strcmp(cmd_str, "start")) {
			cmd_code = CODE_START;
			is_start = 1;
		}
		else if (!strcmp(cmd_str, "stop")) {
			cmd_code = CODE_STOP;
			is_start = 0;
		}
	}
	else {
		usage_sac();
		goto out;
	}

	if (cmd_code == CODE_START) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sac_uuid:%s: fast_release",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_fast_release(mgrsac_p, chan_uuid, is_start);
	}
	else if (cmd_code == CODE_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sac_uuid:%s: fast_release",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_fast_release(mgrsac_p, chan_uuid, is_start);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, sac_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, chan_uuid);
		usage_sac();
	}
out:
	return rc;
}

static int
__sac_ioinfo_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_ioinfo_func";
	char *cmd_code_str = NULL;
	int rc  = 0, cmd_code = CODE_NONE;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start") && chan_uuid)
			cmd_code = CODE_IOINFO_START;
		else if (!strcmp(cmd_code_str, "stop") && chan_uuid)
			cmd_code = CODE_IOINFO_STOP;
		else if (!strcmp(cmd_code_str, "check") && chan_uuid)
			cmd_code = CODE_IOINFO_CHECK;
		else if (!strcmp(cmd_code_str, "start") && !chan_uuid)
			cmd_code = CODE_IOINFO_START_ALL;
		else if (!strcmp(cmd_code_str, "stop") && !chan_uuid)
			cmd_code = CODE_IOINFO_STOP_ALL;
		else if (!strcmp(cmd_code_str, "check") && !chan_uuid)
			cmd_code = CODE_IOINFO_CHECK_ALL;
	}

	if (cmd_code == CODE_IOINFO_START) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s ioinfo_start",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_ioinfo_start(mgrsac_p, chan_uuid);

	} else if (cmd_code == CODE_IOINFO_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s ioinfo_stop",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_ioinfo_stop(mgrsac_p, chan_uuid);

	} else if (cmd_code == CODE_IOINFO_CHECK) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: ioinfo_checl",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_ioinfo_check(mgrsac_p, chan_uuid);

	} else if (cmd_code == CODE_IOINFO_START_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s ioinfo_start_all",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_ioinfo_start_all(mgrsac_p);

	} else if (cmd_code == CODE_IOINFO_STOP_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s ioinfo_stop_all",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_ioinfo_stop_all(mgrsac_p);

	} else if (cmd_code == CODE_IOINFO_CHECK_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: ioinfo_check_all",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_ioinfo_check_all(mgrsac_p);

	} else {
		rc = -1;
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, chan_uuid);
		usage_sac();
	}

	return rc;
}

static int
__sac_display_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound",
			log_header, optind);
		usage_sac();
		goto out;
	}
	if (optind < argc) {
		cmd_str = argv[optind++];
	}
	if (chan_uuid != NULL && cmd_str != NULL) {
		if (strlen(chan_uuid) > NID_MAX_UUID) {
			nid_log_warning("%s: wrong sac uuid(%s)",
				log_header, chan_uuid);
			usage_sac();
			goto out;
		}
		if (!strcmp(cmd_str, "setup")) {
			cmd_code = CODE_S_DIS;
			is_setup = 1;
		}
		else if (!strcmp(cmd_str, "working")) {
			cmd_code = CODE_C_DIS;
			is_setup = 0;
		}
	}
	else if (chan_uuid == NULL && cmd_str != NULL){
		if (!strcmp(cmd_str, "setup")) {
			cmd_code = CODE_S_DIS_ALL;
			is_setup = 1;
		}
		else if (!strcmp(cmd_str, "working")) {
			cmd_code = CODE_C_DIS_ALL;
			is_setup = 0;
		}
	}
	else if (cmd_str == NULL) {
		nid_log_warning("%s: did not get cmd", log_header);
		usage_sac();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sac_uuid:%s: display",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_display(mgrsac_p, chan_uuid, is_setup);
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sac_uuid:%s: display all",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_display_all(mgrsac_p, is_setup);
	}
	else if (cmd_code == CODE_C_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sac_uuid:%s: display",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_display(mgrsac_p, chan_uuid, is_setup);
	}
	else if (cmd_code == CODE_C_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, sac_uuid:%s: display",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrsac_p->sa_op->sa_display_all(mgrsac_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, sac_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, chan_uuid);
		usage_sac();
	}
out:
	return rc;
}

static int
__sac_hello_func(int argc, char* const argv[], char *chan_uuid, struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__sac_hello_func";
	int rc = -1, cmd_code = CODE_NONE;

	assert(argv);
	assert(&chan_uuid);

	if (optind > argc) {
		nid_log_warning("%s: optind(%d) out of bound: hello",
			log_header, optind);
		usage_sac();
		goto out;
	}

	cmd_code = CODE_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind:%d: hello",
		log_header, cmd_code, optind);
	rc = mgrsac_p->sa_op->sa_hello(mgrsac_p);
out:
	return rc;
}

typedef int (*sac_operation_func)(int, char* const * , char *, struct mgrsac_interface *);
struct manager_sac_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	sac_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_sac_task sac_task_table[] = {
	{"information",		"info",		CMD_INFORMATION,		__sac_information_func,		NULL,		NULL},
	{"add", 		"add", 		CMD_ADD,			__sac_add_func,			NULL,		NULL},
	{"delete",		"del",		CMD_DELETE,			__sac_del_func,			NULL,		NULL},
	{"switch_bwc",		"swm",		CMD_SWITCH,			__sac_swtich_func,		NULL,		NULL},
	{"set_keepalive",	"skp",		CMD_SET_KEEPALIVE,		__sac_set_keepalive_func,	NULL,		NULL},
	{"fast_release",	"fr",		CMD_FAST_RELEASE,		__sac_fast_release_func,	NULL,		NULL},
	{"ioinfo",		"ioinfo",	CMD_IOINFO,			__sac_ioinfo_func,		NULL,		NULL},
	{"display",		"disp",		CMD_DISPLAY,			__sac_display_func,		NULL,		NULL},
	{"hello",		"hello",	CMD_HELLO,			__sac_hello_func,		NULL,		NULL},
	{NULL,			NULL,		CMD_MAX,			NULL,				NULL,		NULL},
};

static void
__setup_information_setup_description(struct manager_sac_task *info_mtask)
{
	info_mtask->description = "info:	show sac general information command\n";

	info_mtask->detail = x_malloc(1024);
	sprintf(info_mtask->detail, "%s",
		first_intend"stat:			show bio stat\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid info stat\n"
		first_intend"			sample:nidmanager -r s -m sac info stat\n"
		first_intend"list:			show sac list counter information\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid info list\n");
}

static void
__setup_add_setup_description(struct manager_sac_task *add_mtask)
{
	add_mtask->description = "add:	sac adding command\n";

	add_mtask->detail = x_malloc(1024*1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"[sync]:[direct_io]:[enable_kill_myself]:[alignment]:[tp_name]:[ds_name]:[dev_name]:[exportname][:[dev_size]]			sac configurations\n"
		first_intend"			sample:nidmanager -r s -m sac -i your_sac_uuid add 1:1:1:4096:your_tp_name:your_ds_name:your_drw_name:exportname\n"
		first_intend"			sample:nidmanager -r s -m sac -i your_sac_uuid add 1:1:1:4096:your_tp_name:your_ds_name:your_mrw_name:your_volume_uuid:your_dev_size\n");
}

static void
__setup_del_setup_description(struct manager_sac_task *del_mtask)
{
	del_mtask->description = "del:	sac deleting command\n";

	del_mtask->detail = x_malloc(1024*1024);
	sprintf(del_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m sac -i your_sac_uuid del\n");
}


static void
__setup_swm_setup_description(struct manager_sac_task *swm_mtask)
{
	swm_mtask->description = "swm:	sac bwc switching command\n";

	swm_mtask->detail = x_malloc (1024*1024);
	sprintf(swm_mtask->detail, "%s",
		first_intend"[bwc_uuid]:		uuid of the bwc the sac switches to\n"
		first_intend"			sample:nidmanager -r s -m sac -i your_sac_uuid swm your_bwc_uuid\n");
}

static void
__setup_skp_setup_description(struct manager_sac_task *swm_mtask)
{
	swm_mtask->description = "skp:	sac set keepalive command\n";

	swm_mtask->detail = x_malloc (1024*1024);
	sprintf(swm_mtask->detail, "%s%s",
		first_intend"			sample:nidmanager -r s -m sac -i your_sac_uuid skp 0\n",
		first_intend"			sample:nidmanager -r s -m sac -i your_sac_uuid skp 1 keepalive_count\n");
}

static void
__setup_fast_release_setup_description(struct manager_sac_task *fr_mtask)
{
	fr_mtask->description = "fr:	fast release command\n";

	fr_mtask->detail = x_malloc(1024);
	sprintf(fr_mtask->detail, "%s",
		first_intend"start:			start fast release\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid fr start\n"
		first_intend"stop:			stop fast release\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid fr stop\n");
}

static void
__setup_ioinfo_setup_description(struct manager_sac_task *ioinfo_mtask)
{
	ioinfo_mtask->description = "ioinfo:	count io inofrmation command";

	ioinfo_mtask->detail = x_malloc(1024);
	sprintf(ioinfo_mtask->detail, "%s",
		first_intend"start:			start counting request stat\n"
		first_intend"			sample:nidmanager -r s -m sac ioinfo start\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid ioinfo start\n"
		first_intend"stop:			stop counting request stat\n"
		first_intend"			sample:nidmanager -r s -m sac ioinfo stop\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid ioinfo stop\n"
		first_intend"check:			check request stat\n"
		first_intend"			sample:nidmanager -r s -m sac ioinfo check\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid ioinfo check\n");
}

static void
__setup_display_setup_description(struct manager_sac_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of sac\n";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show sac configuration\n"
		first_intend"			sample:nidmanager -r s -m sac display setup\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid display setup\n"
		first_intend"working:		show working sac configuration\n"
		first_intend"			sample:nidmanager -r s -m sac display working\n"
		first_intend"			sample:nidmanager -r s -m sac -i sac_uuid display working\n");
}

static void
__setup_hello_setup_description(struct manager_sac_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m sac hello\n");
}

static void
setup_descriptions()
{
	struct manager_sac_task *mtask;

	mtask = &sac_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_INFORMATION)
			__setup_information_setup_description(mtask);
		else if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DELETE)
			__setup_del_setup_description(mtask);
		else if (mtask->cmd_index == CMD_SWITCH)
			__setup_swm_setup_description(mtask);
		else if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
		else if (mtask->cmd_index == CMD_SET_KEEPALIVE)
			__setup_skp_setup_description(mtask);
		else if (mtask->cmd_index == CMD_FAST_RELEASE)
			__setup_fast_release_setup_description(mtask);
		else if (mtask->cmd_index == CMD_IOINFO)
			__setup_ioinfo_setup_description(mtask);
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
usage_sac()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_sac_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m sac ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &sac_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (sac_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &sac_task_table[i];
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
manager_server_sac(int argc, char * const argv[])
{
	char *log_header = "manager_server_sac";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	char *chan_uuid = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrsac_interface *mgrsac_p;
	struct mgrsac_setup setup;
	struct manager_sac_task *mtask = &sac_task_table[0];
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
			chan_uuid = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_sac();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrsac_p = x_calloc(1, sizeof(*mgrsac_p));
	mgrsac_initialization(mgrsac_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_sac();
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
		usage_sac();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, chan_uuid, mgrsac_p);

out:
	return rc;
}
