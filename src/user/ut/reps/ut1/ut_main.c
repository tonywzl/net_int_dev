#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "reps_if.h"
#include "allocator_if.h"

#include "nid.h"
#include "dxa_if.h"
#include "dxag_if.h"
#include "dxtg_if.h"
#include "ini_if.h"
#include "lstn_if.h"
#include "pp_if.h"
#include "pp2_if.h"
#include "server.h"
#include "tp_if.h"
#include "umpk_if.h"
#include "sdsg_if.h"
#include "cdsg_if.h"
#include "txn_if.h"
#include "ppg_if.h"

#include "ms_intf.h"

MsRet
MS_Read_Fp_Async(const char *voluuid,
                       const char *snapuuid,
                       off_t voff,
                       size_t len,
                       Callback func,
                       void *arg,
                       void *fp,		// non-existing fp will cleared to 0
                       bool *fpFound, 	// output parm indicating fp found or not
                       IoFlag flag)
{
	char *log_header = "reps MS_Read_Fp_Async UT";
	nid_log_warning("%s: start ...", log_header);
	assert(voluuid);
	assert(snapuuid);
	assert(voff>=0);
	assert(len);
	assert(flag>=0);
	fpFound[0] = 1;
	fpFound[2] = 1;

	memcpy(fp, "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456", FP_SIZE*3);
	(*func)(0, arg);
	return retOk;
}

MsRet
MS_Read_Fp_Snapdiff_Async(const char *voluuid,
					const char *snapuuidPre,
					const char *snapuuidCur,
					off_t voff,
					size_t len,
					Callback func,
					void *arg,
					void *fp,
					bool *fpDiff)

{
	char *log_header = "reps MS_Read_Fp_Snapdiff_Async UT";
	nid_log_warning("%s: start ...", log_header);
	assert(voluuid);
	assert(snapuuidPre);
	assert(snapuuidCur);
	assert(voff>=0);
	assert(len);
	fpDiff[0] = 1;
	fpDiff[2] = 1;

	memcpy(fp, "987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210987654", FP_SIZE*3);
	(*func)(0, arg);
	return retOk;
}


MsRet MS_Read_Data_Async(struct iovec* vec,
                         size_t count,
                         void *fp,
                         // bool *fpToRead, enable until required by nid
                         Callback func,
                         void *arg,
                         IoFlag flag)
{
	char *log_header = "reps MS_Read_Data_Async UT";
	nid_log_warning("%s: start ...", log_header);

	assert(vec);
	assert(count);
	assert(fp);
	assert(flag>=0);
	memcpy(vec->iov_base, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij", 100);
	nid_log_warning("%s: run callback", log_header);
	(*func)(0, arg);
	nid_log_warning("%s: finish callback", log_header);
	return retOk;
}

#define  NID_CONF_SERVER "ut_reps_nidserver.conf"

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

	struct tp_setup tp_setup;
	struct tp_interface *ut_dxag_tp;
	memset(&tp_setup, 0, sizeof(tp_setup));
	ut_dxag_tp = (struct tp_interface *)calloc(1, sizeof(*ut_dxag_tp));
	tp_setup.pp2 = pp2_p;
	tp_setup.delay = 0;
	tp_setup.extend = 2;
	tp_setup.min_workers = 4;
	tp_setup.max_workers = 8;
	strcpy(tp_setup.name, "dxag_ut_tp");
	tp_initialization(ut_dxag_tp, &tp_setup);

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

	setup->allocator = ut_dxag_al;
	setup->dxtg = ut_dxtg;
	setup->pp2 = pp2_p;
	setup->lstn = ut_dxag_lstn;
	setup->tp = ut_dxag_tp;
	setup->umpk = umpk_p;
	setup->ini = dxag_ini;
	dxag_initialization(dxag_p, setup);

	return 0;
}



int
main()
{
	static struct reps_setup ut_reps_setup;
	static struct reps_interface ut_reps;

	nid_log_open();
	nid_log_info("reps ut module start ...");

	struct dxag_setup dxag_setup;
	struct dxag_interface ut_dxag;
	dxag_prepare(&ut_dxag, &dxag_setup);


	struct allocator_interface *allocator_p;
	struct allocator_setup allocator_setup;
	allocator_p = calloc(1, sizeof(*allocator_p));
	allocator_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(allocator_p, &allocator_setup);

	strcpy(ut_reps_setup.rs_name, "test_reps");
	strcpy(ut_reps_setup.rt_name, "test_rept");
	strcpy(ut_reps_setup.voluuid, "3f3a8671-baaa-3db8-a0ff-023717636945");
	strcpy(ut_reps_setup.snapuuid, "test_sp1_name");
	memset(ut_reps_setup.snapuuid2, 0, NID_MAX_UUID);
	strcpy(ut_reps_setup.dxa_name, "test_dxa");
	ut_reps_setup.vol_len = 1073741824;	//match the config file
	ut_reps_setup.allocator = allocator_p;
	ut_reps_setup.dxag_p = &ut_dxag;
	ut_reps_setup.umpk = dxag_setup.umpk;
	ut_reps_setup.tp = dxag_setup.tp;

	reps_initialization(&ut_reps, &ut_reps_setup);

	ut_reps.rs_op->rs_start(&ut_reps);

	nid_log_warning("running reps. ctrl c to quit.");
	printf("running reps. ctrl c to quit.\n");
	float progress = 0;
	while (progress < 1.0) {
		progress = ut_reps.rs_op->rs_get_progress(&ut_reps);
		printf("reps progress %2.1f\n", progress*100);
		sleep(1);
	}

	nid_log_info("reps ut module end...");
	nid_log_close();

	return 0;
}

