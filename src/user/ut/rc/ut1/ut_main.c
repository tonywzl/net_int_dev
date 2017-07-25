#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

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
#define UT_RC_TEST_N4K              1
#define UT_RC_START_OFFSET          (0)
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
//static struct rw_interface          ut_rc_rw;
//static void*                        ut_rc_rw_handle;

static size_t           ut_rc_data_size = UT_RC_TEST_DATA_SIZE;
static char            *ut_rc_data;
static struct list_head ut_rc_fp_head;
static off_t            ut_rc_offset = UT_RC_START_OFFSET;

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
    size_t i;

    ut_rc_data = malloc(ut_rc_data_size);

    for (i=0; i<ut_rc_data_size; i++) {
        ut_rc_data[i] = rand() % 256;
    }
    INIT_LIST_HEAD(&ut_rc_fp_head);
    ut_rc_fpc.fpc_op->fpc_bcalculate(&ut_rc_fpc, &ut_rc_fpn_data, ut_rc_data, ut_rc_data_size, 4096, &ut_rc_fp_head);
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

int
main()
{
    size_t i, len;
    int j, n4k;

	nid_log_open();
	printf("rc ut module start ...\n");

	printf("rc ut init ENV ...\n"); fflush(stdout);

	ut_rc_init_env();

	printf("rc ut init DATA ...\n"); fflush(stdout);
	ut_rc_gen_data();

	printf("\nStep 1: Do rc update ...\n"); fflush(stdout);
	ut_rc.rc_op->rc_update(&ut_rc, ut_rc_handle, ut_rc_data, ut_rc_data_size, ut_rc_offset, &ut_rc_fp_head);



	struct sub_request_node sreq, *newseq;
	struct list_head res_head;

	INIT_LIST_HEAD(&res_head);
	(void)newseq;
	(void)sreq;
	(void)n4k;
	(void)i;
	(void)j;

	len = 4096;

    printf("\nStep 2: Do rc search found test...\n"); fflush(stdout);

    sreq.sr_len = len;
    sreq.sr_buf = malloc(len);
    sreq.sr_offset = 0;
    ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &res_head);
    assert(list_empty(&res_head));
    free(sreq.sr_buf);

    INIT_LIST_HEAD(&res_head);
    sreq.sr_len = len;
    sreq.sr_buf = malloc(len);
    sreq.sr_offset = 12288;
    ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &res_head);
    assert(!list_empty(&res_head));
    free(sreq.sr_buf);

    INIT_LIST_HEAD(&res_head);
    sreq.sr_len = len;
    sreq.sr_buf = malloc(len);
    sreq.sr_offset = 1072627712;
    ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &res_head);
    assert(!list_empty(&res_head));
    free(sreq.sr_buf);

//	printf("\nStep 2: Do rc search found test...\n"); fflush(stdout);
//
//	for (j = 0; j < UT_RC_READ_REPEAT_TIMES; j++) {
//	    n4k = rand() % UT_RC_TEST_N4K;
//	    len = (size_t)rand() % (UT_RC_TEST_N4K - n4k - 1) << 12;
//	    INIT_LIST_HEAD(&res_head);
//	    sreq.sr_len = len;
//	    sreq.sr_buf = malloc(len);
//	    sreq.sr_offset = ut_rc_offset + (n4k << 12);
//        ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &res_head);
//        assert(list_empty(&res_head));
//        for (i = 0; i<len; i++) {
//            if (sreq.sr_buf[i] != ut_rc_data[sreq.sr_offset - ut_rc_offset + i]) {
//                printf("ERROR: DATA MIS-MATCH 1 %lu\n", i); fflush(stdout);
//                assert(0);
//            }
//        }
//        free(sreq.sr_buf);
//	}
//
//    printf("\nStep 3: Do rc search not found test right...\n"); fflush(stdout);
//
//    for (j = 0; j < UT_RC_READ_REPEAT_TIMES; j++) {
//        n4k = rand() % UT_RC_TEST_N4K;
//        len = ((UT_RC_TEST_N4K - n4k) + 1 + rand() % 10) << 12;
//        INIT_LIST_HEAD(&res_head);
//
//        sreq.sr_len = len;
//        sreq.sr_buf = malloc(len);
//        sreq.sr_offset = ut_rc_offset + (n4k << 12);
//
//        ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &res_head);
//        assert(!list_empty(&res_head));
//        list_for_each_entry(newseq, struct sub_request_node, &res_head, sr_list) {
//            printf("Original seq:: base offset:%ld len:%u offset:%ld Not_found_seq:: len:%u offset:%ld\n",
//                    ut_rc_offset, sreq.sr_len, sreq.sr_offset,
//                    newseq->sr_len, newseq->sr_offset);
//            fflush(stdout);
//        }
//
//        newseq = list_entry(res_head.next, struct sub_request_node, sr_list);
//        for (i = 0; i<len - newseq->sr_len; i++) {
//            if (sreq.sr_buf[i] != ut_rc_data[sreq.sr_offset - ut_rc_offset + i]) {
//                printf("ERROR: DATA MIS-MATCH 2 %lu\n", i); fflush(stdout);
//                assert(0);
//            }
//        }
//
//        free(sreq.sr_buf);
//    }
//
//    printf("\nStep 4: Do rc search not found test left...\n"); fflush(stdout);
//    for (j = 0; j < UT_RC_READ_REPEAT_TIMES; j++) {
//        n4k = rand() % UT_RC_TEST_N4K;
//        len = (UT_RC_TEST_N4K + rand() % 10) << 12;
//        INIT_LIST_HEAD(&res_head);
//
//        sreq.sr_len = len;
//
//        sreq.sr_offset = ut_rc_offset - (n4k << 12);
//        if (sreq.sr_offset < 0) {
//            printf("Random sreq.sr_offset < 0, try again.");
//            continue;
//        }
//        sreq.sr_buf = malloc(len);
//
//        ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &res_head);
//        assert(!list_empty(&res_head));
//        list_for_each_entry(newseq, struct sub_request_node, &res_head, sr_list) {
//            printf("Original seq::base offset:%ld len:%u offset:%ld Not_found_seq:: len:%u offset:%ld\n",
//                    ut_rc_offset, sreq.sr_len, sreq.sr_offset,
//                    newseq->sr_len, newseq->sr_offset);
//            fflush(stdout);
//        }
//
//        newseq = list_entry(res_head.next, struct sub_request_node, sr_list);
//        assert(newseq->sr_offset == sreq.sr_offset);
//        for (i = 0; i<len - newseq->sr_len; i++) {
//            if (sreq.sr_buf[i + newseq->sr_len] != ut_rc_data[i]) {
//                printf("ERROR: DATA MIS-MATCH 3 %lu\n", i); fflush(stdout);
//                assert(0);
//            }
//        }
//
//        free(sreq.sr_buf);
//    }

	printf("\nrc ut module end...\n"); fflush(stdout);
	nid_log_close();

	return 0;
}
