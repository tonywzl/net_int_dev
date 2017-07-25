/*
 * manager-rc.c
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
#include "mgrcrc_if.h"
#include "mac_if.h"
#include "manager.h"

/*
 * First level code
 */
#define CMD_DROPCACHE			1
#define CMD_INFORMATION			2
#define CMD_SETWGT			3
#define CMD_ADD				4
#define CMD_DISPLAY			5
#define CMD_HELLO			6
#define CMD_MAX				7

/*
 * Second level code
 */

#define CODE_NONE			0

/* code of dropcache */
#define CODE_START			1
#define CODE_START_SYNC			2
#define CODE_STOP			3

/* code of info */
#define CODE_FREE_SPACE			4
#define CODE_FREE_SPACE_DIST		5
#define CODE_NSE_STAT			6
#define CODE_NSE_DETAIL			7
#define CODE_DSBMP_RTREE		8
#define CODE_CHECK_FP			9
#define CODE_CSE_HIT			10
#define CODE_DSREC_STAT			11
#define CODE_SP_HEADS_SIZE		12

/* code of setwgt */
#define CODE_SETWGT			1

/* code of display */
#define CODE_S_DIS			1
#define CODE_S_DIS_ALL			2
#define CODE_W_DIS			3
#define CODE_W_DIS_ALL			4

/* code of hello */
#define CODE_HELLO			1

void usage_crc ();

static int
__crc_add_func(int argc, char* const argv[], char *crc_uuid, char *chan_uuid, struct mgrcrc_interface *mgrcrc_p)
{
	char *log_header = "__crc_add_func";
	char *crc_conf;
	char args[4][1024];
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";
	int n;
	char *pp_name, *cache_device;
	int cache_size;

	assert(!chan_uuid);
	if (!crc_uuid) {
		goto out;
	}

	if (optind >= argc) {
		warn_str = "got empty crc configuration!";
		goto out;
	}

	crc_conf = argv[optind++];
	n = sscanf(crc_conf, "%[^:]:%[^:]:%[^:]:%[^:]",
			args[0], args[1], args[2], args[3]);

	if (n != 3) {
		warn_str = "wrong number of crc configuration items!";
		goto out;
	}

	pp_name = args[0];
	if (strlen(pp_name) >= NID_MAX_PPNAME) {
		warn_str = "wrong pp name configuration!";
		goto out;
	}

	cache_device = args[1];
	if (strlen(cache_device) >= NID_MAX_PATH) {
		warn_str = "wrong cache device configuration!";
		goto out;
	}

	cache_size = atoi(args[2]);

	nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s",
				log_header, cmd_code, optind, crc_uuid);
	rc = mgrcrc_p->mcr_op->mcr_add(mgrcrc_p, crc_uuid, pp_name, cache_device, (uint32_t)cache_size);
	return rc;

out:
	if (rc) {
		usage_crc();
		nid_log_warning("%s: crc_uuid:%s: %s",
			log_header, crc_uuid, warn_str);
	}
	return rc;
}

static int
__crc_dropcache_func (int argc, char* const argv[], char *crc_uuid, char *chan_uuid, struct mgrcrc_interface *mgrcrc_p)
{

	char *log_header = "__crc_dropcache_func";
	char *cmd_code_str = NULL;
	int rc  = 0, cmd_code = CODE_NONE;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start"))
			cmd_code = CODE_START;
		else if (!strcmp(cmd_code_str, "start_sync"))
			cmd_code = CODE_START_SYNC;
		else if (!strcmp(cmd_code_str, "stop"))
			cmd_code = CODE_STOP;
	}

	if (cmd_code == CODE_START) {
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s dropcache_start",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_dropcache_start(mgrcrc_p, crc_uuid, chan_uuid, 0);
	} else if (cmd_code == CODE_START_SYNC) {
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s dropcache_start_sync",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_dropcache_start(mgrcrc_p, crc_uuid, chan_uuid, 1);
	} else if (cmd_code == CODE_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: dropcache_stop",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_dropcache_stop(mgrcrc_p, crc_uuid, chan_uuid);
	} else {
		rc = -1;
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, crc_uuid);
		usage_crc();
	}
	return rc;

}

