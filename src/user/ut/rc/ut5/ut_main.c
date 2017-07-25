#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>

#include "rc_wc_cbn.h"
#include "rc_wcn.h"
#include "nid_log.h"
#include "crc_if.h"
#include "rc_if.h"
#include "allocator_if.h"
#include "brn_if.h"
#include "fpn_if.h"
#include "fpc_if.h"
#include "pp_if.h"
#include "srn_if.h"
#include "rw_if.h"

#define UT_RC_READ_REPEAT_TIMES     6
#define UT_RC_TEST_N4K              1024
#define UT_RC_START_OFFSET          (1024 << 12)
#define UT_RC_TEST_DATA_SIZE        (UT_RC_TEST_N4K<<12)
#define UT_RC_READ_DEVICE           "/rc_device"

static struct rc_interface          ut_rc;
static void*                        ut_rc_handle;
static struct allocator_interface   ut_rc_al;
static struct brn_interface         ut_rc_brn;
static struct pp_interface          ut_rc_pp;
static struct srn_interface         ut_rc_srn;
static struct fpn_interface         ut_rc_fpn;
static struct fpn_interface         ut_rc_fpn_data;
static struct fpc_interface         ut_rc_fpc;
static struct rw_interface	    ut_rc_rw;
static void* 			    ut_rc_rw_handle;
struct rc_wcn_interface	    ut_rc_bwcn;
struct rc_wc_cbn_interface	    ut_rc_bwc_cbn;

static char *           uts_rc_data[4];

static struct list_head uts_rc_fp_heads[4];
static size_t           uts_rc_data_size[] = {1, 2, 4, 1};
static off_t            uts_rc_data_offset[4] = {1, 4, 7, 15};

static char *           uts_rc_datas[16];
static char             uts_rc_data_map[] = {0, 1, 0, 0, 1, 1, 0, 1,1,1,1, 0,0,0,0, 1};
volatile int 		testCnt = 1;

static void
check_read_device()
{
    struct stat fstat;
    char cmd[1024];
    int ret = stat(UT_RC_READ_DEVICE, &fstat);

    if (ret == -1) {
        if (errno == ENOENT ) {
            // File not exist, create one.
            sprintf(cmd, "touch %s", UT_RC_READ_DEVICE);
            system(cmd);
        } else {
            assert(0);
        }
    }
    printf("rc ut clean read buffer device: %s \n", UT_RC_READ_DEVICE);
    sprintf(cmd, "dd if=/dev/zero of=%s bs=1M count=64 oflag=sync",
            UT_RC_READ_DEVICE);
    system(cmd);
}

static void
ut_rc_gen_data ()
{
    size_t i, j;

    for (j = 0; j < 4; j++) {
        uts_rc_data[j] = malloc(uts_rc_data_size[j] << 12);
        for (i=0; i<(uts_rc_data_size[j]<<12); i++) {
            uts_rc_data[j][i] = rand() % 256;
        }
        INIT_LIST_HEAD(&uts_rc_fp_heads[j]);
        ut_rc_fpc.fpc_op->fpc_bcalculate(&ut_rc_fpc, &ut_rc_fpn_data, uts_rc_data[j], uts_rc_data_size[j]<<12, 4096, &uts_rc_fp_heads[j]);
    }

    int m = 0, n = 0;
    int currentdsz = uts_rc_data_size[m];

    for (j=0; j< 16; j++) {
        if (uts_rc_data_map[j]) {
            uts_rc_datas[j] = uts_rc_data[m] + (n << 12);
            n++;
            if (--currentdsz == 0) {
                m++;
                n=0;
                currentdsz = uts_rc_data_size[m];
            }
        } else {
            uts_rc_datas[j] = NULL;
        }
    }
}

