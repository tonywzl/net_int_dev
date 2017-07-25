/*
 * manager-bwc.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_bwc_if.h"
#include "mgrbwc_if.h"
#include "manager.h"

#define CMD_DROPCACHE		1
#define CMD_INFORMATION		2
#define CMD_THROUGHPUT		3
#define CMD_ADD			4
#define CMD_REMOVE		5
#define CMD_FFLUSH		6
#define CMD_RECOVER		7
#define CMD_SNAPSHOT_FREEZE	8
#define CMD_SNAPSHOT_UNFREEZE	9
#define CMD_WATER_MARK		10
#define CMD_IOINFO		11
#define CMD_DELAY		12
#define CMD_FLUSH_EMPTY		13
#define CMD_DISPLAY		14
#define CMD_HELLO		15
#define	CMD_MAX			16

#define CODE_NONE		0

/* code of dropcache */
#define CODE_DC_START		1
#define CODE_DC_START_SYNC	2
#define CODE_DC_STOP		3

/* code of info */
#define CODE_INFO_FLUSHING	1
#define CODE_INFO_STAT		2
#define CODE_INFO_THROUGHPUT	3
#define CODE_INFO_RW		4
#define CODE_INFO_DELAY		5
#define CODE_INFO_LIST_STAT	6

/* code of throughput */
#define CODE_THPT_RESET		1

/* code of fast flush */
#define CODE_FFLUSH_START	1
#define CODE_FFLUSH_GET		2
#define CODE_FFLUSH_STOP	3

/* code of recover */
#define CODE_RECOVER_START	1
#define CODE_RECOVER_GET	2

/* code of snapshot */
#define CODE_SNAPSHOT_FREEZE_STAGE_1 UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_1
#define CODE_SNAPSHOT_FREEZE_STAGE_2 UMSG_BWC_CODE_SNAPSHOT_FREEZE_STAGE_2
#define CODE_SNAPSHOT_UNFREEZE UMSG_BWC_CODE_SNAPSHOT_UNFREEZE

/* code of ioinfo */
#define CODE_IOINFO_START	1
#define CODE_IOINFO_START_ALL	2
#define CODE_IOINFO_STOP	3
#define CODE_IOINFO_STOP_ALL	4
#define CODE_IOINFO_CHECK	5
#define CODE_IOINFO_CHECK_ALL	6

/* code of flush empty */
#define CODE_FLUSH_EMPTY_START	1
#define CODE_FLUSH_EMPTY_STOP	2

/* code of display */
#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_W_DIS		3
#define CODE_W_DIS_ALL		4

/* code of hello */
#define CODE_HELLO		1

#define BFP_POLICY_BFP1		1
#define BFP_POLICY_BFP2		2
#define BFP_POLICY_BFP3		3
#define BFP_POLICY_BFP4		4
#define BFP_POLICY_BFP5		5


static void usage_bwc();

static int
__bwc_dropcache_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_dropcache_func";
	char *cmd_code_str = NULL;
	int rc  = 0, cmd_code = CODE_NONE;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start"))
			cmd_code = CODE_DC_START;
		else if (!strcmp(cmd_code_str, "start_sync"))
			cmd_code = CODE_DC_START_SYNC;
		else if (!strcmp(cmd_code_str, "stop"))
			cmd_code = CODE_DC_STOP;
	}

	if (cmd_code == CODE_DC_START) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s dropcache_start",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_dropcache_start(mgrbwc_p, bwc_uuid, chan_uuid, 0);

	} else if (cmd_code == CODE_DC_START_SYNC) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s dropcache_start_sync",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_dropcache_start(mgrbwc_p, bwc_uuid, chan_uuid, 1);

	} else if (cmd_code == CODE_DC_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: dropcache_stop",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_dropcache_stop(mgrbwc_p, bwc_uuid, chan_uuid);

	} else {
		rc = -1;
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, bwc_uuid);
		usage_bwc();
	}

	return rc;
}

static int
__bwc_information_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_information_func";
	char *cmd_code_str = NULL;
	int rc  = -1, cmd_code = CODE_NONE;

	if (!bwc_uuid) {
		usage_bwc();
		return rc;
	}

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
		else if (!strcmp(cmd_code_str, "delay"))
			cmd_code = CODE_INFO_DELAY;
		else if (!strcmp(cmd_code_str, "list"))
			cmd_code = CODE_INFO_LIST_STAT;
	}

	if (cmd_code == CODE_INFO_FLUSHING) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: information_flushing",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_information_flushing(mgrbwc_p, bwc_uuid);

	} else if (cmd_code == CODE_INFO_STAT) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: information_stat",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_information_stat(mgrbwc_p, bwc_uuid);

	} else if (cmd_code == CODE_INFO_THROUGHPUT) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: information_stat_throughput",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_information_throughput_stat(mgrbwc_p, bwc_uuid);

	} else if (cmd_code == CODE_INFO_RW) {
		if (!chan_uuid) {
			usage_bwc();
			return rc;
		}
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: information_stat_rw",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_information_rw_stat(mgrbwc_p, bwc_uuid, chan_uuid);

	} else if (cmd_code == CODE_INFO_LIST_STAT) {
		if (!chan_uuid) {
			usage_bwc();
			return rc;
		}
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: information_list_stat",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_information_list_stat(mgrbwc_p, bwc_uuid, chan_uuid);

	} else if (cmd_code == CODE_INFO_DELAY){
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: information_stat_delay",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_information_delay_stat(mgrbwc_p, bwc_uuid);
	} else {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, bwc_uuid);
		usage_bwc();
	}

	return rc;
}