static int
__crc_information_func(int argc, char* const argv[], char *crc_uuid, char *chan_uuid, struct mgrcrc_interface *mgrcrc_p)
{

	char *log_header = "__crc_information_func";
	char *cmd_code_str = NULL;
	int rc  = -1, cmd_code = CODE_NONE;


	if ( optind < argc ) {
			cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "disk_space"))
			cmd_code = CODE_FREE_SPACE;
		else if (!strcmp(cmd_code_str, "space_dist"))
			cmd_code = CODE_FREE_SPACE_DIST;
		else if (!strcmp(cmd_code_str, "sp_heads_size"))
			cmd_code = CODE_SP_HEADS_SIZE;
		else if (!strcmp(cmd_code_str, "dsbmp_rtree"))
			cmd_code = CODE_DSBMP_RTREE;
		else if (!strcmp(cmd_code_str, "check_fp"))
			cmd_code = CODE_CHECK_FP;
		else if (!strcmp(cmd_code_str, "nse_stat"))
			cmd_code = CODE_NSE_STAT;
		else if (!strcmp(cmd_code_str, "nse_detail"))
			cmd_code = CODE_NSE_DETAIL;
		else if (!strcmp(cmd_code_str, "cse_hit"))
			cmd_code = CODE_CSE_HIT;
		else if (!strcmp(cmd_code_str, "dsrec_stat"))
			cmd_code = CODE_DSREC_STAT;
	}

	if ( cmd_code == CODE_FREE_SPACE ) {
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: info disk_space",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_info_freespace( mgrcrc_p, crc_uuid );
	} else if ( cmd_code == CODE_FREE_SPACE_DIST ) {
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: info space_dist",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_info_freespace_dist( mgrcrc_p, crc_uuid );
	} else if ( cmd_code == CODE_SP_HEADS_SIZE ) {
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: info sp_heads_size",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_info_sp_heads_size( mgrcrc_p, crc_uuid );
	} else if ( cmd_code == CODE_DSBMP_RTREE ) {
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: info dsbmp_rtree",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_info_dsbmp_rtree( mgrcrc_p, crc_uuid );
	} else if ( cmd_code == CODE_CHECK_FP ) {
		nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: info disk_space",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_info_check_fp( mgrcrc_p, crc_uuid );
	} else if (cmd_code == CODE_NSE_STAT) {
		nid_log_warning("%s: got cmd CODE_NSE_COUNTER", log_header);
		rc = mgrcrc_p->mcr_op->mcr_info_nse_stat(mgrcrc_p, crc_uuid, chan_uuid);
	} else if (cmd_code == CODE_NSE_DETAIL) {
		nid_log_warning("%s: got cmd CODE_NSE_DETAIL", log_header);
		rc = mgrcrc_p->mcr_op->mcr_info_nse_detail(mgrcrc_p, crc_uuid, chan_uuid);
	} else if (cmd_code == CODE_CSE_HIT) {
		nid_log_warning("%s: got cmd CODE_CSE_HIT", log_header);
		rc = mgrcrc_p->mcr_op->mcr_info_cse_hit(mgrcrc_p, crc_uuid);
	}else if (cmd_code == CODE_DSREC_STAT) {
		nid_log_warning("%s: got cmd CODE_DSREC_STAT", log_header);
		rc = mgrcrc_p->mcr_op->mcr_info_dsrec_stat(mgrcrc_p, crc_uuid, chan_uuid);
	}else {
		rc = -1;
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, chan_uuid);
		usage_crc();
	}
	return rc;
}

static int
__crc_setwgt_func (int argc, char* const argv[], char *crc_uuid, char *chan_uuid, struct mgrcrc_interface *mgrcrc_p)
{

	char *log_header = "__crc_setwgt_func";
	char *cmd_code_str = NULL;
	uint8_t cmd_wgt = 0;
	int rc  = 0, cmd_code = CODE_NONE;

	assert(!chan_uuid);
	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "set")) {
			cmd_code = CODE_SETWGT;
			cmd_wgt = atoi(argv[optind++]);
			assert(cmd_wgt < 16);
		}

		if (cmd_code == CODE_SETWGT) {
			nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: setwgt", log_header, cmd_code, optind, crc_uuid);
			rc = mgrcrc_p->mcr_op->mcr_set_wgt(mgrcrc_p, crc_uuid, cmd_wgt);
		} else {
			rc = -1;
			nid_log_warning("%s: cmd_code=%d, optind:%d crc_uuid:%s: got wrong cmd_code", log_header, cmd_code, optind, crc_uuid);
			usage_crc();
		}
	}
	return rc;

}