static void
ut_rc_init_env ()
{
	check_read_device();

	struct allocator_setup ut_rc_al_setup;
	allocator_initialization(&ut_rc_al, &ut_rc_al_setup);

	struct fpc_setup fpc_setup;
	fpc_setup.fpc_algrm = FPC_SHA224;
	fpc_initialization(&ut_rc_fpc, &fpc_setup);


	struct fpn_setup fpn_setup;
	fpn_setup.allocator = &ut_rc_al;
	fpn_setup.fp_size = 32;
	fpn_setup.set_id = ALLOCATOR_SET_SCG_FPN;
	fpn_setup.seg_size = 4096;
	fpn_initialization(&ut_rc_fpn, &fpn_setup);
	fpn_initialization(&ut_rc_fpn_data, &fpn_setup);

	struct srn_setup srn_setup;
	srn_setup.allocator = &ut_rc_al;
	srn_setup.set_id = ALLOCATOR_SET_SCG_SRN;
	srn_setup.seg_size = 1024;
	srn_initialization(&ut_rc_srn, &srn_setup);

	struct pp_setup pp_setup;
	pp_setup.pool_size = 160;
	pp_setup.page_size = 4;
	pp_setup.allocator =  &ut_rc_al;
	pp_setup.set_id = ALLOCATOR_SET_SCG_PP;
	pp_setup.name = "RC_TEST_PP";
	pp_initialization(&ut_rc_pp, &pp_setup);

	struct brn_setup brn_setup;
	brn_setup.allocator = &ut_rc_al;
	brn_setup.set_id = ALLOCATOR_SET_SCG_BRN;
	brn_setup.seg_size = 4096;
	brn_initialization(&ut_rc_brn, &brn_setup);

	struct rc_setup rc_setup;
	rc_setup.allocator = &ut_rc_al;
	rc_setup.brn = &ut_rc_brn;
	rc_setup.cachedev = UT_RC_READ_DEVICE;
	rc_setup.cachedevsz = 4;
	rc_setup.fpn = &ut_rc_fpn;
	rc_setup.pp = &ut_rc_pp;
	rc_setup.srn = &ut_rc_srn;
	rc_setup.type = RC_TYPE_CAS;
	rc_setup.uuid = "123456";

	rc_initialization(&ut_rc, &rc_setup);

	struct rw_setup rw_setup;
	rw_setup.allocator = &ut_rc_al;
	rw_setup.fpn_p = &ut_rc_fpn;
	rw_setup.type = RW_TYPE_MSERVER;
	rw_setup.name = "test";
	rw_initialization(&ut_rc_rw, &rw_setup);
	ut_rc_rw_handle = ut_rc_rw.rw_op->rw_create_worker(&ut_rc_rw, "0",
			1, 1, 4096);

	struct rc_wcn_setup rmsetup;
	rmsetup.allocator = &ut_rc_al;
	rmsetup.seg_size = 16;
	rmsetup.set_id = ALLOCATOR_SET_BWC_RCBWC;
	rc_wcn_initialization(&ut_rc_bwcn, &rmsetup);

	struct rc_wc_cbn_setup rmc_setup;
	rmc_setup.allocator = &ut_rc_al;
	rmc_setup.seg_size = 128;
	rmc_setup.set_id = ALLOCATOR_SET_BWC_RCBWC_CB;
	rc_wc_cbn_initialization(&ut_rc_bwc_cbn, &rmc_setup);

	struct rc_channel_info cinfo;
	cinfo.rc_rw = &ut_rc_rw;
	cinfo.rc_rw_handle = ut_rc_rw_handle;

	int new_rc;
	ut_rc_handle = ut_rc.rc_op->rc_create_channel(&ut_rc, NULL, &cinfo, "/dev/sdb1", &new_rc);

}

volatile int nstep = 1;

struct rc_test_args {
    pthread_t pid;
    int startblock;
    int lenblock;
    int *expect_found_indexs;
    int expect_found_cnt;
    int expect_not_found_cnt;
};

