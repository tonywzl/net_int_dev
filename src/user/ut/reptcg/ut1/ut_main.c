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
#include "tpg_if.h"
#include "umpk_if.h"
#include "sdsg_if.h"
#include "cdsg_if.h"
#include "txn_if.h"
#include "ppg_if.h"

#include "list.h"
#include "reptc_if.h"
#include "reptg_if.h"
#include "reptcg_if.h"

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


MsRet
MS_Read_Data_Async(struct iovec* vec,
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

MsRet
MS_Write_Fp_Async(const char *voluuid,
                        off_t voff,
                        size_t len,
                        Callback func,
                        void *arg,
                        void *fp,
                        bool *fpToWrite,
                        bool *fpMiss)
{
	char *log_header = "rept MS_Write_Fp_Async_UT";
	nid_log_warning("%s: start ...", log_header);
	assert(voluuid);
	assert(voff>=0);
	assert(len);
	assert(fp);
	assert(fpToWrite);
	fpMiss[2] = 1;
	(*func)(0, arg);
	return retOk;
}

MsRet
MS_Write_Data_Async(const char *voluuid,
                          struct iovec *iovec,
                          size_t count,
                          off_t voff,
                          size_t len,
                          Callback func,
                          void *arg,
                          void *fp,
                          bool *fpToWrite)
{
	assert(voluuid);
	assert(iovec);
	assert(count);
	assert(voff>=0);
	assert(len);
	assert(fp);
	assert(fpToWrite);
	(*func)(0, arg);
	return retOk;
}

#define  NID_CONF_SERVER "ut_reptcg_nidserver.conf"

int
dxpg_prepare(struct dxpg_interface *dxpg_p, struct dxpg_setup *setup)
{
	assert(dxpg_p && setup);
	return 0;
}

int
main()
{
	struct reptcg_setup ut_reptcg_setup;
	struct reptcg_interface ut_reptcg;

	nid_log_open();
	nid_log_info("reptcg ut module start ...");

	//init dxpg
	struct dxpg_setup ut_dxpg_setup;
	struct dxpg_interface ut_dxpg;
	dxpg_prepare(&ut_dxpg, &ut_dxpg_setup);

	//init allocator
	struct allocator_interface *allocator_p;
	struct allocator_setup allocator_setup;
	allocator_p = calloc(1, sizeof(*allocator_p));
	allocator_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(allocator_p, &allocator_setup);

	//init pp2
	struct pp2_setup pp2_setup;
	struct pp2_interface *pp2_p;
	pp2_p = calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "ut_reptcg";
	pp2_setup.allocator = allocator_p;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_REP;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 4;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 0;
	pp2_initialization(pp2_p, &pp2_setup);

	//init tpg
	struct tpg_interface ut_tpg;
	struct tpg_setup ut_tpg_setup;
	memset(&ut_tpg_setup, 0, sizeof(ut_tpg_setup));
	ut_tpg_setup.ini = ut_dxpg_setup.ini;
	ut_tpg_setup.pp2 = pp2_p;
	tpg_initialization(&ut_tpg, &ut_tpg_setup);

	//init reptg
	struct reptg_interface ut_reptg;
	struct reptg_setup ut_reptg_setup;
	ut_reptg_setup.ini = ut_dxpg_setup.ini;
	ut_reptg_setup.tpg = &ut_tpg;
	ut_reptg_setup.allocator = allocator_p;
	ut_reptg_setup.dxpg = &ut_dxpg;
	reptg_initialization(&ut_reptg, &ut_reptg_setup);

	//init reptcg
	ut_reptcg_setup.reptg= &ut_reptg;
	ut_reptcg_setup.umpk = ut_dxpg_setup.umpk;
	reptcg_initialization(&ut_reptcg, &ut_reptcg_setup);

	while (1) {
		nid_log_warning("running reptcg. ctrl c to quit.");
		sleep(1);
	}

	nid_log_info("reptcg ut module end...");
	nid_log_close();

	return 0;
}