static int
__bwc_throughput_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_throughput_func";
	char *cmd_code_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;

	assert(!chan_uuid);
	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "reset"))
			cmd_code = CODE_THPT_RESET;
	}

	if (cmd_code == CODE_THPT_RESET) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: reset throughput information",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_throughput_stat_reset(mgrbwc_p, bwc_uuid);

	} else {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, bwc_uuid);
		usage_bwc ();
	}

	return rc;
}


static int
__bwc_add_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_add_func";
	char *bwc_conf;
	char args[18][1024];
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";
	int n;
	char *bufdev, *flush_policy, *tp_name;
	int bufdev_sz, rw_sync, two_step_read, do_fp, bfp_type, ssd_mode, max_flush_size, write_delay_first_level, write_delay_second_level, high_water_mark, low_water_mark;
	double coalesce_ratio = 0.0, load_ratio_max = 0.0, load_ratio_min = 0.0,
			load_ctrl_level = 0.0, flush_delay_ctl = 0.0, throttle_ratio = 1.0;

	assert(!chan_uuid);
	if (!bwc_uuid) {
		goto out;
	}

	if (optind >= argc) {
		warn_str = "got empty bwc configuration!";
		goto out;
	}

	bwc_conf = argv[optind++];
	n = sscanf(bwc_conf, "%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]",
			args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7],
			args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);

	if (n < 7) {
		warn_str = "wrong number of bwc configuration items!";
		goto out;
	}

	bufdev = args[0];
	if (strlen(bufdev) >= NID_MAX_PATH) {
		warn_str = "wrong buffer device configuration!";
		goto out;
	}

	bufdev_sz = atoi(args[1]);

	rw_sync = atoi(args[2]);
	if (rw_sync != 0 && rw_sync != 1) {
		warn_str = "wrong rw sync configuration!";
		goto out;
	}

	two_step_read = atoi(args[3]);
	if (two_step_read != 0 && two_step_read != 1) {
		warn_str = "wrong two step read configuration!";
		goto out;
	}

	do_fp = atoi(args[4]);
	if (do_fp != 0 && do_fp != 1) {
		warn_str = "wrong do fp configuration!";
		goto out;
	}

	tp_name = args[5];
	if (strlen(tp_name) >= NID_MAX_TPNAME) {
		warn_str = "wrong tp_name configuration!";
		goto out;
	}

	ssd_mode = atoi(args[6]);
	if (ssd_mode != 0 && ssd_mode != 1) {
		warn_str = "wrong ssd_mode configuration!";
		goto out;
	}

	max_flush_size = atoi(args[7]);
	if (max_flush_size < 0 && max_flush_size > 1024) {
		warn_str = "wrong max_flush_size configuration, range [0, 1024]!";
		goto out;
	}

	write_delay_first_level = atoi(args[8]);
	if (write_delay_first_level < 0 && write_delay_first_level > 1024) {
		warn_str = "wrong write_delay_first_level configuration, range [0, 1024]!";
		goto out;
	}

	write_delay_second_level = atoi(args[9]);
	if (write_delay_second_level < 0 && write_delay_second_level > 1024) {
		warn_str = "wrong write_delay_second_level configuration, range [0, 1024]!";
		goto out;
	}

	flush_policy = args[10];
	if (strcmp(flush_policy, "bfp5") == 0) {
		bfp_type = BFP_POLICY_BFP5;
	} else if (strcmp(flush_policy, "bfp4") == 0) {
		bfp_type = BFP_POLICY_BFP4;
	} else if (strcmp(flush_policy, "bfp3") == 0) {
		bfp_type = BFP_POLICY_BFP3;
	} else if (strcmp(flush_policy, "bfp2") == 0) {
		bfp_type = BFP_POLICY_BFP2;
	} else if (strcmp(flush_policy, "bfp1") == 0) {
		bfp_type = BFP_POLICY_BFP1;
	} else {
		warn_str = "wrong flush policy configuration!";
		goto out;
	}

	if((bfp_type == BFP_POLICY_BFP1 && n != 13) ||
			(bfp_type == BFP_POLICY_BFP2 && n != 15) ||
			(bfp_type == BFP_POLICY_BFP3 && n != 16)  ||
			(bfp_type == BFP_POLICY_BFP4 && n != 16) ||
			(bfp_type == BFP_POLICY_BFP5 && n != 17)) {
			warn_str = "wrong number of bwc configuration items!";
			goto out;
	}

	if (bfp_type == BFP_POLICY_BFP1) {
		high_water_mark = atoi(args[11]);
		if (high_water_mark > 100 || high_water_mark < 0) {
			warn_str = "wrong high water mark confiugration!";
			goto out;
		}

		low_water_mark = atoi(args[12]);
		if (low_water_mark > 100 || low_water_mark < 0) {
			warn_str = "wrong low water mark confiugration!";
			goto out;
		}

		if (low_water_mark >= high_water_mark) {
			warn_str = "high_water_mark must be greater than low_water_mark!";
			goto out;
		}
	}

	if (bfp_type != BFP_POLICY_BFP1) {
		load_ratio_min = atof(args[11]);
#if 0
		if (load_ratio_min > LOAD_RATIO_MAX || load_ratio_min < LOAD_RATIO_MIN) {
			warn_str = "wrong min load ratio configuration!";
			goto out;
		}
#endif
		load_ratio_max = atof(args[12]);
#if 0
		if (load_ratio_max > LOAD_RATIO_MAX || load_ratio_max < LOAD_RATIO_MIN || load_ratio_min >= load_ratio_max) {
			warn_str = "wrong max load ratio configuration!";
			goto out;
		}
#endif
		load_ctrl_level = atof(args[13]);
#if 0
		if (load_ctrl_level > LOAD_CTRL_LEVEL_MAX || load_ctrl_level < LOAD_CTRL_LEVEL_MIN) {
			warn_str = "wrong load control ratio configuration!";
			goto out;
		}
#endif
		flush_delay_ctl = atof(args[14]);
#if 0
		if (FLUSH_DELAY_CTL_MIN < 0 || flush_delay_ctl > FLUSH_DELAY_CTL_MAX) {
			warn_str = "wrong flush delay control configuration!";
			goto out;
		}
#endif
	}

	if (bfp_type == BFP_POLICY_BFP3) {
		coalesce_ratio = atof(args[15]);
		if (coalesce_ratio > 1.0 || coalesce_ratio < 0.0) {
			warn_str = "wrong coalesce ratio configuration!";
			goto out;
		}
	} else if (bfp_type == BFP_POLICY_BFP4 || bfp_type == BFP_POLICY_BFP5) {
		throttle_ratio = atof(args[15]);
		if (throttle_ratio > 1.0 || throttle_ratio <= 0.1) {
			warn_str = "wrong throttle ratio configuration!";
			goto out;
		}
	}

	if (bfp_type == BFP_POLICY_BFP5) {
		coalesce_ratio = atof(args[16]);
		if (coalesce_ratio > 1.0 || coalesce_ratio < 0.0) {
			warn_str = "wrong coalesce ratio configuration!";
			goto out;
		}
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s",
				log_header, cmd_code, optind, bwc_uuid);
	rc = mgrbwc_p->bw_op->bw_add(mgrbwc_p, bwc_uuid, bufdev, (uint32_t)bufdev_sz,
			(uint8_t)rw_sync, (uint8_t)two_step_read, (uint8_t)do_fp, (uint8_t)bfp_type,
			coalesce_ratio, load_ratio_max, load_ratio_min, load_ctrl_level, flush_delay_ctl, throttle_ratio, tp_name,
			(uint8_t)ssd_mode, (uint8_t)max_flush_size, (uint16_t)write_delay_first_level, (uint16_t)write_delay_second_level, (uint16_t)high_water_mark, (uint16_t)low_water_mark);
	return rc;

