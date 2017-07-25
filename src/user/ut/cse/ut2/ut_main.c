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
#include "list.h"

struct crc_super_header {
	uint64_t s_blocksz;  // size of block in bytes, generally speaking, 4096
	uint64_t s_headersz; // number of blocks for the cache header
	uint64_t s_bmpsz;    // number of blocks for the bitmap
	uint64_t s_metasz;   // number of blocks for all meta data
	uint64_t s_size;     // number of data blocks
};

struct crc_private_clone {
	char p_uuid[NID_MAX_UUID];
	struct allocator_interface *p_allocator;
	struct cdn_interface p_cdn;
	struct brn_interface *p_brn;
	struct rc_interface *p_rc;
	pthread_mutex_t p_lck;
	struct list_head p_chan_head;
	struct list_head p_inactive_head;
	struct srn_interface *p_srn;
	struct pp_interface *p_pp;
	struct cse_interface p_cse;
	struct fpn_interface *p_fpn;
	struct fpc_interface p_fpc;
	struct blksn_interface p_blksn;
	struct dsmgr_interface p_dsmgr;
	struct dsrec_interface p_dsrec;
	struct crc_super_header p_super;
	uint64_t *p_bitmap;
	struct crc_meta_block *p_meta_data;
	char *p_fp_buf;
	char p_cachedev[NID_MAX_PATH];
	int p_fd;
	uint32_t p_blocksz;  // likely be 4K
	uint32_t p_block_shift;  // LINKELY 12
	uint32_t p_supersz;  // in blocks, likely 1M/4K = 256
	uint64_t p_total_size; // total number of blocks, including super/bitmap/meta/data
	uint64_t p_bmpsz; // bitmap size in number of blocks, must be multiple of 64 blocks
	uint64_t p_metasz;
	uint64_t p_size; // number of data blocks, must be multiple of 64 blocks
	uint8_t p_stop;
};

struct rc_private_clone {
	int p_type;
	void *p_real_rc;
	struct allocator_interface *p_allocator;
	struct fpn_interface *p_fpn;
	struct brn_interface *p_brn;
};

#define UT_RC_READ_DEVICE           "/cse_device"

static struct rc_interface ut_cse;
static struct allocator_interface ut_cse_al;
static struct brn_interface ut_cse_brn;
static struct pp_interface ut_cse_pp;
static struct fpn_interface ut_cse_fpn;
static struct fpc_interface ut_cse_fpc;
static struct rc_private_clone *ut_cse_private;
static struct crc_interface *ut_cse_crc;
static struct crc_private_clone *ut_cse_crc_private;
static struct cse_interface *ut_cse_cse;

static void check_read_device() {
	struct stat fstat;
	char cmd[1024];
	if (stat(UT_RC_READ_DEVICE, &fstat) == 0) {
		sprintf(cmd, "rm %s", UT_RC_READ_DEVICE);
		system(cmd);
	}

	sprintf(cmd, "touch %s", UT_RC_READ_DEVICE);
	if (system(cmd) !=0 ) {
		assert(0);
	}

	printf("cse ut re-create read buffer device: %s \n", UT_RC_READ_DEVICE);
}

static void ut_cse_init_env() {
	check_read_device();

	struct allocator_setup ut_cse_al_setup;
	allocator_initialization(&ut_cse_al, &ut_cse_al_setup);

	struct fpc_setup fpc_setup;
	fpc_setup.fpc_algrm = FPC_SHA256;
	fpc_initialization(&ut_cse_fpc, &fpc_setup);

	struct fpn_setup fpn_setup;
	fpn_setup.allocator = &ut_cse_al;
	fpn_setup.fp_size = 32;
	fpn_setup.set_id = ALLOCATOR_SET_SCG_FPN;
	fpn_setup.seg_size = 4096;
	fpn_initialization(&ut_cse_fpn, &fpn_setup);

	struct pp_setup pp_setup;
	pp_setup.pool_size = 160;
	pp_setup.page_size = 4;
	pp_setup.allocator = &ut_cse_al;
	pp_setup.set_id = ALLOCATOR_SET_SCG_PP;
	pp_setup.name = "CSE_TEST_PP";
	pp_initialization(&ut_cse_pp, &pp_setup);

	struct brn_setup brn_setup;
	brn_setup.allocator = &ut_cse_al;
	brn_setup.set_id = ALLOCATOR_SET_SCG_BRN;
	brn_setup.seg_size = 4096;
	brn_initialization(&ut_cse_brn, &brn_setup);

	struct rc_setup rc_setup;
	rc_setup.allocator = &ut_cse_al;
	rc_setup.brn = &ut_cse_brn;
	rc_setup.cachedev = UT_RC_READ_DEVICE;
	rc_setup.cachedevsz = 4;
	rc_setup.fpn = &ut_cse_fpn;
	rc_setup.pp = &ut_cse_pp;
	rc_setup.type = RC_TYPE_CAS;
	rc_setup.uuid = "123456";

	rc_initialization(&ut_cse, &rc_setup);

	ut_cse_private = (struct rc_private_clone*) ut_cse.rc_private;
	ut_cse_crc = (struct crc_interface *) ut_cse_private->p_real_rc;
	ut_cse_crc_private = (struct crc_private_clone *) ut_cse_crc->cr_private;

	ut_cse_cse = (struct cse_interface*) &ut_cse_crc_private->p_cse;
}

