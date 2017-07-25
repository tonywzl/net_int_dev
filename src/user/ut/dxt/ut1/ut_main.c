#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid.h"
#include "nid_log.h"
#include "allocator_if.h"
#include "dxa_if.h"
#include "dxag_if.h"
#include "dxt_if.h"
#include "dxtg_if.h"
#include "ini_if.h"
#include "lstn_if.h"
#include "pp_if.h"
#include "pp2_if.h"
#include "server.h"
#include "tpg_if.h"
#include "umpk_if.h"
#include "umpk_dxa_if.h"
#include "umpk_dxp_if.h"
#include "sdsg_if.h"
#include "cdsg_if.h"
#include "txn_if.h"
#include "ppg_if.h"
#include "tx_shared.h"

#define  NID_CONF_SERVER "nidserver.conf"

int
dxag_prepare(struct dxag_interface *dxag_p, struct dxag_setup *setup)
{
	char *log_header = "dxag_prepare";
	assert(dxag_p && setup);

	struct allocator_setup ut_dxag_al_setup;
	struct allocator_interface *ut_dxag_al;
	memset(&ut_dxag_al_setup, 0, sizeof(ut_dxag_al_setup));
	ut_dxag_al = calloc(1, sizeof(*ut_dxag_al));
	ut_dxag_al_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(ut_dxag_al, &ut_dxag_al_setup);

	struct pp_setup dxag_pp_setup;
	struct pp_interface *ut_dxag_pp;
	memset(&dxag_pp_setup, 0, sizeof(dxag_pp_setup));
	ut_dxag_pp = (struct pp_interface *)calloc(1, sizeof(*ut_dxag_pp));
	dxag_pp_setup.pool_size = 160;
	dxag_pp_setup.page_size = 4;
	dxag_pp_setup.allocator =  ut_dxag_al;
	dxag_pp_setup.set_id = ALLOCATOR_SET_SCG_PP;
	strcpy(dxag_pp_setup.pp_name, "dxag_ut_pp");
	pp_initialization(ut_dxag_pp, &dxag_pp_setup);

	struct pp2_setup pp2_setup;
	struct pp2_interface *pp2_p;
	memset(&pp2_setup, 0, sizeof(pp2_setup));
	pp2_p = (struct pp2_interface *)calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "dxag_ut_pp2";
	pp2_setup.allocator = ut_dxag_al;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_PP2;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 16;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 1;
	pp2_initialization(pp2_p, &pp2_setup);

	struct lstn_setup lstn_setup;
	struct lstn_interface *ut_dxag_lstn;
	memset(&lstn_setup, 0, sizeof(lstn_setup));
	ut_dxag_lstn = calloc(1, sizeof(*ut_dxag_lstn));
	lstn_setup.allocator = ut_dxag_al;
	lstn_setup.seg_size = 128;
	lstn_setup.set_id = 0;
	lstn_initialization(ut_dxag_lstn, &lstn_setup);

	// no member in umpk_setup, so not more code
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	memset(&umpk_setup, 0, sizeof(umpk_setup));
	umpk_p = calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	struct txn_setup txn_setup;
	struct txn_interface *txn_p;
	txn_setup.allocator = ut_dxag_al;
	txn_setup.seg_size = 128;
	txn_setup.set_id = ALLOCATOR_SET_CXP_TXN;
	txn_p = calloc(1, sizeof(*txn_p));
	txn_initialization(txn_p, &txn_setup);

	struct list_head conf_key_set;
	struct list_node *conf_key_node;
	struct ini_key_desc *conf_key;

	INIT_LIST_HEAD(&conf_key_set);
	server_generate_conf_tmplate(&conf_key_set, ut_dxag_lstn);
	list_for_each_entry(conf_key_node, struct list_node, &conf_key_set, ln_list) {
		conf_key = (struct ini_key_desc *)conf_key_node->ln_data;
		while (conf_key->k_name) {
			if (!conf_key->k_description) {
				nid_log_error("%s: key: %s has no description.", log_header, conf_key->k_name);
				return 1;
			}
			conf_key++;
		}
	}

	struct ini_setup ini_setup;
	struct ini_interface *dxag_ini;
	memset(&ini_setup, 0, sizeof(ini_setup));
	ini_setup.path = NID_CONF_SERVER;
	ini_setup.key_set = conf_key_set;
	dxag_ini = calloc(1, sizeof(*dxag_ini));
	ini_initialization(dxag_ini, &ini_setup);
	dxag_ini->i_op->i_parse(dxag_ini);

	struct ppg_setup ppg_setup;
	struct ppg_interface *ppg_p;
	ppg_p = calloc(1, sizeof(*ppg_p));
	ppg_setup.allocator = ut_dxag_al;
	ppg_setup.ini = dxag_ini;
	ppg_initialization(ppg_p, &ppg_setup);

	struct cdsg_setup cdsg_setup;
	struct cdsg_interface *cdsg_p;
	cdsg_p = calloc(1, sizeof(*cdsg_p));
	cdsg_setup.allocator = ut_dxag_al;
	cdsg_setup.ini = dxag_ini;
	cdsg_initialization(cdsg_p, &cdsg_setup);

	struct sdsg_setup sdsg_setup;
	struct sdsg_interface *sdsg_p;
	sdsg_p = calloc(1, sizeof(*sdsg_p));
	sdsg_setup.allocator = ut_dxag_al;
	sdsg_setup.ini = dxag_ini;
	sdsg_setup.ppg = ppg_p;
	sdsg_initialization(sdsg_p, &sdsg_setup);

	struct dxtg_setup dxtg_setup;
	struct dxtg_interface *ut_dxtg;
	memset(&dxtg_setup, 0, sizeof(dxtg_setup));
	dxtg_setup.allocator = ut_dxag_al;
	dxtg_setup.ini = dxag_ini;
	dxtg_setup.lstn = ut_dxag_lstn;
	dxtg_setup.pp2 = pp2_p;
	dxtg_setup.txn = txn_p;
	dxtg_setup.cdsg_p = cdsg_p;
	dxtg_setup.sdsg_p = sdsg_p;
	ut_dxtg = calloc(1, sizeof(*ut_dxtg));
	dxtg_initialization(ut_dxtg, &dxtg_setup);

	/*
	 * Create thread pool for guardian
	 */
	struct tpg_interface *tpg_p;
	struct tpg_setup tpg_setup;
	memset(&tpg_setup, 0, sizeof(tpg_setup));
	tpg_setup.ini = dxag_ini;
	tpg_setup.pp2 = pp2_p;
	tpg_p = calloc(1, sizeof(*tpg_p));
	tpg_initialization(tpg_p, &tpg_setup);

	setup->allocator = ut_dxag_al;
	setup->dxtg = ut_dxtg;
	setup->pp2 = pp2_p;
	setup->lstn = ut_dxag_lstn;
	setup->umpk = umpk_p;
	setup->ini = dxag_ini;
	setup->tpg = tpg_p;
	dxag_initialization(dxag_p, setup);

	return 0;
}

