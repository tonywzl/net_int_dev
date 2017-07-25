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

#define UT_RC_READ_DEVICE           "/rc_device"

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
	int ret = stat(UT_RC_READ_DEVICE, &fstat);

	if (ret == -1) {
		if (errno == ENOENT) {
			// File not exist, create one.
			sprintf(cmd, "touch %s", UT_RC_READ_DEVICE);
			system(cmd);
		} else {
			assert(0);
		}
	}
	printf("cse ut clean read buffer device: %s \n", UT_RC_READ_DEVICE);
	sprintf(cmd, "dd if=/dev/zero of=%s bs=1M count=64 oflag=sync", UT_RC_READ_DEVICE);
	system(cmd);
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

#define UT_CSE_CD_NUMBER 2048
#define UT_CSE_CD_BASE_LEN (40960)
#define UT_NTHREADS 1

static struct content_description_node *ut_cse_cnp[UT_CSE_CD_NUMBER];
static struct content_description_node *ut_cse_cnp2[UT_CSE_CD_NUMBER];
static struct list_head fp_head;
static struct list_head fp_head2;

static volatile int ut_cse_nthread = UT_NTHREADS;

struct list_head ut_cse_sreq_head;
struct list_head ut_cse_sreq_head2;

static void ut_cse_init_data() {
	int i;

	INIT_LIST_HEAD(&fp_head);
	struct fp_node *fp;
	struct cdn_interface *cdnp = &ut_cse_crc_private->p_cdn;
	struct fpn_interface *fpnp = ut_cse_crc_private->p_fpn;
	for (i = 0; i < UT_CSE_CD_NUMBER; i++) {
		fp = fpnp->fp_op->fp_get_node(fpnp);
		ut_cse_cnp[i] = cdnp->cn_op->cn_get_node(cdnp);
		ut_cse_cnp[i]->cn_data = i;
		sprintf(ut_cse_cnp[i]->cn_fp, "testfp%d", i);
		ut_cse_cnp[i]->cn_is_ondisk = 1;
		ut_cse_cnp[i]->cn_is_lru = 0;

		sprintf(fp->fp, "%s", ut_cse_cnp[i]->cn_fp);
		list_add_tail(&fp->fp_list, &fp_head);
	}

	uint32_t len;
	off_t off = 0;
	struct sub_request_node *rn_p;
	INIT_LIST_HEAD(&ut_cse_sreq_head);
	for (i = 0; i < UT_CSE_CD_NUMBER; i++) {
		len = UT_CSE_CD_BASE_LEN;
		while (len) {
			rn_p = malloc(sizeof(*rn_p));
			rn_p->sr_buf = malloc(4096);
			assert(rn_p->sr_buf);
			rn_p->sr_len = 4096;
			rn_p->sr_offset = off;
			list_add_tail(&rn_p->sr_list, &ut_cse_sreq_head);
			off += 4096;
			len -= 4096;
		}
	}

	INIT_LIST_HEAD(&fp_head2);
	for (i = 0; i < UT_CSE_CD_NUMBER; i++) {
		fp = fpnp->fp_op->fp_get_node(fpnp);
		ut_cse_cnp2[i] = cdnp->cn_op->cn_get_node(cdnp);
		ut_cse_cnp2[i]->cn_data = i;
		sprintf(ut_cse_cnp2[i]->cn_fp, "wrong_testfp%d", i);
		ut_cse_cnp2[i]->cn_is_ondisk = 1;
		ut_cse_cnp2[i]->cn_is_lru = 0;

		sprintf(fp->fp, "%s", ut_cse_cnp2[i]->cn_fp);
		list_add_tail(&fp->fp_list, &fp_head2);
	}

	INIT_LIST_HEAD(&ut_cse_sreq_head2);
	for (i = 0; i < UT_CSE_CD_NUMBER; i++) {
		len = UT_CSE_CD_BASE_LEN;
		while (len) {
			rn_p = malloc(sizeof(*rn_p));
			rn_p->sr_buf = malloc(4096);
			assert(rn_p->sr_buf);
			rn_p->sr_len = 4096;
			rn_p->sr_offset = off;
			list_add_tail(&rn_p->sr_list, &ut_cse_sreq_head2);
			off += 4096;
			len -= 4096;
		}
	}
}

/*
 * test if the fp can be found after invoking cs_recover_fp()
 */
void*
do_cse_test() {
	int i;
	struct list_head ut_cse_not_found_sreq_head;
	int expecting_not_found;
	struct sub_request_node *rn_p;
	struct list_head cd_head;


	INIT_LIST_HEAD(&ut_cse_not_found_sreq_head);
	INIT_LIST_HEAD(&cd_head);

	for (i = 0; i < UT_CSE_CD_NUMBER; i++) {
		list_add_tail(&ut_cse_cnp[i]->cn_slist, &cd_head);
	}

	printf("cs_recover_fp: recovering fp.\n\n");
	ut_cse_cse->cs_op->cs_recover_fp(ut_cse_cse, &cd_head);

	ut_cse_cse->cs_op->cs_search_list(ut_cse_cse, &fp_head, &ut_cse_sreq_head,
			&ut_cse_not_found_sreq_head);
	printf("cs_search_list: search all recovered fp.\n");
	assert(list_empty(&ut_cse_not_found_sreq_head));
	printf("all fp are found!\n\n");

	expecting_not_found = 0;
	ut_cse_cse->cs_op->cs_search_list(ut_cse_cse, &fp_head2, &ut_cse_sreq_head2,
				&ut_cse_not_found_sreq_head);
	list_for_each_entry(rn_p, struct sub_request_node, &ut_cse_not_found_sreq_head, sr_list) {
		expecting_not_found++;
	}

	printf("cs_search_list: search all non-recovered fp.\n");
	assert(expecting_not_found == UT_CSE_CD_NUMBER);
	printf("all fp are not found!\n");

	__sync_fetch_and_sub(&ut_cse_nthread, 1);

	return NULL;
}

int main() {
	nid_log_open();
	printf("cse ut1 module start ...\n");

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

	printf("\npassed!\ncse ut1 module end...\n");
	fflush(stdout);
	nid_log_close();

	return 0;
}