#define UT_CSE_TEST_DATA_NUMBER 2048	// must be multiple of 64.
#define UT_CSE_CD_BASE_LEN (40960)
#define UT_NTHREADS 1

struct test_data {
	u_int64_t index;	// bitmap position
	char fp[32];		// fp
	char data[4096];	// test data
};
static struct test_data ut_cse_td[UT_CSE_TEST_DATA_NUMBER];

static volatile int ut_cse_nthread = UT_NTHREADS;

struct list_head ut_cse_sreq_head;
struct list_head ut_cse_sreq_head2;

static void ut_cse_init_data() {
	int i;
	for (i = 0; i < UT_CSE_TEST_DATA_NUMBER; i++) {
		ut_cse_td[i].index = i;
		sprintf(ut_cse_td[i].fp, "testfp-%010d", rand());
		sprintf(ut_cse_td[i].data, "testdata-%010d", rand());
	}
}

struct crc_private;
void
__recover_meta_data(struct crc_private *, struct list_head *);
void
__load_super_block(struct crc_private *);

/*
 * write some bmp/fp/data into cache disk directly. then read out with __recover_meta_data() and check if the data are right.
 */
void*
do_cse_test() {
	struct crc_private_clone *priv_p = ut_cse_crc->cr_private;
	int i;
	off_t offset;
	int fd = priv_p->p_fd;
	struct list_head cd_head;
	struct content_description_node *cd_np;
	char tmp_data[4096];

	offset = (priv_p->p_supersz << priv_p->p_block_shift);
	int bmp_end = UT_CSE_TEST_DATA_NUMBER / 8;
	char tmp[UT_CSE_TEST_DATA_NUMBER / 8];

	INIT_LIST_HEAD(&cd_head);

	printf("write test bmp/fp/data into cache disk.\n");
	for (i=0; i<bmp_end; i++) {
		tmp[i] = 0xff;
	}
	pwrite(fd, tmp, bmp_end, offset);

	offset = ((priv_p->p_supersz + priv_p->p_bmpsz) << priv_p->p_block_shift);

	for (i=0; i<UT_CSE_TEST_DATA_NUMBER; i++) {
		pwrite(fd, ut_cse_td[i].fp, 32, offset);
		offset += 32;
	}

	offset = ((priv_p->p_supersz + priv_p->p_bmpsz + priv_p->p_metasz) << priv_p->p_block_shift);
	for (i=0; i<UT_CSE_TEST_DATA_NUMBER; i++) {
		pwrite(fd, ut_cse_td[i].data, 4096, offset);
		offset += 4096;
	}

	printf("__load_super_block: load super block of cache disk again.\n");
	__load_super_block((struct crc_private*)priv_p);

	printf("__recover_meta_data: get all content_description_node from loaded super block.\n");
	__recover_meta_data((struct crc_private*)priv_p, &cd_head);

	printf("comparing with orignal fp and data.\n");
	list_for_each_entry(cd_np, struct content_description_node, &cd_head, cn_slist) {
		printf("bmp offset:[%lu] orignal fp[%s] recoverd fp[%s]\t", cd_np->cn_data, cd_np->cn_fp, ut_cse_td[cd_np->cn_data].fp);
		assert(memcmp(cd_np->cn_fp, ut_cse_td[cd_np->cn_data].fp, 32) == 0);
		offset = (priv_p->p_supersz + priv_p->p_bmpsz + priv_p->p_metasz + cd_np->cn_data) << priv_p->p_block_shift;
		pread(fd, tmp_data, 4096, offset);
		printf("orignal data:[%s] recoverd data:[%s]\n", tmp_data, ut_cse_td[cd_np->cn_data].data);
		assert(memcmp(tmp_data, ut_cse_td[cd_np->cn_data].data, 4096) == 0);
	}

	__sync_fetch_and_sub(&ut_cse_nthread, 1);

	return NULL;
}

int main() {
	nid_log_open();
	printf("cse ut2 module start ...\n");

	printf("cse ut init ENV ...\n");
	fflush(stdout);
	ut_cse_init_env();

	printf("cse ut init Data ...\n");
	fflush(stdout);
	ut_cse_init_data();


	pthread_t pid[UT_NTHREADS];
	int i;
	for (i = 0; i < UT_NTHREADS; i++) {
		pthread_create(&pid[i], NULL, do_cse_test, NULL);
	}
	for (i = 0; i < UT_NTHREADS; i++) {
		pthread_join(pid[i], NULL);
	}

	while (ut_cse_nthread) {
		sleep(1);
	}

	printf("\npassed!\ncse ut2 module end...\n");
	fflush(stdout);
	nid_log_close();

	return 0;
}