int
main()
{
	char *log_header = "ut1_dx";
	struct dxag_setup dxag_setup;
	struct dxag_interface ut_dxag;

	nid_log_open();
	nid_log_set_level(7);
	nid_log_warning("dxag ut module start ...");

	dxag_prepare(&ut_dxag, &dxag_setup);
	struct dxa_interface *dxa_p = ut_dxag.dxg_op->dxg_create_channel(&ut_dxag, "test_dxa");
	if(!dxa_p) {
		nid_log_error("%s: failed to create dxa channel", log_header);
		goto out;
	}
	int ret = dxa_p->dx_op->dx_start(dxa_p);
	if(ret) {
		nid_log_error("%s: failed to start dxa", log_header);
		goto cleanup;
	}

	nid_log_warning("%s: success start dxa, begin send out message test", log_header);

	struct dxt_interface *dxt_p = dxa_p->dx_op->dx_get_dxt(dxa_p);
	uint32_t num_msg = 0, dmsg_len, cmsg_len;
	char msg_buf[1024], *cbuf, *dbuf;
	struct umessage_tx_hdr cmsg;
	while(1) {
		sprintf(msg_buf, "dxa test message %d", num_msg);
		dmsg_len = strlen(msg_buf);
		dbuf = dxt_p->dx_op->dx_get_buffer(dxt_p, dmsg_len);
		if(!dbuf)
			goto wait_it;
		memcpy(dbuf, msg_buf, dmsg_len);
		cmsg_len = sizeof(cmsg);
		memset(&cmsg, 0, cmsg_len);
		cmsg.um_dlen = dmsg_len;
		cmsg.um_len = cmsg_len;
		cmsg.um_level = UMSG_LEVEL_APP;
		cbuf = dxt_p->dx_op->dx_get_buffer(dxt_p, cmsg_len);
		memcpy(cbuf, &cmsg, cmsg_len);
		dxt_p->dx_op->dx_send(dxt_p, cbuf, cmsg_len, dbuf, dmsg_len);
		nid_log_warning("send out %uth message", num_msg);
		num_msg++;

wait_it:
		sleep(1);
		// wait here for ever, and re-start nidserver, then check dxt recover action in syslog
		if(num_msg == 1000) {
			while(1) {
				sleep(100);
			}
		}
	}

cleanup:
	dxa_p->dx_op->dx_cleanup(dxa_p);

out:
	nid_log_warning("dxag ut module end...");
	nid_log_close();

	return 0;
}