out:
	if (rc) {
		usage_bwc();
		nid_log_warning("%s: mbwc_uuid:%s: %s",
			log_header, bwc_uuid, warn_str);
	}
	return rc;
}

static int
__bwc_remove_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_remove_func";
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	assert(!chan_uuid);
	if (optind < argc) {
		warn_str = "got too many arguments!";
		goto out;
	}

	if (bwc_uuid == NULL){
		goto out;

	}

	nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s",
		log_header, cmd_code, optind, bwc_uuid);
	rc = mgrbwc_p->bw_op->bw_remove(mgrbwc_p, bwc_uuid);
	return rc;

out:
	if (rc) {
		usage_bwc();
		nid_log_warning("%s: mbwc_uuid:%s: %s",
			log_header, bwc_uuid, warn_str);
	}
	return rc;
}

static int
__bwc_fflush_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_fflush_func";
	char *cmd_code_str = NULL;
	int rc  = 0, cmd_code = CODE_NONE;

	assert(bwc_uuid);
	assert(chan_uuid);
	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start"))
			cmd_code = CODE_FFLUSH_START;
		else if (!strcmp(cmd_code_str, "get"))
			cmd_code = CODE_FFLUSH_GET;
		else if (!strcmp(cmd_code_str, "stop"))
			cmd_code = CODE_FFLUSH_STOP;
	}

	if (cmd_code == CODE_FFLUSH_START) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s fflush_start",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_fflush_start(mgrbwc_p, bwc_uuid, chan_uuid);

	} else if (cmd_code == CODE_FFLUSH_GET) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s fflush_get",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_fflush_get(mgrbwc_p, bwc_uuid, chan_uuid);

	} else if (cmd_code == CODE_FFLUSH_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: fflush_stop",
			log_header, cmd_code, optind, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_fflush_stop(mgrbwc_p, bwc_uuid, chan_uuid);

	} else {
		rc = -1;
		nid_log_warning("%s: cmd_code=%d, optind:%d chan_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, chan_uuid);
		usage_bwc();
	}

	return rc;
}

