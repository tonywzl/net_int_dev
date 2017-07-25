/*
 * admin.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "version.h"
#include "nid.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "msc_if.h"
#include "mac_if.h"
#include "manager.h"

#define M_ROLE_DEFAULT		0
#define M_ROLE_SERVER		1
#define M_ROLE_AGENT		2
#define M_ROLE_DRIVER		3
#define M_ROLE_MAX		4

#define M_MOD_BIO		1
#define M_MOD_DOA		2
#define M_MOD_WC		3
#define M_MOD_RC		4
#define M_MOD_BWC		5
#define M_MOD_CRC		6
#define M_MOD_MRW		9
#define M_MOD_SAC		10
#define M_MOD_PP		11
#define M_MOD_SDS		12
#define M_MOD_DRW		13
#define M_MOD_DX		14
#define M_MOD_CX		15
#define M_MOD_TWC		16
#define M_MOD_TP		18
#define M_MOD_INI		19
#define M_MOD_SERVER		22
#define M_MOD_AGENT		23
#define M_MOD_DRIVER		24
#define M_MOD_ALL		25
#define M_MOD_MAX		26


//int nid_log_level = LOG_WARNING;
int nid_log_level = LOG_DEBUG;

typedef int (*mgr_operation_func)(int, char * const *);
struct manager_task {
	int			role;
	int			module;
	char			*mod_name;
	mgr_operation_func	func;
	char			*description;
};

struct manager_task nid_task_table[] = {
	{M_ROLE_SERVER,	M_MOD_BIO,	"cache",	manager_server_bio, NULL},
	{M_ROLE_SERVER,	M_MOD_DOA,	"doa",		manager_server_doa, "sub command of Delegation of Authority module"},
	{M_ROLE_SERVER,	M_MOD_WC,	"wc",		manager_server_wc, "sub command of Write Cache module"},
//	{M_ROLE_SERVER,	M_MOD_RC,	"rc",		manager_server_rc, "sub command of Read Cache module"},
	{M_ROLE_SERVER,	M_MOD_BWC,	"bwc",		manager_server_bwc, "sub command of Write Back Write Cache module"},
	{M_ROLE_SERVER,	M_MOD_CRC,	"crc",		manager_server_crc, "sub command of CAS Read Cache module"},
	{M_ROLE_SERVER,	M_MOD_MRW,	"mrw",		manager_server_mrw, "sub command of Meta Server Read Write module"},
	{M_ROLE_SERVER,	M_MOD_SAC,	"sac",		manager_server_sac, "sub command of Server Agent Channel module"},
	{M_ROLE_SERVER,	M_MOD_PP,	"pp",		manager_server_pp, "sub command of Page Pool module"},
	{M_ROLE_SERVER,	M_MOD_SDS,	"sds",		manager_server_sds, "sub command of Split Data Stream module"},
	{M_ROLE_SERVER,	M_MOD_DRW,	"drw",		manager_server_drw, "sub command of Device Read Write module"},
	{M_ROLE_SERVER,	M_MOD_DX,	"dx",		manager_server_dx, "sub command of Data Exchange module"},
	{M_ROLE_SERVER,	M_MOD_CX,	"cx",		manager_server_cx, "sub command of Control Exchange module"},
	{M_ROLE_SERVER,	M_MOD_TWC,	"twc",		manager_server_twc, "sub command of Write Through Write Cache module"},
	{M_ROLE_SERVER, M_MOD_TP,	"tp",		manager_server_tp, "sub command of Thread Pool module"},
	{M_ROLE_SERVER, M_MOD_INI,	"ini",		manager_server_ini, "sub command of INI module"},
	{M_ROLE_SERVER, M_MOD_SERVER,	"server",	manager_server_server, "sub command of Server module"},
	{M_ROLE_AGENT,	M_MOD_AGENT,	"agent",	manager_agent_agent, "sub command of Agent module"},
	{M_ROLE_DRIVER, M_MOD_DRIVER,	"driver",	manager_driver_driver, "sub command of Driver module"},
	{M_ROLE_SERVER, M_MOD_ALL,	"all",		manager_server_all, "sub command of All server modules"},
	{M_ROLE_MAX,	M_MOD_MAX,	NULL,		NULL, NULL},
};

void
usage()
{
	char *log_header = "usage";
	struct manager_task *task;
	int i;
	printf("%s: nidmanager [options]\n", log_header);
	printf("   -r   a|s|d   sub command of agent|server|driver role\n");
	printf("   -m");
	printf("   ");
	for (i = 0; i < M_MOD_MAX ; i++){
		task = &nid_task_table[i];
		if (task->mod_name == NULL)
			break;
		printf("%s	%s ", task->mod_name, task->description);
		printf("\n");
		printf("     ");
		if (nid_task_table[i+1].mod_name != NULL)
			printf("   |");
	}
	printf("\n");
	return;
}

void
legacy_usage()
{
	printf("legacy_usage: idbmanager [options]\n");
	printf("Options:\n");
//	printf("  --mode    choose mode: status|control|debug\n");
	printf("  --role    choose role: agent|server|driver\n");
//	printf("  --job     choose job: bio\n");
	printf("  --module/M    choose module: cache/lck/dck\n");
	printf("  -a    IP address of server/agent/driver\n");
	printf("  -b    bio related\n");
	printf("  -c    check the connection\n");
	printf("  -e    send error notification\n");
	printf("  -d    delete <dev>\n");
	printf("  -i    uuid\n");
	printf("  -r    role[s|d|a]\n");
	printf("  -s    status\n");
	printf("  -S    stop\n");
	printf("  -u    update\n");
	printf("  -U    upgrade <dev>\n");
	printf("  -F    forcely upgrade <dev>\n");
	printf("  -l    set log level <0-7>\n");
	printf("  -v    show version\n");
	printf("  -h    enable/disable hook functions\n");

	return;
}

char *optstr = "za:b:cd:e:i:m::r:s:SuU:F:l:vh:";
struct option long_options[] = {
	{"module", required_argument, NULL, 'm'},
	{"role", required_argument, NULL, 'r'},
	{0, 0, 0, 0},
};

int
main(int argc, char* argv[])
{
	char *log_header = "nidmanager";
	int i;
	int order = 0;
	char c, role;
	char *ipstr = "127.0.0.1";
	char *uuid = NULL, *devname = NULL, *bio_cmd = NULL, *uuid_check = NULL, *fname = NULL;
	int is_service = 0, is_agent = 0, is_driver = 0;
	int len = 0;
	off_t offset = 0;
	int nonspecial = 0;
	int do_check_conn = 0,
	    do_mem_size = 0,
	    do_bio = 0,
	    do_stop = 0,
	    do_ioerror = 0,
	    do_delete = 0,
	    do_stat_get = 0,
	    do_stat_get_wd = 0,
	    do_stat_get_wu = 0,
	    do_stat_get_wud = 0,
	    do_stat_get_ud = 0,
	    do_stat_get_rwwd = 0,
	    do_stat_get_rwwu = 0,
	    do_stat_get_rwwud = 0,
	    do_stat_reset = 0,
	    do_update = 0,
	    do_upgrade = 0,
	    do_force_upgrade = 0,
	    do_version = 0,
	    do_hook_cmd = 0,
	    do_set_log_level = 0,
	    do_read = 0;
	int cmd_role = M_ROLE_DEFAULT;
	int rc = 0;
	struct mpk_setup mpk_setup;
	struct mpk_interface *mpk_p;
	int level;
	int cmd;

#ifdef DEBUG_LEVEL
	nid_log_level = DEBUG_LEVEL;
#endif

	while (optind < argc) {
		if ((c=getopt_long(argc, argv, optstr, long_options, &i)) >= 0) {
			order++;
			if (order == 1 && c == 'r') {
				if (!strcmp(optarg, "server") || !strcmp(optarg, "s")) {
					is_service = 1;
					cmd_role = M_ROLE_SERVER;
					nid_log_debug("this is nid service manager");
				} else if (!strcmp(optarg, "driver") || !strcmp(optarg, "d")) {
					is_driver = 1;
					cmd_role = M_ROLE_DRIVER;
					nid_log_debug("this is nid driver manager");
				} else if (!strcmp(optarg, "agent") || !strcmp(optarg, "a")) {
					is_agent = 1;
					cmd_role = M_ROLE_AGENT;
					nid_log_debug("this is nid agent manager");
				} else {
					nid_log_error("%s: got wrong role (%s)", log_header, optarg);
					printf("%s: got wrong role (%s)\n", log_header, optarg);
					usage();
					return 1;
				}
				continue;
			}

			if (order == 2 && cmd_role != M_ROLE_DEFAULT){
				if (c == 'm') {
					if (!argv[optind] && !optarg){
						usage();
						rc= 1;
						return rc;
					}
					struct manager_task *mtask = &nid_task_table[0];

					while (mtask->role < M_ROLE_MAX) {
						assert(mtask->module < M_MOD_MAX);
						if (!optarg) {
							if (cmd_role == mtask->role && !strcmp(argv[optind], mtask->mod_name)) {
								break;
							}
						} else {
							if (cmd_role == mtask->role && !strcmp(optarg, mtask->mod_name)) {
								break;
							}
						}
						mtask++;
					}

					if (mtask->role < M_ROLE_MAX) {
						if (!optarg)
							optind++;
						rc = mtask->func(argc, argv);
					} else {
						usage();
						rc = 1;
					}
					return rc;
				}
			}

			/* Legacy part */
			switch (c) {
			case 'z':
				printf("printf memory information:\n");
				do_mem_size = 1;
				break;

			case 'a':
				ipstr = optarg;
				nid_log_debug("got ipaddr: %s", ipstr);
				break;

			case 'b':
				bio_cmd = optarg;
				do_bio = 1;
				nid_log_debug("got bio_cmd: %s", bio_cmd);
				break;

			case 'c':
				do_check_conn = 1;
				nid_log_debug("need do_check_conn");
				break;

			case 'e':
				do_ioerror = 1;
				devname = optarg;
				nid_log_debug("got devname: %s", devname);
				break;

			case 'd':
				do_delete = 1;
				devname = optarg;
				nid_log_debug("delete devname: %s", devname);
				break;

			case 'i':
				uuid = optarg;
				nid_log_debug("got uuid: %s", uuid);
				break;

			case 'r':
				role = *optarg;
				if (role == 's') {
					nid_log_debug("this is nid service manager");
					is_service = 1;
				} else if (role == 'd') {
					nid_log_debug("this is nid driver manager");
					is_driver = 1;
				} else if (role == 'a') {
					nid_log_debug("this is nid agent manager");
					is_agent = 1;
				} else {
					legacy_usage();
					exit(1);
				}
				break;

			case 's':
				if (!strcmp(optarg, "get")) {
					nid_log_debug("this is get stat command");
					do_stat_get = 1;
				} else if (!strcmp(optarg, "get_wd")) {
					nid_log_debug("this is get_wd stat command");
					do_stat_get_wd = 1;
				} else if (!strcmp(optarg, "get_wu")) {
					nid_log_debug("this is get_wu stat command");
					do_stat_get_wu = 1;
				} else if (!strcmp(optarg, "get_wud")) {
					nid_log_debug("this is get_wud stat command");
					do_stat_get_wud = 1;
				} else if (!strcmp(optarg, "get_rwwd")) {
					nid_log_debug("this is get_rwud stat command");
					do_stat_get_rwwd = 1;
				} else if (!strcmp(optarg, "get_rwwu")) {
					nid_log_debug("this is get_rwwu stat command");
					do_stat_get_rwwu = 1;
				} else if (!strcmp(optarg, "get_rwwud")) {
					nid_log_debug("this is get_rwwud stat command");
					do_stat_get_rwwud = 1;
				} else if (!strcmp(optarg, "get_ud")) {
					nid_log_debug("this is get_ud stat command");
					do_stat_get_ud = 1;
				} else if (!strcmp(optarg, "reset")) {
					nid_log_debug("this is reset stat command");
					do_stat_reset = 1;
				} else {
					nid_log_debug("bad stat command, should be get|reset");
					legacy_usage();
					exit(1);
				}
				break;

			case 'S':
				nid_log_debug("this is stop command");
				do_stop = 1;
				break;

			case 'u':
				nid_log_debug("this is update command");
				do_update = 1;
				break;

			case 'U':
				nid_log_debug("this is upgrade command");
				devname = optarg;
				do_upgrade = 1;
				break;

			case 'F':
				nid_log_debug("this is forcely upgrade command");
				devname = optarg;
				do_force_upgrade = 1;
				break;

			case 'l':
				level = atoi(optarg);
				if (level >= LOG_EMERG && level <= LOG_DEBUG) {
					printf("Set log level to %d\n", level);
					nid_log_debug("Set log level to %d", level);
					do_set_log_level = 1;
				} else {
					printf("log level should be <0-7>\n");
					exit(1);
				}
				break;

			case 'v':
				printf("Manager build time: %s %s\n", __DATE__, __TIME__);
				do_version = 1;
				break;

			case 'h':
				nid_log_debug("%s hook functions", optarg);
				if (strcmp(optarg, "enable") == 0) {
					cmd = 1;
					do_hook_cmd = 1;
				}else if (strcmp(optarg, "disable") == 0) {
					cmd = 0;
					do_hook_cmd = 1;
				} else {
					printf("%s is wrong\n", optarg);
					printf("nidmanager -r a -h [enable|disable]\n");
					exit(1);
				}
				break;

			default:
				legacy_usage();
				exit(1);
			}
		} else {
			printf("non-option ARGV-elements: ");
			while (optind < argc){
				switch (nonspecial++){
					case 0:
						if (!strcmp (argv[optind++], "read")){
							do_read = 1;
						} else {
							legacy_usage();
							exit(1);
						}
						break;
					case 1:
						uuid_check = argv[optind++];
						break;
					case 2:
						offset = atol (argv[optind++]);
						break;
					case 3:
						len = atoi (argv[optind++]);
						break;
					case 4:
						fname = argv[optind++];
						break;
					default:
						break;
				  }

			}


		}
	}

	nid_log_warning("this is nid admin (version:%s) for %s", NID_VERSION, ipstr);

	if (is_service + is_agent + is_driver != 1) {
		legacy_usage();
		exit(1);
	}

	mpk_setup.type = 0;
	mpk_setup.do_pp2 = 0;
	mpk_p = calloc(1, sizeof(*mpk_p));
	mpk_initialization(mpk_p, &mpk_setup);

	if (is_service) {
		struct msc_interface *msc_p;
		struct msc_setup setup;

		setup.ipstr = ipstr;
		setup.port = NID_SERVICE_PORT;
		setup.mpk = mpk_p;
		msc_p = calloc(1, sizeof(*msc_p));
		msc_initialization(msc_p, &setup);

		if (do_stop) {
			rc = msc_p->m_op->m_stop(msc_p);
		} else if (do_bio) {
			if (!strcmp(bio_cmd, "fast_flush") || !strcmp(bio_cmd, "ff")) {
				rc = msc_p->m_op->m_bio_fast_flush(msc_p);
			} else if (!strcmp(bio_cmd, "stop_fast_flush") || !strcmp(bio_cmd, "stop_ff")) {
				rc = msc_p->m_op->m_bio_stop_fast_flush(msc_p);
			} else {
				printf("got wrong bio command {%s}\n", bio_cmd);
			}
		} else if (do_check_conn) {
			rc = msc_p->m_op->m_check_conn(msc_p, uuid);
		} else if (do_stat_get) {
			struct msc_stat stat;
			rc = msc_p->m_op->m_stat_get(msc_p, &stat);
		} else if (do_stat_reset) {
			rc = msc_p->m_op->m_stat_reset(msc_p);
		} else if (do_mem_size) {
			rc = msc_p->m_op->m_mem_size(msc_p);
		} else if (do_update) {
			rc = msc_p->m_op->m_update(msc_p);
		} else if (do_set_log_level) {
			rc = msc_p->m_op->m_set_log_level(msc_p, level);
		} else if (do_version) {
			rc = msc_p->m_op->m_get_version(msc_p);
		} else if (do_read){
			nid_log_error();
			rc = msc_p->m_op->m_do_read(msc_p, uuid_check, offset, len, fname);
		} else {
			rc = msc_p->m_op->m_check_server(msc_p);
		}
	} else if (is_agent) {
		struct mac_interface *mac_p;
		struct mac_setup setup;

		setup.ipstr = ipstr;
		setup.port = NID_AGENT_PORT;
		setup.mpk = mpk_p;
		mac_p = calloc(1, sizeof(*mac_p));
		mac_initialization(mac_p, &setup);

		if (do_stop) {
			mac_p->m_op->m_stop(mac_p);
		} else if (do_ioerror) {
			rc = mac_p->m_op->m_ioerror(mac_p, devname);
		} else if (do_delete) {
			rc = mac_p->m_op->m_delete(mac_p, devname);
		} else if (do_check_conn) {
			rc = mac_p->m_op->m_check_conn(mac_p, uuid);
		} else if (do_stat_get) {
			struct mac_stat stat;
			rc = mac_p->m_op->m_stat_get(mac_p, &stat);
		} else if (do_stat_get_wd) {
			struct mac_stat stat;
			rc = mac_p->m_op->m_stat_get_wd(mac_p, &stat);
		} else if (do_stat_get_wu) {
			struct mac_stat stat;
			rc = mac_p->m_op->m_stat_get_wu(mac_p, &stat);
		} else if (do_stat_get_wud) {
			struct mac_stat stat;
			rc = mac_p->m_op->m_stat_get_wud(mac_p, &stat);
		} else if (do_stat_get_ud) {
			struct mac_stat stat;
			rc = mac_p->m_op->m_stat_get_ud(mac_p, &stat);
		} else if (do_stat_get_rwwd) {
			struct mac_stat stat;
			rc = mac_p->m_op->m_stat_get_rwwd(mac_p, &stat);
		} else if (do_stat_get_rwwu) {
			struct mac_stat stat;
			rc = mac_p->m_op->m_stat_get_rwwu(mac_p, &stat);
		} else if (do_stat_get_rwwud) {
			struct mac_stat stat;
			rc = mac_p->m_op->m_stat_get_rwwud(mac_p, &stat);
		} else if (do_stat_reset) {
			rc = mac_p->m_op->m_stat_reset(mac_p);
		} else if (do_update) {
			rc = mac_p->m_op->m_update(mac_p);
		} else if (do_upgrade) {
			rc = mac_p->m_op->m_upgrade(mac_p, devname);
		} else if (do_force_upgrade) {
			rc = mac_p->m_op->m_upgrade_force(mac_p, devname);
		} else if (do_set_log_level) {
			rc = mac_p->m_op->m_set_log_level(mac_p, level);
		} else if (do_version) {
			rc = mac_p->m_op->m_get_version(mac_p);
		} else if (do_hook_cmd) {
			rc = mac_p->m_op->m_set_hook(mac_p, cmd);
		} else {
			printf("checking agent...\n");
			rc = mac_p->m_op->m_check_agent(mac_p);
		}
	}

	return rc;
}
