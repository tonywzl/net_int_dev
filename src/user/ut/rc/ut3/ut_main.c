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
#include "nid_shared.h"
#include "cdn_if.h"
#include "cse_if.h"
#include "dsmgr_if.h"
#include "dsrec_if.h"
#include "blksn_if.h"
#include "nse_crc_cbn.h"
#include "cse_crc_cbn.h"

#define CRC_BLOCK_SHIFT     12
#define CRC_BLOCK_SIZE      (1 << CRC_BLOCK_SHIFT)
#define CRC_BLOCK_MASK      (CRC_BLOCK_SIZE - 1)
#define CRC_SUPER_BLOCK_SIZE    (1UL << 20)     // 1M, must be multiple of 64 blocks
#define CRC_MBLOCK_SIZE     32          // bytes
#define CRC_MAX_MBLOCK_FLUSH    4096

#define CRC_SUPER_HEADRE_SIZE   64
struct crc_super_header {
    uint64_t    s_blocksz;  // size of block in bytes, generally speaking, 4096
    uint64_t    s_headersz; // number of blocks for the cache header
    uint64_t    s_bmpsz;    // number of blocks for the bitmap
    uint64_t    s_metasz;   // number of blocks for all meta data
    uint64_t    s_size;     // number of data blocks
};

struct crc_private_clone {
	char				p_uuid[NID_MAX_UUID];
	struct allocator_interface	*p_allocator;
	struct cdn_interface		p_cdn;
	struct brn_interface		*p_brn;
	struct rc_interface		*p_rc;
	pthread_mutex_t			p_lck;
	struct list_head		p_chan_head;
	struct list_head		p_inactive_head;
	struct srn_interface		*p_srn;
	struct pp_interface		*p_pp;
	struct cse_interface		p_cse;
	struct fpn_interface		*p_fpn;
	struct fpc_interface	 	p_fpc;
	struct blksn_interface		p_blksn;
	struct dsmgr_interface		p_dsmgr;
	struct dsrec_interface		p_dsrec;
	struct crc_super_header		p_super;
	struct nse_crc_cbn_interface 	p_ncc;
	struct cse_crc_cbn_interface 	p_ccc;
	uint64_t			*p_bitmap;
	struct crc_meta_block		*p_meta_data;
	char				*p_fp_buf;
	char				p_cachedev[NID_MAX_PATH];
	int				p_fd;
	uint32_t			p_blocksz;	// likely be 4K
	uint32_t			p_block_shift;	// LINKELY 12
	uint32_t			p_supersz;	// in blocks, likely 1M/4K = 256
	uint64_t			p_total_size;	// total number of blocks, including super/bitmap/meta/data
	uint64_t			p_bmpsz;	// bitmap size in number of blocks, must be multiple of 64 blocks
	uint64_t			p_metasz;
	uint64_t			p_size;		// number of data blocks, must be multiple of 64 blocks
	uint8_t				p_stop;
};

struct rc_private_clone {
	char				p_name[NID_MAX_UUID];
	int				p_type;
	void				*p_real_rc;
	struct allocator_interface	*p_allocator;
	struct fpn_interface		*p_fpn;
	struct brn_interface		*p_brn;
};

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
static struct rc_private_clone      *ut_rc_private;
static struct crc_interface         *ut_rc_crc;
static struct crc_private_clone     *ut_rc_crc_private;
static struct cse_interface         *ut_rc_cse;

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

    ut_rc_private = (struct rc_private_clone*)ut_rc.rc_private;
    ut_rc_crc = (struct crc_interface *) ut_rc_private->p_real_rc;
    ut_rc_crc_private = (struct crc_private_clone *) ut_rc_crc->cr_private;

    ut_rc_cse = (struct cse_interface*) &ut_rc_crc_private->p_cse;
}

#define UT_RC_IOV_NUMBER 10
#define UT_RC_IOV_BASE_LEN (1<<22)
#define UT_NTHREADS 8

static struct iovec ut_rc_iov[UT_RC_IOV_NUMBER];
static struct list_head ut_rc_iov_fp_head;

static volatile int ut_rc_nthread=UT_NTHREADS;

struct list_head ut_rc_sreq_head;

