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

#define UT_RC_READ_DEVICE           "/rc_device"

// Not continue blocks number
#define UT_RC_NNOTCON_BLOCK         64
// Block number of continue
#define UT_RC_NRANDOM_DATA_MIN      512
#define UT_RC_NRANDOM_DATA_MAX      1024
// Block gap of continue blocks
#define UT_RC_RANDOM_GAP_MIN        0
#define UT_RC_RANDOM_GAP_MAX        1024

static struct rc_interface          ut_rc;
static void*                        ut_rc_handle;
static struct allocator_interface   ut_rc_al;
static struct brn_interface         ut_rc_brn;
static struct pp_interface          ut_rc_pp;
static struct srn_interface         ut_rc_srn;
static struct fpn_interface         ut_rc_fpn;
static struct fpn_interface         ut_rc_fpn_data;
static struct fpc_interface         ut_rc_fpc;

static char *           uts_rc_data[UT_RC_NNOTCON_BLOCK];
static struct list_head uts_rc_fp_heads[UT_RC_NNOTCON_BLOCK];
static size_t           uts_rc_data_size[UT_RC_NNOTCON_BLOCK];
static off_t            uts_rc_data_offset[UT_RC_NNOTCON_BLOCK];
static char             **uts_rc_datas;
static char             *uts_rc_data_map;
static size_t           uts_rc_total_block_num;

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
    int offset, kcb, kgb;
    kcb = UT_RC_NRANDOM_DATA_MAX - UT_RC_NRANDOM_DATA_MIN ;
    kgb = UT_RC_RANDOM_GAP_MAX - UT_RC_RANDOM_GAP_MIN;
    // Generate size
    for (j=0; j<UT_RC_NNOTCON_BLOCK; j++) {

        if (kcb == 0) {
            uts_rc_data_size[j] = UT_RC_NRANDOM_DATA_MIN;
        } else {
            uts_rc_data_size[j] = UT_RC_NRANDOM_DATA_MIN + rand() % kcb;
        }
    }

    // Generate random offset
    for (j=0; j<UT_RC_NNOTCON_BLOCK; j++) {
        if (j == 0) {
            kgb = UT_RC_RANDOM_GAP_MAX - UT_RC_RANDOM_GAP_MIN;
            if (kgb == 0) {
                uts_rc_data_offset[j] = UT_RC_RANDOM_GAP_MIN;
            } else {
                uts_rc_data_offset[j] = UT_RC_RANDOM_GAP_MIN + rand() % kgb;
            }
        } else {
            if (kgb == 0) {
                uts_rc_data_offset[j] = uts_rc_data_offset[j-1] + uts_rc_data_size[j-1] + UT_RC_RANDOM_GAP_MIN;
            } else {
                uts_rc_data_offset[j] = uts_rc_data_offset[j-1] + uts_rc_data_size[j-1] + UT_RC_RANDOM_GAP_MIN + rand() % kgb;
            }
        }
    }
    uts_rc_total_block_num = uts_rc_data_offset[j-1] + uts_rc_data_size[j-1];

    // Generate random data
    for (j = 0; j < UT_RC_NNOTCON_BLOCK; j++) {
        uts_rc_data[j] = malloc(uts_rc_data_size[j] << 12);
        for (i=0; i<(uts_rc_data_size[j]<<12); i++) {
            uts_rc_data[j][i] = rand() % 256;
        }
        INIT_LIST_HEAD(&uts_rc_fp_heads[j]);
        ut_rc_fpc.fpc_op->fpc_bcalculate(&ut_rc_fpc, &ut_rc_fpn_data, uts_rc_data[j], uts_rc_data_size[j]<<12, 4096, &uts_rc_fp_heads[j]);
    }

    // Generate rc_data_map
    uts_rc_data_map = calloc(uts_rc_total_block_num, 1);

    for (j=0; j<UT_RC_NNOTCON_BLOCK; j++) {
        offset = uts_rc_data_offset[j];
        for (i=0; i< uts_rc_data_size[j]; i++) {
            uts_rc_data_map[offset+i] = (char)1;
        }
    }

    // Generate uts_rc_datas for easy compare
    int m = 0, n = 0;
    int currentdsz = uts_rc_data_size[m];
    uts_rc_datas = malloc(sizeof(*uts_rc_datas) * uts_rc_total_block_num);

    for (j=0; j< uts_rc_total_block_num; j++) {
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
    fpn_setup.seg_size = 8192; //UT_RC_NNOTCON_BLOCK * UT_RC_NRANDOM_DATA_MAX ;
    fpn_initialization(&ut_rc_fpn, &fpn_setup);
    fpn_initialization(&ut_rc_fpn_data, &fpn_setup);

    struct srn_setup srn_setup;
    srn_setup.allocator = &ut_rc_al;
    srn_setup.set_id = ALLOCATOR_SET_SCG_SRN;
    srn_setup.seg_size = 1024;// UT_RC_NNOTCON_BLOCK * UT_RC_NRANDOM_DATA_MAX * 100;
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
};