static int
__bwc_snapshot_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_snapshot_func";
	int rc  = 0, cmd_code = CODE_NONE, i;
	char cmd_info[64];

	assert(bwc_uuid);
	assert(chan_uuid);

	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "freeze") == 0) {
			if(strcmp(argv[i+1], "stage1") == 0) {
				cmd_code = CODE_SNAPSHOT_FREEZE_STAGE_1;
				strcpy(cmd_info, "freeze stage1");
			} else if(strcmp(argv[i+1], "stage2") == 0) {
				cmd_code = CODE_SNAPSHOT_FREEZE_STAGE_2;
				strcpy(cmd_info, "freeze stage2");
			}
			break;
		} else if(strcmp(argv[i], "unfreeze") == 0) {
			strcpy(cmd_info, "unfreeze");
			cmd_code = CODE_SNAPSHOT_UNFREEZE;
			break;
		} else {
			continue;
		}
	}


	if (cmd_code == CODE_SNAPSHOT_FREEZE_STAGE_1 || cmd_code == CODE_SNAPSHOT_FREEZE_STAGE_2 ||
		cmd_code == CODE_SNAPSHOT_UNFREEZE) {
		nid_log_warning("%s: snapshot action:%s, bwc_uuid:%s chan_uuid:%s",
			log_header, cmd_info, bwc_uuid, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_snapshot_admin(mgrbwc_p, bwc_uuid, chan_uuid, cmd_code);
	} else {
		rc = -2;
	}

	switch(rc) {
	case -2:
		nid_log_warning("%s: cmd_code=%d, bwc_uuid:%s chan_uuid:%s: got wrong cmd_code",
				log_header, cmd_code, bwc_uuid, chan_uuid);
		usage_bwc();
		break;

	case -1:
		printf("Failed to connect nidserver\n");
		break;

        case 0:
		printf("success to %s bwc_uuid: %s chan_uuid: %s\n",
			cmd_info, bwc_uuid, chan_uuid);
		break;

	case 1:
		if(cmd_code == CODE_SNAPSHOT_FREEZE_STAGE_2) {
			printf("bwc_uuid: %s chan_uuid:%s does not in freeze stage 1 state\n", bwc_uuid, chan_uuid);
		} else {
			printf("bwc_uuid: %s chan_uuid:%s does not exist\n", bwc_uuid, chan_uuid);
		}
		break;

	case 2:
		if(cmd_code == CODE_SNAPSHOT_FREEZE_STAGE_1) {
			printf("failed to freeze bwc_uuid: %s chan_uuid:%s, already in freeze state\n", bwc_uuid, chan_uuid);
		} else if(cmd_code == CODE_SNAPSHOT_FREEZE_STAGE_2) {
			printf("failed to freeze bwc_uuid: %s chan_uuid:%s, already in freeze stage 2 state\n", bwc_uuid, chan_uuid);
		} else if(cmd_code == CODE_SNAPSHOT_UNFREEZE) {
			printf("resource %s is in freezing progress, please try command later", chan_uuid);
		}
		break;

	case 3:
		if(cmd_code == CODE_SNAPSHOT_UNFREEZE) {
			printf("resource %s not in freeze state, no need to unfreeze\n", chan_uuid);
		} else if(cmd_code == CODE_SNAPSHOT_FREEZE_STAGE_1) {
			printf("resource %s busy, please try command later", chan_uuid);
		}
		break;

	default:
		printf("unknown return value: %d\n", rc);
		break;

	}

	return rc;
}

static int
__bwc_recover_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_recover_func";
	char *cmd_code_str = NULL;
	int rc  = 0, cmd_code = CODE_NONE;

	assert(bwc_uuid);
	assert(!chan_uuid);
	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start"))
			cmd_code = CODE_RECOVER_START;
		else if (!strcmp(cmd_code_str, "get"))
			cmd_code = CODE_RECOVER_GET;
	}

	if (cmd_code == CODE_RECOVER_START) {
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s recover_start",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_recover_start(mgrbwc_p, bwc_uuid);

	} else if (cmd_code == CODE_RECOVER_GET) {
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s recover_get",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_recover_get(mgrbwc_p, bwc_uuid);
	} else {
		rc = -1;
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, bwc_uuid);
		usage_bwc();
	}

	return rc;
}

static int
__bwc_water_mark_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_water_mark_func";
	char *bwc_conf;
	char args[2][1024];
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";
	int n;
	int high_water_mark, low_water_mark;

	assert(!chan_uuid);
	if (!bwc_uuid) {
		goto out;
	}

	if (optind >= argc) {
		warn_str = "got empty bwc configuration!";
		goto out;
	}

	bwc_conf = argv[optind++];
	n = sscanf(bwc_conf, "%[^:]:%[^:]", args[0], args[1]);

	if (n < 2) {
		warn_str = "wrong number of bwc configuration items!";
		goto out;
	}

	high_water_mark = atoi(args[0]);
	if (high_water_mark > 100 || high_water_mark < 0) {
		warn_str = "wrong high water mark confiugration!";
		goto out;
	}

	low_water_mark = atoi(args[1]);
	if (low_water_mark > 100 || low_water_mark < 0) {
		warn_str = "wrong low water mark confiugration!";
		goto out;
	}

	if (low_water_mark >= high_water_mark) {
		warn_str = "high_water_mark must be greater than low_water_mark!";
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s",
				log_header, cmd_code, optind, bwc_uuid);
	rc = mgrbwc_p->bw_op->bw_water_mark(mgrbwc_p, bwc_uuid, (uint16_t)high_water_mark, (uint16_t)low_water_mark);
	return rc;

out:
	if (rc) {
		usage_bwc();
		nid_log_warning("%s: mbwc_uuid:%s: %s",
			log_header, bwc_uuid, warn_str);
	}
	return rc;
}