static int
__crc_display_func(int argc, char* const argv[], char *crc_uuid, char *chan_uuid, struct mgrcrc_interface *mgrcrc_p)
{
	char *log_header = "__crc_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	assert(!chan_uuid);

	if (optind >= argc) {
		warn_str = "got empty sub command!";
		usage_crc();
		goto out;
	}

	cmd_str = argv[optind++];

	if (crc_uuid != NULL && cmd_str != NULL) {
		if (strlen(crc_uuid) > NID_MAX_UUID) {
			warn_str = "wrong crc_uuid!";
			usage_crc();
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
	else if (crc_uuid == NULL && cmd_str != NULL){
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
		usage_crc();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, crc_uuid:%s: display",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_display(mgrcrc_p, crc_uuid, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, crc_uuid:%s: display all",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_display_all(mgrcrc_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, crc_uuid:%s: display",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_display(mgrcrc_p, crc_uuid, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, crc_uuid:%s: display",
			log_header, cmd_code, optind, crc_uuid);
		rc = mgrcrc_p->mcr_op->mcr_display_all(mgrcrc_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, crc_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, crc_uuid);
	}

out:
	if (rc) {
		nid_log_warning("%s: crc_uuid:%s: %s",
			log_header, crc_uuid, warn_str);
	}
	return rc;
}

static int
__crc_hello_func (int argc, char* const argv[], char *crc_uuid, char *chan_uuid, struct mgrcrc_interface *mgrcrc_p)
{

	char *log_header = "__crc_setwgt_func";
	int rc  = -1, cmd_code = CODE_NONE;

	assert(&chan_uuid);
	assert(&crc_uuid);
	assert(argv);

	if (optind > argc) {
		usage_crc();
		goto out;
	}

	cmd_code = CMD_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind:%d: hello",
		log_header, cmd_code, optind);
	rc = mgrcrc_p->mcr_op->mcr_hello(mgrcrc_p);

out:
	return rc;
}

typedef int (*crc_operation_func)(int, char* const * , char *, char *, struct mgrcrc_interface *);
struct manager_crc_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	crc_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_crc_task crc_task_table[] = {
	{"dropcache", 	"dc", 	CMD_DROPCACHE,		__crc_dropcache_func,	NULL,		NULL},
	{"information",	"info",	CMD_INFORMATION,	__crc_information_func,	NULL,		NULL},
	{"setwgt",	"sp",	CMD_SETWGT,		__crc_setwgt_func,	NULL,		NULL},
	{"add", 	"add", 	CMD_ADD,		__crc_add_func,		NULL,		NULL},
	{"display",	"disp",	CMD_DISPLAY,		__crc_display_func,	NULL,		NULL},
	{"hello",	"hello",CMD_HELLO,		__crc_hello_func,	NULL,		NULL},
	{NULL,		NULL,	CMD_MAX,		NULL,			NULL,		NULL},
};

static void
__setup_add_setup_description(struct manager_crc_task *add_mtask)
{
	add_mtask->description = "add:	add operation command\n";

	add_mtask->detail = x_malloc(1024*1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"[crc_uuid] add [crc_pp_name]:[cache_device]:[cache_size]"
			"       crc configurations\n"
		first_intend"			sample:nidmanager -r s -m crc -i your_crc_uuid add your_crc_pp_name:/rc_device:4 \n");
}

static void
__setup_dc_setup_description(struct manager_crc_task *dc_mtask)
{
	dc_mtask->description = "dc:	droping cache command\n";

	dc_mtask->detail = x_malloc(1024*1024);
	sprintf(dc_mtask->detail, "%s",
		first_intend"start:			start droping cache \n"
		"				sample:nidmanager -r s -m crc -i crc_uuid dc start\n"
		first_intend"start_sync:		start droping cache with sync mode\n"
		"				sample:nidmanager -r s -m crc -i crc_uuid dc start_sync\n"
		first_intend"stop:			stop  droping cache\n"
		"				sample:nidmanager -r s -m crc -i crc_uuid dc stop\n");
}

static void
__setup_info_setup_description(struct manager_crc_task *info_mtask)
{
	info_mtask->description = "info:	information display command\n";

	info_mtask->detail = x_malloc(1024*1024);
	sprintf(info_mtask->detail, "%s",
		first_intend"disk_space:		display crc disk_space status\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid info disk_space\n"
		first_intend"space_dist:		display crc space_dist status\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid info space_dist\n"
		first_intend"sp_heads_size:		display crc sp heads size\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid info sp_heads_size\n"
		first_intend"dsbmp_rtree:		display crc dsbmp_rtree status\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid info dsbmp_rtree\n"
		first_intend"check_fp:		display crc check_fp status\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid info check_fp\n"
		first_intend"nse_stat:		display crc nse status\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid -c chan_uuid info nse_stat\n"
		first_intend"nse_detail:		display crc nse_detail status\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid -c chan_uuid info nse_detail\n"
		first_intend"cse_hit:		display cse hit status and ratio\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid -c chan_uuid info cse_hit\n"
		first_intend"dsrec_stat:		display dsreclaim status\n"
		"				sample:	nidmanager -r s -m crc -i crc_uuid -c chan_uuid info dsrec_stat\n");

}

static void
__setup_sp_setup_description(struct manager_crc_task *sp_mtask)
{
	sp_mtask->description = "sp:	setting wgt command\n";

	sp_mtask->detail = x_malloc(1024*1024);
	sprintf(sp_mtask->detail, "%s",
		first_intend"set:			start setting wgt \n"
		"				sample:nidmanager -r s -m crc sp set [wgt]\n");
}

static void
__setup_display_setup_description(struct manager_crc_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of crc";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show crc configuration\n"
		first_intend"			sample:nidmanager -r s -m crc display setup\n"
		first_intend"			sample:nidmanager -r s -m crc -i crc_uuid display setup\n"
		first_intend"working:		show working crc configuration\n"
		first_intend"			sample:nidmanager -r s -m crc display working\n"
		first_intend"			sample:nidmanager -r s -m crc -i crc_uuid display working\n");
}