static void
ut_rc_init_data() {
    int i, j;

    for (i=0; i<UT_RC_IOV_NUMBER; i++) {
        ut_rc_iov[i].iov_base = malloc(UT_RC_IOV_BASE_LEN);
        for (j=0; j<UT_RC_IOV_BASE_LEN; j++) {
            *(char*)(ut_rc_iov[i].iov_base + j) = (char)rand();
        }
        ut_rc_iov[i].iov_len = UT_RC_IOV_BASE_LEN;
    }
    ut_rc_fpc.fpc_op->fpc_bcalculatev(&ut_rc_fpc, &ut_rc_fpn, ut_rc_iov, UT_RC_IOV_NUMBER, 4096, &ut_rc_iov_fp_head);

    uint32_t len;
    off_t off = 0;
    struct sub_request_node *rn_p;
    INIT_LIST_HEAD(&ut_rc_sreq_head);
    for (i=0; i<UT_RC_IOV_NUMBER; i++) {
        len = UT_RC_IOV_BASE_LEN;
        while (len) {
            rn_p = malloc(sizeof(*rn_p));
            rn_p->sr_buf = malloc(4096);
            assert(rn_p->sr_buf);
            rn_p->sr_len = 4096;
            rn_p->sr_offset = off;
            list_add_tail(&rn_p->sr_list, &ut_rc_sreq_head);
            off += 4096;
            len -= 4096;
        }
    }

}

void*
do_cse_test(void* p) {
    pthread_t *pid = p;
    int i;
    off_t off = 0;
    struct sub_request_node *rn_p;
    struct list_head ut_rc_not_found_sreq_head, ut_rc_not_found_fp_head;
    int not_found_num;

    INIT_LIST_HEAD(&ut_rc_not_found_sreq_head);
    INIT_LIST_HEAD(&ut_rc_not_found_fp_head);


    ut_rc_cse->cs_op->cs_updatev(ut_rc_cse, ut_rc_iov, UT_RC_IOV_NUMBER, &ut_rc_iov_fp_head);

    ut_rc_cse->cs_op->cs_search_list(ut_rc_cse, &ut_rc_iov_fp_head, &ut_rc_sreq_head, &ut_rc_not_found_sreq_head, &ut_rc_not_found_fp_head, &not_found_num);
    assert(list_empty(&ut_rc_not_found_sreq_head));


    size_t iov_len;
    i = 0;
    off = 0;
    off_t iov_off = 0;
    char* buf = ut_rc_iov[i].iov_base;
    iov_len = ut_rc_iov[i].iov_len;

    list_for_each_entry(rn_p, struct sub_request_node, &ut_rc_sreq_head, sr_list) {
        assert(rn_p->sr_len == 4096);
        assert(i < UT_RC_IOV_NUMBER);
        if (memcmp(rn_p->sr_buf, ut_rc_iov[i].iov_base + iov_off, 4096) != 0) {
            printf("\n %lu Block %ld content not match.",*pid, off >> 12);
            assert(0);
        }

        iov_off += 4096;
        off += 4096;
        buf += 4096;
        iov_len -= 4096;

        printf("%lu Block %ld content matched.\n", *pid, off >> 12);

        if (iov_len == 0) {
            i++;
            buf = ut_rc_iov[i].iov_base;
            iov_len = ut_rc_iov[i].iov_len;
            iov_off = 0;
        }
    }

    while (list_empty(&ut_rc_sreq_head)) {
        rn_p = list_first_entry(&ut_rc_sreq_head, struct sub_request_node, sr_list);
        free(rn_p->sr_buf);
       free(rn_p);
    }

    __sync_fetch_and_sub(&ut_rc_nthread, 1);

    return NULL;
}

int
main()
{
	nid_log_open();
	printf("rc ut3 module start ...\n");

	printf("rc ut init ENV ...\n"); fflush(stdout);
	ut_rc_init_env();

	printf("rc ut init Data ...\n"); fflush(stdout);
	ut_rc_init_data();

//	int times = 10;
//	pthread_t i;
//	for (i=0; i<times; i++) {
//	    do_cse_test(&i);
//	}

	pthread_t pid[UT_NTHREADS];
	int i;
    for (i=0; i<UT_NTHREADS; i++) {
        pthread_create(&pid[i], NULL, do_cse_test, &pid[i]);
    }
    for  (i=0; i<UT_NTHREADS; i++) {
        pthread_join(pid[i], NULL);
    }

	//	pthread_t pid1, pid2;
//	pthread_create(&pid1, NULL, do_cse_test, &pid1);
//	pthread_create(&pid2, NULL, do_cse_test1, &pid2);
//
//	pthread_join(pid1, NULL);
//	pthread_join(pid2, NULL);
    while (ut_rc_nthread) {
        sleep(1);
    }

	printf("\nrc ut3 module end...\n"); fflush(stdout);
	nid_log_close();

	return 0;
}