static int
__bwc_ioinfo_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_ioinfo_func";
	char *cmd_code_str = NULL;
	int rc  = 0, cmd_code = CODE_NONE;
	uint8_t is_bfe = 0;

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start") && bwc_uuid)
			cmd_code = CODE_IOINFO_START;
		else if (!strcmp(cmd_code_str, "stop") && bwc_uuid)
			cmd_code = CODE_IOINFO_STOP;
		else if (!strcmp(cmd_code_str, "check") && bwc_uuid)
			cmd_code = CODE_IOINFO_CHECK;
		else if (!strcmp(cmd_code_str, "start") && !bwc_uuid)
			cmd_code = CODE_IOINFO_START_ALL;
		else if (!strcmp(cmd_code_str, "stop") && !bwc_uuid)
			cmd_code = CODE_IOINFO_STOP_ALL;
		else if (!strcmp(cmd_code_str, "check") && !bwc_uuid)
			cmd_code = CODE_IOINFO_CHECK_ALL;
		else if (!strcmp(cmd_code_str, "start_bfe") && bwc_uuid && chan_uuid) {
			cmd_code = CODE_IOINFO_START;
			is_bfe = 1;
		} else if (!strcmp(cmd_code_str, "stop_bfe") && bwc_uuid && chan_uuid) {
			cmd_code = CODE_IOINFO_STOP;
			is_bfe = 1;
		} else if (!strcmp(cmd_code_str, "check_bfe") && bwc_uuid && chan_uuid) {
			cmd_code = CODE_IOINFO_CHECK;
			is_bfe = 1;
		} else if (!strcmp(cmd_code_str, "start_bfe") && !chan_uuid) {
			cmd_code = CODE_IOINFO_START_ALL;
			is_bfe = 1;
		} else if (!strcmp(cmd_code_str, "stop_bfe") && !chan_uuid) {
			cmd_code = CODE_IOINFO_STOP_ALL;
			is_bfe = 1;
		} else if (!strcmp(cmd_code_str, "check_bfe") && !chan_uuid) {
			cmd_code = CODE_IOINFO_CHECK_ALL;
			is_bfe = 1;
		}

	}

	if (cmd_code == CODE_IOINFO_START) {
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s chan_uuid:%s:ioinfo_start",
			log_header, cmd_code, optind, bwc_uuid, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_ioinfo_start(mgrbwc_p, bwc_uuid, chan_uuid, is_bfe);

	} else if (cmd_code == CODE_IOINFO_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s chan_uuid:%s:ioinfo_stop",
			log_header, cmd_code, optind, bwc_uuid, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_ioinfo_stop(mgrbwc_p, bwc_uuid, chan_uuid, is_bfe);

	} else if (cmd_code == CODE_IOINFO_CHECK) {
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s chan_uuid:%s:ioinfo_check",
			log_header, cmd_code, optind, bwc_uuid, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_ioinfo_check(mgrbwc_p, bwc_uuid, chan_uuid, is_bfe);

	} else if (cmd_code == CODE_IOINFO_START_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s chan_uuid:%s:ioinfo_start_all",
			log_header, cmd_code, optind, bwc_uuid, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_ioinfo_start_all(mgrbwc_p, bwc_uuid, is_bfe);

	} else if (cmd_code == CODE_IOINFO_STOP_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s chan_uuid:%s:ioinfo_stop_all",
			log_header, cmd_code, optind, bwc_uuid, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_ioinfo_stop_all(mgrbwc_p, bwc_uuid, is_bfe);

	} else if (cmd_code == CODE_IOINFO_CHECK_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s chan_uuid:%s:ioinfo_check_all",
			log_header, cmd_code, optind, bwc_uuid, chan_uuid);
		rc = mgrbwc_p->bw_op->bw_ioinfo_check_all(mgrbwc_p, bwc_uuid, is_bfe);

	} else {
		rc = -1;
		nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s chan_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, bwc_uuid, chan_uuid);
		usage_bwc();
	}

	return rc;
}

static int
__bwc_delay_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_delay_func";
	char *bwc_conf;
	char args[4][1024];
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";
	int n;
	int write_delay_first_level, write_delay_second_level, write_delay_first_level_max, write_delay_second_level_max;

	assert(!chan_uuid);
	if (!bwc_uuid) {
		goto out;
	}

	if (optind >= argc) {
		warn_str = "got empty bwc configuration!";
		goto out;
	}

	bwc_conf = argv[optind++];
	n = sscanf(bwc_conf, "%[^:]:%[^:]:%[^:]:%[^:]", args[0], args[1], args[2], args[3]);

	if (n < 4) {
		warn_str = "wrong number of bwc configuration items!";
		goto out;
	}

	write_delay_first_level = atoi(args[0]);
	if (write_delay_first_level > 100 || write_delay_first_level < 0) {
		warn_str = "wrong write_delay_first_level confiugration!";
		goto out;
	}

	write_delay_second_level = atoi(args[1]);
	if (write_delay_second_level > 100 || write_delay_second_level < 0) {
		warn_str = "wrong write_delay_second_level confiugration!";
		goto out;
	}

	if (write_delay_second_level >= write_delay_first_level && write_delay_first_level != 0) {
		warn_str = "write_delay_first_level must be greater than write_delay_second_level!";
		goto out;
	}

	if (write_delay_first_level != 0 && write_delay_second_level == 0) {
		warn_str = "write_delay_second_delay cannot be zero!";
		goto out;
	}

	write_delay_first_level_max = atoi(args[2]);
	write_delay_second_level_max = atoi(args[3]);

	nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s",
				log_header, cmd_code, optind, bwc_uuid);
	rc = mgrbwc_p->bw_op->bw_delay(mgrbwc_p, bwc_uuid, (uint16_t)write_delay_first_level, (uint16_t)write_delay_second_level, (uint32_t)write_delay_first_level_max, (uint32_t)write_delay_second_level_max);
	return rc;

out:
	if (rc) {
		usage_bwc();
		nid_log_warning("%s: mbwc_uuid:%s: %s",
			log_header, bwc_uuid, warn_str);
	}
	return rc;
}

static int
__bwc_flush_empty_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_flush_empty_func";
	char *cmd_code_str = NULL;
	int rc  = 0, cmd_code = CODE_NONE;

	assert(!chan_uuid);

	if (optind < argc) {
		cmd_code_str = argv[optind++];
		if (!strcmp(cmd_code_str, "start"))
			cmd_code = CODE_FLUSH_EMPTY_START;
		else if (!strcmp(cmd_code_str, "stop"))
			cmd_code = CODE_FLUSH_EMPTY_STOP;
	}

	if (cmd_code == CODE_FLUSH_EMPTY_START) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s flush_empty_start",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_flush_empty_start(mgrbwc_p, bwc_uuid);
	} else if (cmd_code == CODE_FLUSH_EMPTY_STOP) {
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: flush_empty_stop",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_flush_empty_stop(mgrbwc_p, bwc_uuid);
	} else {
		rc = -1;
		nid_log_warning("%s: cmd_code=%d, optind:%d mbwc_uuid:%s: got wrong cmd_code",
			log_header, cmd_code, optind, bwc_uuid);
		usage_bwc();
	}

	return rc;
}