static void
__bwc_read_callback_twosteps(int errcode, struct rc_wc_cb_node *arg)
{
	struct sub_request_node *sreq_p = arg->rm_sreq_p;
	struct rc_wc_cbn_interface *p_rc_bwc_cbn = &ut_rc_bwc_cbn;
	struct rc_wc_node *rc_bwc_node = arg->rm_wc_node;
	struct rc_wcn_interface *p_rc_bwcn = &ut_rc_bwcn;
	int cnt;
	struct rc_test_args *tsarg = (struct rc_test_args*)arg->rm_rn_p;
	struct srn_interface *srn_p = &ut_rc_srn;

	printf("\nStep %d: Do found bit check ...\n", nstep++); fflush(stdout);
	char *buf, *buf1;
	int i;
	for (i=0; i<tsarg->expect_found_cnt; i++) {
		buf = sreq_p->sr_buf + ((tsarg->expect_found_indexs[i] - tsarg->startblock) << 12);
		buf1 = uts_rc_datas[tsarg->expect_found_indexs[i] - 1];
		assert(uts_rc_data_map[tsarg->expect_found_indexs[i] - 1]);
		assert(memcmp(buf, buf1, 4096) == 0);
	}
	free(sreq_p->sr_buf);

	/* Read error process. */
	if (errcode) {
		printf("ERROR: %d", errcode);
	}
	/* Free sub request */
	srn_p->sr_op->sr_put_node(srn_p, sreq_p);
	/* Free rc_bwc callback node*/
	p_rc_bwc_cbn->rm_op->rm_put_node(p_rc_bwc_cbn, arg);
	cnt = __sync_sub_and_fetch(&rc_bwc_node->rm_sub_req_counter, 1);
	if (cnt == 0) {
		/* If all sub request comes back.
		 * Send response and free rc_bwc_node.*/
		p_rc_bwcn->rm_op->rm_put_node(p_rc_bwcn, rc_bwc_node);
		__sync_sub_and_fetch(&testCnt, 1);
		free(tsarg);
	}
}


void
doRcTest1(pthread_t pid, struct rc_test_args *tsarg)
{
	struct sub_request_node *sreq_p;
	struct list_head res_head;
	size_t len;
	off_t offset;
	struct rc_wc_cb_node *arg;
	struct rc_wc_node *rc_bwc_node;
	struct rc_wcn_interface *p_rc_bwcn = &ut_rc_bwcn;
	struct rc_wc_cbn_interface *p_rc_bwc_cbn = &ut_rc_bwc_cbn;
	struct srn_interface *srn_p = &ut_rc_srn;

	len = tsarg->lenblock << 12;
	offset = (tsarg->startblock-1) << 12;
	INIT_LIST_HEAD(&res_head);
	sreq_p = srn_p->sr_op->sr_get_node(srn_p);
	sreq_p->sr_len = len;
	sreq_p->sr_buf = malloc(len);
	sreq_p->sr_offset = offset;

	printf("\nStep %d: Thread ID: %lu Search rc from %d block to %d block, expect not found %d blocks, expect found %d blocks...\n", __sync_fetch_and_add(&nstep, 1), pid,
			tsarg->startblock, tsarg->lenblock + tsarg->startblock - 1,  tsarg->expect_not_found_cnt, tsarg->expect_found_cnt); fflush(stdout);
	rc_bwc_node = p_rc_bwcn->rm_op->rm_get_node(p_rc_bwcn);
	rc_bwc_node->rm_sub_req_counter = 1;
	arg = p_rc_bwc_cbn->rm_op->rm_get_node(p_rc_bwc_cbn);
	/* Add sub request counter */
	__sync_add_and_fetch(&rc_bwc_node->rm_sub_req_counter, 1);
	arg->rm_wc_node = rc_bwc_node;
	arg->rm_rn_p = (struct request_node*)tsarg;
	arg->rm_sreq_p = sreq_p;
	//struct rc_interface *, void *, struct sub_request_node *, rc_callback, struct rc_bwc_cb_node*
	ut_rc.rc_op->rc_search_fetch(&ut_rc, ut_rc_handle, sreq_p, __bwc_read_callback_twosteps, arg);

	if (__sync_sub_and_fetch(&rc_bwc_node->rm_sub_req_counter, 1) == 0) {
		/* Free sub request */
		p_rc_bwcn->rm_op->rm_put_node(p_rc_bwcn, rc_bwc_node);
		__sync_sub_and_fetch(&testCnt, 1);
		free(tsarg);
	}
}

static void *
doRcTestThread(void *p) {
    struct rc_test_args *arg = (struct rc_test_args*)p;
    doRcTest1(arg->pid, arg);
    return NULL;
}