void
generate_expect_result(int startblock, int lenblock,
        int **expect_found_indexs, int *expect_found_cnt, int *expect_not_found_cnt)
{
    int i, j;
    int *efi;
    *expect_found_cnt = 0;
    *expect_not_found_cnt = 0;
    for (i=0; i< lenblock; i++) {
        if ((startblock-1 + i) >= (int)uts_rc_total_block_num) {
            (*expect_not_found_cnt)++;
        } else {
            if (uts_rc_data_map[startblock-1 + i]) {
                (*expect_found_cnt)++;
            } else {
                (*expect_not_found_cnt)++;
            }
        }
    }

    efi = malloc(sizeof(int) * (*expect_found_cnt));
    *expect_found_indexs = efi;
    j = 0;
    for (i=0; i< lenblock; i++) {
        if ((startblock-1 + i) >= (int)uts_rc_total_block_num) {
            break;
        } else {
            if (i >= (int)uts_rc_total_block_num) break;
            if (uts_rc_data_map[startblock-1 + i]) {
                efi[j++] = startblock + i;
                fflush(stdout);
            }
        }
    }
}

void
doRcTest1(pthread_t pid, int startblock, int lenblock)
{
    int notfound_cnt;
    struct sub_request_node sreq, *newseq, *newseq_nx;
    struct list_head sreq_head;
    size_t len;
    off_t offset;
    int i,k;

    int *expect_found_indexs;
    int expect_found_cnt;
    int expect_not_found_cnt;

    len = lenblock << 12;
    offset = (startblock-1) << 12;
    INIT_LIST_HEAD(&sreq_head);
    sreq.sr_len = len;
    sreq.sr_buf = malloc(len);
    sreq.sr_offset = offset;

    generate_expect_result(startblock, lenblock,
            &expect_found_indexs, &expect_found_cnt, &expect_not_found_cnt);

    printf("\nStep %d: Thread ID: %lu Search rc from %d block to %d block, expect not found %d blocks, expect found %d blocks, expect found index (", __sync_fetch_and_add(&nstep, 1), pid,
                   startblock, lenblock + startblock - 1,  expect_not_found_cnt, expect_found_cnt);
    for (i=0; i<expect_found_cnt; i++) {
        printf("%d,", expect_found_indexs[i]);
    }
    printf(")\n");
    fflush(stdout);

    ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &sreq_head);
    notfound_cnt = 0;
    list_for_each_entry_safe(newseq, newseq_nx, struct sub_request_node, &sreq_head, sr_list) {
        printf("Original seq:: offset:%ld len:%u Not_found_seq:: offset:%ld len:%u\n",
                sreq.sr_offset >> 12, sreq.sr_len >> 12,
                newseq->sr_offset >> 12, newseq->sr_len >> 12);
        fflush(stdout);
        notfound_cnt += newseq->sr_len >> 12;
        ut_rc_srn.sr_op->sr_put_node(&ut_rc_srn, newseq);
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
    doRcTest1(arg->pid, arg->startblock, arg->lenblock);
    __sync_sub_and_fetch(&testCnt, 1);
    free(arg);
    return NULL;
}

void
doRcTest(int startblock, int lenblock) {
    struct rc_test_args *arg = (struct rc_test_args*) malloc(sizeof(*arg));
    arg->startblock = startblock,
    arg->lenblock = lenblock;

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
	for (j=0; j<UT_RC_NNOTCON_BLOCK; j++) {
	    ut_rc.rc_op->rc_update(&ut_rc, ut_rc_handle, uts_rc_data[j], uts_rc_data_size[j] << 12, uts_rc_data_offset[j] << 12, &uts_rc_fp_heads[j]);
	}

	printf("\nWait for 5 seconds ...\n"); fflush(stdout);
	sleep(5);

	int maxblock = UT_RC_NNOTCON_BLOCK * UT_RC_NRANDOM_DATA_MAX + UT_RC_NNOTCON_BLOCK * UT_RC_RANDOM_GAP_MAX;

	doRcTest(1, maxblock);

    doRcTest(1, maxblock/2);

    doRcTest(maxblock/2 + 1, maxblock);

    doRcTest(1, maxblock + 100);

    doRcTest(maxblock /3, maxblock / 3 * 2);

    __sync_sub_and_fetch(&testCnt, 1);
    while (testCnt) {
        sleep(1);
    }

	printf("\nrc ut module end...\n"); fflush(stdout);
	nid_log_close();

	return 0;
}