static int
__bwc_display_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	assert(!chan_uuid);

	if (optind >= argc) {
		warn_str = "got empty sub command!";
		usage_bwc();
		goto out;
	}

	cmd_str = argv[optind++];

	if (bwc_uuid != NULL && cmd_str != NULL) {
		if (strlen(bwc_uuid) > NID_MAX_UUID) {
			warn_str = "wrong bwc_uuid!";
			usage_bwc();
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
	else if (bwc_uuid == NULL && cmd_str != NULL){
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
		usage_bwc();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, bwc_uuid:%s: display",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_display(mgrbwc_p, bwc_uuid, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, bwc_uuid:%s: display all",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_display_all(mgrbwc_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, bwc_uuid:%s: display",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_display(mgrbwc_p, bwc_uuid, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, bwc_uuid:%s: display",
			log_header, cmd_code, optind, bwc_uuid);
		rc = mgrbwc_p->bw_op->bw_display_all(mgrbwc_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, bwc_uuid=%s: got wrong cmd_code",
			log_header, cmd_code, optind, bwc_uuid);
	}

out:
	if (rc) {
		nid_log_warning("%s: bwc_uuid:%s: %s",
			log_header, bwc_uuid, warn_str);
	}
	return rc;
}

static int
__bwc_hello_func(int argc, char* const argv[], char *bwc_uuid, char *chan_uuid, struct mgrbwc_interface *mgrbwc_p)
{
	char *log_header = "__bwc_hello_func";
	int rc  = -1, cmd_code = CODE_NONE;

	assert(&bwc_uuid);
	assert(&chan_uuid);
	assert(argv);

	if (optind > argc) {
		usage_bwc();
		goto out;
	}

	cmd_code = CODE_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind:%d bwc_uuid:%s hello",
		log_header, cmd_code, optind, bwc_uuid);
	rc = mgrbwc_p->bw_op->bw_hello(mgrbwc_p);

out:
	return rc;
}