static void
__setup_hello_setup_description(struct manager_crc_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m crc hello\n");
};

static void
setup_descriptions()
{
	struct manager_crc_task *mtask;

	mtask = &crc_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_DROPCACHE)
			__setup_dc_setup_description(mtask);
		else if (mtask->cmd_index == CMD_INFORMATION)
			__setup_info_setup_description(mtask);
		else if (mtask->cmd_index == CMD_SETWGT)
			__setup_sp_setup_description(mtask);
		else if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
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
usage_crc()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_crc_task *task;
	int i;

	setup_descriptions();
	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m crc ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &crc_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (crc_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &crc_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s\n",task->description );
		printf("%s\n",task->detail);

	}
	printf ("\n");


	return;
}


static char *optstr_ss = "a:c:i:v:";

static struct option long_options_ss[] = {
	{0, 0, 0, 0},
};

int
manager_server_crc(int argc, char * const argv[])
{
	char *log_header = "manager_server_crc";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	char *chan_uuid = NULL;		// all channels
	char* crc_uuid = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrcrc_interface *mgrcrc_p;
	struct mgrcrc_setup setup;
	struct manager_crc_task *mtask = &crc_task_table[0];
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
			chan_uuid = optarg;
			break;

		case 'i':
		  	crc_uuid = optarg;
		  	break;

		default:
			printf("the option has not been create\n");
			usage_crc();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrcrc_p = x_calloc(1, sizeof(*mgrcrc_p));
	mgrcrc_initialization(mgrcrc_p, &setup);

	/* get command */

	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		printf("need more option \n");
		usage_crc();
		return 0;
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
		usage_crc();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, crc_uuid, chan_uuid, mgrcrc_p);

#if 0
	if (!strcmp(cmd_str, "dropcache") || !strcmp(cmd_str, "dc")) {
		cmd_index = CODE_DROPCACHE;
	} else if (!strcmp(cmd_str, "information") || !strcmp(cmd_str, "info")) {
		cmd_index = CODE_INFOMATION;
	} else {
		usage_crc();
		rc = -1;
		goto out;
	}



		break;
	default:
		rc = -1;
		goto out;
	}	
#endif
out:
	return rc;
}