void
doRcTest(int startblock, int lenblock, int *expect_found_indexs, int expect_found_cnt, int expect_not_found_cnt) {
    struct rc_test_args *arg = (struct rc_test_args*) malloc(sizeof(*arg));
    arg->startblock = startblock,
    arg->lenblock = lenblock,
    arg->expect_found_indexs = expect_found_indexs,
    arg->expect_found_cnt = expect_found_cnt,
    arg->expect_not_found_cnt = expect_not_found_cnt;


    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    __sync_add_and_fetch(&testCnt, 1);
    pthread_create(&arg->pid, &attr, doRcTestThread, arg);

}

struct rw_callback_arg arg;

int
main()
{
	int j;

	nid_log_open();
	printf("rc ut module start ...\n");

	printf("rc ut init ENV ...\n"); fflush(stdout);

	ut_rc_init_env();

	printf("rc ut init DATA ...\n"); fflush(stdout);
	ut_rc_gen_data();

	printf("\nStep %d: Do rc update ...\n", nstep++); fflush(stdout);
	for (j=0; j<4; j++) {
		ut_rc_rw.rw_op->rw_pwrite_async_fp(
				&ut_rc_rw, ut_rc_rw_handle, uts_rc_data[j], uts_rc_data_size[j] << 12, uts_rc_data_offset[j] << 12, NULL, &arg);
	}

	printf("\nWait for 5 seconds ...\n"); fflush(stdout);
	sleep(5);

	int expect_found_indexs0[] = {2};
	doRcTest(1, 2, expect_found_indexs0, 1, 1);

	int expect_found_indexs1[] = {2, 5, 6};
	doRcTest(1, 7, expect_found_indexs1, 3, 4);

	int expect_found_indexs2[] = {2, 5, 6, 8, 9, 10, 11, 16};
	doRcTest(1, 16, expect_found_indexs2, 8, 8);

	int expect_found_indexs3[] = {2, 5, 6, 8, 9, 10, 11, 16};
	doRcTest(2, 15, expect_found_indexs3, 8, 7);

	int expect_found_indexs4[] = {2, 5};
	doRcTest(2, 4, expect_found_indexs4, 2, 2);

	int expect_found_indexs5[] = {2, 5, 6};
	doRcTest(2, 5, expect_found_indexs5, 3, 2);

	int expect_found_indexs6[] = {2, 5, 6, 8};
	doRcTest(2, 7, expect_found_indexs6, 4, 3);

	int expect_found_indexs7[] = {2, 5, 6, 8, 9};
	doRcTest(2, 8, expect_found_indexs7, 5, 3);

	int expect_found_indexs8[] = {2, 5, 6, 8, 9, 10};
	doRcTest(2, 9, expect_found_indexs8, 6, 3);

	int expect_found_indexs9[] = {2, 5, 6, 8, 9, 10, 11};
	doRcTest(2, 11, expect_found_indexs9, 7, 4);

	int expect_found_indexs10[] = {2, 5, 6, 8, 9, 10, 11};
	doRcTest(2, 12, expect_found_indexs10, 7, 5);

	int expect_found_indexs11[] = {2, 5, 6, 8, 9, 10, 11};
	doRcTest(2, 13, expect_found_indexs11, 7, 6);

	int expect_found_indexs12[] = {};
	doRcTest(3, 1, expect_found_indexs12, 0, 1);

	int expect_found_indexs13[] = {};
	doRcTest(3, 2, expect_found_indexs13, 0, 2);

	int expect_found_indexs14[] = {5};
	doRcTest(3, 3, expect_found_indexs14, 1, 2);

	int expect_found_indexs15[] = {5, 6};
	doRcTest(3, 4, expect_found_indexs15, 2, 2);

	int expect_found_indexs16[] = {5, 6};
	doRcTest(3, 5, expect_found_indexs16, 2, 3);

	int expect_found_indexs17[] = {5, 6, 8};
	doRcTest(3, 6, expect_found_indexs17, 3, 3);

	__sync_sub_and_fetch(&testCnt, 1);
	while (testCnt) {
		sleep(1);
	}
	printf("\nrc ut module end...\n"); fflush(stdout);
	nid_log_close();

	return 0;
}
