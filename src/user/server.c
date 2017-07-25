#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include "version.h"
#include "nid_log.h"
#include "nid.h"
#include "ini_if.h"
#include "allocator_if.h"
#include "mpk_if.h"
#include "umpk_if.h"
#include "tp_if.h"
#include "tpg_if.h"
#include "ds_if.h"
#include "scg_if.h"
#include "nw_if.h"
#include "server.h"
#include "version.h"
#include "pp2_if.h"
#include "lstn_if.h"
#include "txn_if.h"
#include "mqtt_if.h"

#ifdef METASERVER
#include "ms_intf.h"
#endif

int
main()
{
	char *log_header = "main-server";
	struct allocator_interface *allocator_p;
	struct allocator_setup allocator_setup;
	struct mpk_setup mpk_setup;
	struct mpk_interface *mpk_p;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct ini_setup ini_setup;
	struct ini_interface *ini_p;
	struct ini_section_content *global_sect;
	struct ini_key_content *the_key;
	struct tpg_interface *tpg_p;
	struct tpg_setup tpg_setup;
	struct scg_setup scg_setup;
	struct scg_interface *scg_p;
	struct nw_setup nw_setup;
	struct nw_interface *nw_p;
	struct sigaction sa;
	struct rlimit rlim;
	struct stat buf;
	struct pp2_setup pp2_setup;
	struct pp2_interface *pp2_p, *dyn_pp2_p;
	struct lstn_setup lstn_setup;
	struct lstn_interface *lstn_p;
	struct txn_setup txn_setup;
	struct txn_interface *txn_p;
	struct list_head conf_key_set;
	struct list_node *conf_key_node;
	struct ini_key_desc *conf_key;
	struct mqtt_interface *mqtt_p;
	struct mqtt_setup mqtt_setup;
	struct mqtt_message msg;
	int rc;

	printf("%s: nid_log_level:%d\n", log_header, nid_log_level);
	daemon(1, 0);
	nid_log_open();
	nid_log_warning("%s: version:%s, log_level:%d start ...", log_header, NID_VERSION, nid_log_level);

	if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
		rlim.rlim_cur = RLIM_INFINITY;
		rlim.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
			nid_log_error("nid server setrlimit failed");
		}
	}

	rc = stat(NID_CONF_SERVICE, &buf);
	if (rc) {
		nid_log_notice("%s: %s! nidserver exit.", NID_CONF_SERVICE, strerror(errno));
		exit(1);
	}

	/*
	 * Create allocator
	 */
	allocator_p = calloc(1, sizeof(*allocator_p));
	allocator_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(allocator_p, &allocator_setup);

	/*
	 * Create pp2
	 */
	pp2_p = calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "server_pp2";
	pp2_setup.allocator = allocator_p;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_PP2;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 64;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 1;
	pp2_initialization(pp2_p, &pp2_setup);

	/*
	 *Create dynamic pp2
	 */
	dyn_pp2_p = calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "server_dynamic_pp2";
	pp2_setup.allocator = allocator_p;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_DYN_PP2;
	pp2_setup.page_size = 1;
	pp2_setup.pool_size = 0;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 0;
	pp2_initialization(dyn_pp2_p, &pp2_setup);

	/*
	 * Create umpk
	 */
	memset(&umpk_setup, 0, sizeof(umpk_setup));
	umpk_p = calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	/*
	 * Create mpk (legacy stuff, will be obsoleted by umpk)
	 */
	memset(&mpk_setup, 0, sizeof(mpk_setup));
	mpk_setup.type = 0;
	mpk_setup.pp2 = pp2_p;
	mpk_setup.do_pp2 = 1;
	mpk_p = calloc(1, sizeof(*mpk_p));
	mpk_initialization(mpk_p, &mpk_setup);

	/*
	 * Create lstn
	 */
	lstn_setup.allocator = allocator_p;
	lstn_setup.set_id = ALLOCATOR_SET_SERVER_LSTN;
	lstn_setup.seg_size = 128;
	lstn_p = calloc(1, sizeof(*lstn_p));
	lstn_initialization(lstn_p, &lstn_setup);

	/*
	 * Create txn
	 */
	txn_setup.allocator = allocator_p;
	txn_setup.seg_size = 128;
	txn_setup.set_id = ALLOCATOR_SET_CXP_TXN;
	txn_p = calloc(1, sizeof(*txn_p));
	txn_initialization(txn_p, &txn_setup);

	/*
	 * Create ini
	 */
	INIT_LIST_HEAD(&conf_key_set);
	server_generate_conf_tmplate(&conf_key_set, lstn_p);
	list_for_each_entry(conf_key_node, struct list_node, &conf_key_set, ln_list) {
		conf_key = (struct ini_key_desc *)conf_key_node->ln_data;
		while (conf_key->k_name) {
			if (!conf_key->k_description) {
				nid_log_error("%s: key: %s has no description.", log_header, conf_key->k_name);
				return 0;
			}
			conf_key++;
		}
	}
	memset(&ini_setup, 0, sizeof(ini_setup));
	ini_setup.path = NID_CONF_SERVICE;
	ini_setup.key_set = conf_key_set;
	ini_p = calloc(1, sizeof(*ini_p));
	ini_initialization(ini_p, &ini_setup);
	ini_p->i_op->i_parse(ini_p);
	global_sect = ini_p->i_op->i_search_section(ini_p, "global");
	assert(global_sect);

	/*
	 * Create thread pool for guardian
	 */
	memset(&tpg_setup, 0, sizeof(tpg_setup));
	tpg_setup.ini = ini_p;
	tpg_setup.pp2 = pp2_p;
	tpg_p = calloc(1, sizeof(*tpg_p));
	tpg_initialization(tpg_p, &tpg_setup);

	/*
	 * Create mqtt
	 */
	memset(&mqtt_setup, 0, sizeof(mqtt_setup));
	mqtt_setup.client_id = "nidserver";
	mqtt_setup.clean_session = 0;
	mqtt_setup.pp2 = pp2_p;
	mqtt_setup.role = NID_ROLE_SERVICE;
	mqtt_p = calloc(1, sizeof(*mqtt_p));
	mqtt_initialization(mqtt_p, &mqtt_setup);

	msg.type = "nidserver";
	msg.module = "NIDServer";
	msg.message = "NID server started";
	msg.status = "OK";
	strcpy (msg.uuid, "NULL");
	strcpy (msg.ip, "NULL");

	mqtt_p->mt_op->mt_publish(mqtt_p, &msg);

	/*
	 * setup scg
	 */
	memset(&scg_setup, 0, sizeof(scg_setup));
	scg_setup.allocator = allocator_p;
	scg_setup.mpk = mpk_p;
	scg_setup.umpk = umpk_p;
	scg_setup.tpg = tpg_p;
	scg_setup.ini = ini_p;
	scg_setup.pp2 = pp2_p;
	scg_setup.lstn = lstn_p;
	scg_setup.txn = txn_p;
	scg_setup.mqtt = mqtt_p;


    the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "log_level");
	nid_log_set_level(*(int *)the_key->k_value);
	/*
	 * Get smc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_smc");
	scg_setup.support_smc = *(int *)the_key->k_value;

	/*
	 * Get wcc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_wcc");
	scg_setup.support_wcc = *(int *)the_key->k_value;

	/*
	 * Get rcc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_rcc");
	scg_setup.support_rcc = *(int *)the_key->k_value;

	/*
	 * Get bwcc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_bwcc");
	scg_setup.support_bwcc = *(int *)the_key->k_value;

	/*
	 * Get twcc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_twcc");
	scg_setup.support_twcc = *(int *)the_key->k_value;

	/*
	 * Get crcc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_crcc");
	scg_setup.support_crcc = *(int *)the_key->k_value;

	/*
	 * Get sac support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_sac");
	scg_setup.support_sac = *(int *)the_key->k_value;

	/*
	 * Get sacc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_sacc");
	scg_setup.support_sacc = *(int *)the_key->k_value;

	/*
	 * Get sdsc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_sdsc");
	scg_setup.support_sdsc = *(int *)the_key->k_value;

	/*
	 * Get ppc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_ppc");
	scg_setup.support_ppc = *(int *)the_key->k_value;

	/*
	 * Get ds support type
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_cds");
	scg_setup.support_cds = *(int *)the_key->k_value;
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_sds");
	scg_setup.support_sds = *(int *)the_key->k_value;

	/*
	 * Get io support type
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_rio");
	scg_setup.support_rio = *(int *)the_key->k_value;
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_bio");
	scg_setup.support_bio = *(int *)the_key->k_value;

	/*
	 * Get bwc support type
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_bwc");
	scg_setup.support_bwc = *(int *)the_key->k_value;

	/*
	 * Get twc support type
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_twc");
	scg_setup.support_twc = *(int *)the_key->k_value;

	/*
	 * Get rc support type
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_crc");
	scg_setup.support_crc = *(int *)the_key->k_value;

	/*
	 * Get rw support type
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_drw");
	scg_setup.support_drw = *(int *)the_key->k_value;
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_drwc");
	scg_setup.support_drwc = *(int *)the_key->k_value;

	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_mrw");
	scg_setup.support_mrw = *(int *)the_key->k_value;
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_mrwc");
	scg_setup.support_mrwc = *(int *)the_key->k_value;

	/*
	 * Get dxt support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_dxt");
	scg_setup.support_dxt = *(int *) the_key->k_value;

	/*
	 * Get dxa support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_dxa");
	scg_setup.support_dxa = *(int *) the_key->k_value;
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_dxac");
	scg_setup.support_dxac = *(int *) the_key->k_value;

	/*
	 * Get dxp support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_dxp");
	scg_setup.support_dxp = *(int *) the_key->k_value;
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_dxpc");
	scg_setup.support_dxpc = *(int *) the_key->k_value;

	/*
	 * Get cxa support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_cxa");
	scg_setup.support_cxa = *(int *) the_key->k_value;
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_cxac");
	scg_setup.support_cxac = *(int *) the_key->k_value;

	/*
	 * Get cxp support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_cxp");
	scg_setup.support_cxp = *(int *) the_key->k_value;
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_cxpc");
	scg_setup.support_cxpc = *(int *) the_key->k_value;

	/*
	 * Get cxt support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_cxt");
	scg_setup.support_cxt = *(int *) the_key->k_value;

	/*
	 * Get tpc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_tpc");
	scg_setup.support_tpc = *(int *) the_key->k_value;

	/*
	 * Get doac support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_doac");
	scg_setup.support_doac = *(int *) the_key->k_value;

	/*
	 * Get inic support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_inic");
	scg_setup.support_inic = *(int *) the_key->k_value;

	/*
	 * Get rept support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_rept");
	scg_setup.support_rept = *(int *) the_key->k_value;

	/*
	 * Get reps support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_reps");
	scg_setup.support_reps = *(int *) the_key->k_value;

	/*
	 * Get reptc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_reptc");
	scg_setup.support_reptc = *(int *) the_key->k_value;

	/*
	 * Get repsc support
	 */
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "support_repsc");
	scg_setup.support_repsc = *(int *) the_key->k_value;

	/*
	 * Create scg
	 */
	scg_setup.server_keys = conf_key_set;
	scg_p = calloc(1, sizeof(*scg_p));
	scg_initialization(scg_p, &scg_setup);

	/*
	 * Create Network
	 */
	memset(&nw_setup, 0, sizeof(nw_setup));
	nw_setup.type = NID_ROLE_SERVICE;
	nw_setup.tpg = tpg_p;
	nw_setup.cg = (void *)scg_p;
	nw_setup.port = NID_SERVICE_PORT;
	nw_setup.pp2 = pp2_p;
	nw_setup.dyn_pp2 = dyn_pp2_p;
	nw_p = calloc(1, sizeof(*nw_p));
	nw_initialization(nw_p, &nw_setup);

	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		exit(1);
	}

	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) == -1) {
		exit(1);
	}

	nid_log_info("while sleeping...");
	while (1)
		sleep(10);

	nid_log_warning("%s: end...", log_header);
	nid_log_close();


	return 0;
}
