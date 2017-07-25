#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>

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

static char *           uts_rc_data[4];

static struct list_head uts_rc_fp_heads[4];
static size_t           uts_rc_data_size[] = {1, 2, 4, 1};
static off_t            uts_rc_data_offset[4] = {1, 4, 7, 15};

static char *           uts_rc_datas[16];
static char             uts_rc_data_map[] = {0, 1, 0, 0, 1, 1, 0, 1,1,1,1, 0,0,0,0, 1};

/**
 * DATA distributed
 * ## Empty
 * $$ Not Empty
 * * Gap
 **********************************************
 * 1##*2$$*3-4##*5-6$$*7##*8-11$$*12-15##*16$$*
 **********************************************
 */

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
    fpc_setup.fpc_algrm = FPC_SHA256;
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

    struct rc_channel_info cinfo;
    cinfo.rc_rw = NULL;
    cinfo.rc_rw_handle = NULL;
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

void
doRcTest1(pthread_t pid, int startblock, int lenblock, int *expect_found_indexs, int expect_found_cnt, int expect_not_found_cnt)
{
    int notfound_cnt;
    struct sub_request_node sreq, *newseq;
    struct list_head res_head;
    size_t len;
    off_t offset;
    int i,k;

    len = lenblock << 12;
    offset = (startblock-1) << 12;
    INIT_LIST_HEAD(&res_head);
    sreq.sr_len = len;
    sreq.sr_buf = malloc(len);
    sreq.sr_offset = offset;

    printf("\nStep %d: Thread ID: %lu Search rc from %d block to %d block, expect not found %d blocks, expect found %d blocks...\n", __sync_fetch_and_add(&nstep, 1), pid,
                   startblock, lenblock + startblock - 1,  expect_not_found_cnt, expect_found_cnt); fflush(stdout);

    ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &res_head);
    notfound_cnt = 0;
    list_for_each_entry(newseq, struct sub_request_node, &res_head, sr_list) {
        printf("Original seq:: offset:%ld len:%u Not_found_seq:: offset:%ld len:%u\n",
                sreq.sr_offset >> 12, sreq.sr_len >> 12,
                newseq->sr_offset >> 12, newseq->sr_len >> 12);
        fflush(stdout);
        notfound_cnt += newseq->sr_len >> 12;
    }
    assert(notfound_cnt == expect_not_found_cnt);
    printf("\nStep %d: Do found bit check ...\n", nstep++); fflush(stdout);
    char *buf, *buf1;
    for (i=0; i<expect_found_cnt; i++) {
        buf = sreq.sr_buf + ((expect_found_indexs[i] - startblock) << 12);
        buf1 = uts_rc_datas[expect_found_indexs[i] - 1];
        assert(uts_rc_data_map[expect_found_indexs[i] - 1]);
        for (k=0; k< 4096; k++) {
            assert(buf[k] == buf1[k]);
        }
    }

    free(sreq.sr_buf);
}

volatile int testCnt = 1;

static void *
doRcTestThread(void *p) {
    struct rc_test_args *arg = (struct rc_test_args*)p;
    doRcTest1(arg->pid, arg->startblock, arg->lenblock, arg->expect_found_indexs, arg->expect_found_cnt, arg->expect_not_found_cnt);
    __sync_sub_and_fetch(&testCnt, 1);

    int j;
    for (j=0; j<4; j++) {
        ut_rc.rc_op->rc_update(&ut_rc, ut_rc_handle, uts_rc_data[j], uts_rc_data_size[j] << 12, uts_rc_data_offset[j] << 12, &uts_rc_fp_heads[j]);
    }

    free(arg);
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
	    ut_rc.rc_op->rc_update(&ut_rc, ut_rc_handle, uts_rc_data[j], uts_rc_data_size[j] << 12, uts_rc_data_offset[j] << 12, &uts_rc_fp_heads[j]);
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