typedef int (*bwc_operation_func)(int, char* const * , char *, char *, struct mgrbwc_interface *);
struct manager_bwc_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	bwc_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_bwc_task bwc_task_table[] = {
	{"dropcache", 		"dc", 		CMD_DROPCACHE,		__bwc_dropcache_func,	NULL,	NULL},
	{"information",		"info",		CMD_INFORMATION,	__bwc_information_func,	NULL,	NULL},
	{"throughput",		"tpt",		CMD_THROUGHPUT,		__bwc_throughput_func,	NULL,	NULL},
	{"add",			"add",		CMD_ADD,		__bwc_add_func,		NULL,	NULL},
	{"remove",		"rm",		CMD_REMOVE,		__bwc_remove_func,	NULL,	NULL},
	{"fast_flush",		"ff",		CMD_FFLUSH,		__bwc_fflush_func,	NULL,	NULL},
	{"recover",		"rcv",		CMD_RECOVER,		__bwc_recover_func,	NULL,	NULL},
	{"freeze snapshot",	"freeze",	CMD_SNAPSHOT_FREEZE,	__bwc_snapshot_func,	NULL,	NULL},
	{"unfreeze snapshot",	"unfreeze",	CMD_SNAPSHOT_UNFREEZE,	__bwc_snapshot_func,	NULL,	NULL},
	{"ioinfo",		"ioinfo",	CMD_IOINFO,		__bwc_ioinfo_func,	NULL,	NULL},
	{"update water mark",	"uwm",		CMD_WATER_MARK,		__bwc_water_mark_func,	NULL,	NULL},
	{"update delay",	"ud",		CMD_DELAY,		__bwc_delay_func,	NULL,	NULL},
	{"flush empty",		"fe",		CMD_FLUSH_EMPTY,	__bwc_flush_empty_func,	NULL,	NULL},
	{"display",		"disp",		CMD_DISPLAY,		__bwc_display_func,	NULL,	NULL},
	{"hello",		"hello",	CMD_HELLO,		__bwc_hello_func,	NULL,	NULL},
	{NULL,			NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_dc_setup_description(struct manager_bwc_task *dc_mtask)
{
	dc_mtask->description = "dc:	droping cache command\n";
	dc_mtask->detail = x_malloc(1024*1024);
	sprintf(dc_mtask->detail, "%s",
		first_intend"start:			start droping cache\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c channel_uuid dc start\n"
		first_intend"start_sync:		start droping cache with sync mode\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c channel_uuid dc start_sync\n"
		first_intend"stop:			stop  droping cache\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c channel_uuid dc stop\n");
}

static void
__setup_info_setup_description(struct manager_bwc_task *info_mtask)
{
	info_mtask->description = "info:	information display command\n";

	info_mtask->detail = x_malloc(1024*1024);
	sprintf(info_mtask->detail, "%s",
		first_intend"flushing:		???\n"
		first_intend"stat:			display bwc gerenal status\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid info stat\n"
		first_intend"throughput|tpt:		display bwc throughput information\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid info throughput|tpt\n"
		first_intend"rw:			display bwc flush related information (overwritten&coalesce)\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c channel_uuid info rw\n"
		first_intend"delay:			display bwc delay (flow control) information\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid info delay\n"
		first_intend"rw:			display bwc request list related information \n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c channel_uuid info list\n");
}


static void
__setup_tpt_setup_description(struct manager_bwc_task *tpt_mtask)
{
	tpt_mtask->description = "tpt:	throughput operation command\n";
	tpt_mtask->detail = x_malloc (1024*1024);
	sprintf(tpt_mtask->detail, "%s",
		first_intend"reset:			reset bwc throughput counter\n"
		first_intend"			sample:nidmanager -r s -m bwc tpt reset  -i bwc_uuid  \n");
}

static void
__setup_add_setup_description(struct manager_bwc_task *add_mtask)
{
	add_mtask->description = "add:	add operation command\n";
	add_mtask->detail = x_malloc (1024*1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"	[bufdev]:[bufdev_size]:[rw_sync]:[two_step_read]:[do_fp]:[tp_name]:[ssd_mode]:[max_flush_size]:[write_delay_first_level]:[write_delay_second_level]:bfp1:[high_water_mark][low_water_mark]\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid add /wc_device:4:1:1:1:your_tp_name:0:16:30:10:bfp1:60:40 \n\n"
		first_intend"	[bufdev]:[bufdev_size]:[rw_sync]:[two_step_read]:[do_fp]:[tp_name]:[ssd_mode]:[max_flush_size]:[write_delay_first_level]:[write_delay_second_level]:bfp2"
			":[load_ratio_min]:[load_ratio_max]:[load_ctrl_level]:[flush_delay_ctl]\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid add /wc_device:4:1:1:1:your_tp_name:0:16:30:10:bfp2:0.1:0.8:0.7:20 \n\n"
		first_intend"	[bufdev]:[bufdev_size]:[rw_sync]:[two_step_read]:[do_fp]:[tp_name]:[ssd_mode]:[max_flush_size]:[write_delay_first_level]:[write_delay_second_level]:bfp3"
			":[load_ratio_min]:[load_ratio_max]:[load_ctrl_level]:[flush_delay_ctl]:[coalesce_ratio]\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid add /wc_device:4:1:1:1:your_tp_name:0:16:30:10:bfp3:0.1:0.8:0.7:20:0.4 \n\n"
		first_intend"	[bufdev]:[bufdev_size]:[rw_sync]:[two_step_read]:[do_fp]:[tp_name]:[ssd_mode]:[max_flush_size]:[write_delay_first_level]:[write_delay_second_level]:bfp4"
			":[load_ratio_min]:[load_ratio_max]:[load_ctrl_level]:[flush_delay_ctl]:[throttle_ratio]\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid add /wc_device:4:1:1:1:your_tp_name:0:16:30:10:bfp4:0.1:0.8:0.7:20:0.8 \n\n"
		first_intend"	[bufdev]:[bufdev_size]:[rw_sync]:[two_step_read]:[do_fp]:[tp_name]:[ssd_mode]:[max_flush_size]:[write_delay_first_level]:[write_delay_second_level]:bfp5"
			":[load_ratio_min]:[load_ratio_max]:[load_ctrl_level]:[flush_delay_ctl]:[throttle_ratio]:[coalesce_ratio]\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid add /wc_device:4:1:1:1:your_tp_name:0:16:30:10:bfp5:0.1:0.8:0.7:20:0.8:0.4 \n");
}

static void
__setup_rm_setup_description(struct manager_bwc_task *rm_mtask)
{
	rm_mtask->description = "rm:	remove operation command\n";
	rm_mtask->detail = x_malloc (1024*1024);
	sprintf(rm_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid rm  \n");
}

static void
__setup_ff_setup_description(struct manager_bwc_task *ff_mtask)
{
	ff_mtask->description = "ff:	per channel fast flush command\n";
	ff_mtask->detail = x_malloc(1024*1024);
	sprintf(ff_mtask->detail, "%s",
		first_intend"start:			start fast flushing for specified channel\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c chan_uuid ff start\n"
		first_intend"stop:			stop fast flushing for specified channel\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c chan_uuid ff stop\n"
		first_intend"get:			get fast flushing state for specified channel\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c chan_uuid ff get\n");
}

static void
__setup_rcv_setup_description(struct manager_bwc_task *ff_mtask)
{
	ff_mtask->description = "rcv:	recovery operation command\n";
	ff_mtask->detail = x_malloc(1024*1024);
	sprintf(ff_mtask->detail, "%s",
		first_intend"start:			start recovery\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid rcv start\n"
		first_intend"get:			get recovery state\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid rcv get\n");
}

static void
__setup_snapshot_setup_description(struct manager_bwc_task *ff_mtask)
{
	ff_mtask->description = "freeze/unfreeze:	snapshot operation command\n";
	ff_mtask->detail = x_malloc(1024*1024);
	sprintf(ff_mtask->detail, "%s",
		first_intend"freeze:			freeze an volume by its uuid\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c chan_uuid freeze\n"
		first_intend"unfreeze:		unfreeze an volume by its uuid\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c chan_uuid unfreeze\n");
}

static void
__setup_water_mark_setup_description(struct manager_bwc_task *uwm_mtask)
{
	uwm_mtask->description = "uwm:	update water mark command (high_water_mark must be greater than low_water_mark)\n";
	uwm_mtask->detail = x_malloc (1024);
	sprintf(uwm_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid uwm [high_water_mark]:[low_water_mark]\n");
}

static void
__setup_ioinfo_setup_description(struct manager_bwc_task *ioinfo_mtask)
{
	ioinfo_mtask->description = "ioinfo:	count io information command";

	ioinfo_mtask->detail = x_malloc(2*1024);
	sprintf(ioinfo_mtask->detail, "%s",
		first_intend"start:			start counting request stat\n"
		first_intend"			sample:nidmanager -r s -m bwc ioinfo start\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid ioinfo start\n"
		first_intend"stop:			stop counting request stat\n"
		first_intend"			sample:nidmanager -r s -m bwc ioinfo stop\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid ioinfo stop\n"
		first_intend"check:			check request stat\n"
		first_intend"			sample:nidmanager -r s -m bwc ioinfo check\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid ioinfo check\n"
		first_intend"start_bfe:		start counting bfe request stat\n"
		first_intend"			sample:nidmanager -r s -m bwc ioinfo start_bfe\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid ioinfo start_bfe\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c chan_uuid ioinfo start_bfe\n"
		first_intend"stop_bfe:		stop counting bfe request stat\n"
		first_intend"			sample:nidmanager -r s -m bwc ioinfo stop_bfe\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid ioinfo stop_bfe\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c chan_uuid ioinfo stop_bfe\n"
		first_intend"check_bfe:		check bfe request stat\n"
		first_intend"			sample:nidmanager -r s -m bwc ioinfo check_bfe\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid ioinfo check_bfe\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid -c chan_uuid ioinfo check_bfe\n");
}

static void
__setup_delay_setup_description(struct manager_bwc_task *ud_mtask)
{
	ud_mtask->description = "ud:	update write delay setting command (write_delay_first_level must be greater than write_delay_second_level)\n";
	ud_mtask->detail = x_malloc (1024);
	sprintf(ud_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid ud [write_delay_first_level]:[write_delay_second_level]:[write_delay_first_level_max]:[write_delay_second_level_max]\n");
}

static void
__setup_flush_empty_setup_description(struct manager_bwc_task *fe_mtask)
{
	fe_mtask->description = "fe:	flush empty segments onto bufdevice command\n";
	fe_mtask->detail = x_malloc(1024*1024);
	sprintf(fe_mtask->detail, "%s",
		first_intend"start:			start flushing empty segments\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid fe start\n"
		first_intend"stop:			stop flushing empty segments\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid fe stop\n");
}

static void
__setup_display_setup_description(struct manager_bwc_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of bwc";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show bwc configuration\n"
		first_intend"			sample:nidmanager -r s -m bwc display setup\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid display setup\n"
		first_intend"working:		show working bwc configuration\n"
		first_intend"			sample:nidmanager -r s -m bwc display working\n"
		first_intend"			sample:nidmanager -r s -m bwc -i bwc_uuid display working\n");
}

static void
__setup_hello_setup_description(struct manager_bwc_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m bwc hello\n");
}

static void
setup_descriptions()
{
	struct manager_bwc_task *mtask;
	int snapshot_setup_displayed = 0;

	mtask = &bwc_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_DROPCACHE)
			__setup_dc_setup_description(mtask);
		else if (mtask->cmd_index == CMD_INFORMATION)
			__setup_info_setup_description(mtask);
		else if (mtask->cmd_index == CMD_THROUGHPUT)
			__setup_tpt_setup_description(mtask);
		else if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
		else if (mtask->cmd_index == CMD_REMOVE)
			__setup_rm_setup_description(mtask);
		else if (mtask->cmd_index == CMD_FFLUSH)
			__setup_ff_setup_description(mtask);
		else if (mtask->cmd_index == CMD_RECOVER)
			__setup_rcv_setup_description(mtask);
		else if (mtask->cmd_index == CMD_SNAPSHOT_FREEZE || mtask->cmd_index == CMD_SNAPSHOT_UNFREEZE) {
			if(!snapshot_setup_displayed) {
				__setup_snapshot_setup_description(mtask);
				snapshot_setup_displayed = 1;
			}
		}
		else if (mtask->cmd_index == CMD_WATER_MARK)
			__setup_water_mark_setup_description(mtask);
		else if (mtask->cmd_index == CMD_IOINFO)
			__setup_ioinfo_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DELAY)
			__setup_delay_setup_description(mtask);
		else if (mtask->cmd_index == CMD_FLUSH_EMPTY)
			__setup_flush_empty_setup_description(mtask);
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
usage_bwc()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_bwc_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m bwc ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &bwc_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (bwc_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &bwc_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		if(task->description && task->detail) {
			printf("%s\n",task->description );
			printf("%s\n",task->detail);
		}

	}
	printf ("\n");

	return;
}

#define VAL_SNAPSHOT_NM			1
#define	VAL_SNAPSHOT_ID			2
#define VAL_SNAPSHOT_CALLBACK	3
#define VAL_SNAPSHOT_FREEZE		4
#define VAL_SNAPSHOT_UNFREEZE	5

static char *optstr_ss = "a:i:c:";
static struct option long_options_ss[] = {
	{"vec", required_argument, NULL, 'v'},
	{0, 0, 0, 0},
};

int
manager_server_bwc(int argc, char * const argv[])
{
	char *log_header = "manager_server_bwc";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	char *bwc_uuid = NULL;
	char *chan_uuid = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrbwc_interface *mgrbwc_p;
	struct mgrbwc_setup setup;
	struct manager_bwc_task *mtask = &bwc_task_table[0];
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
			bwc_uuid = optarg;
			break;

		case 'c':
			chan_uuid = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_bwc();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;

	mgrbwc_p = x_calloc(1, sizeof(*mgrbwc_p));
	mgrbwc_initialization(mgrbwc_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_bwc();
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
		usage_bwc();
		rc = -1;
		goto out;
	}

	rc = mtask->func(argc, argv, bwc_uuid, chan_uuid, mgrbwc_p);

out:
	return rc;
}
