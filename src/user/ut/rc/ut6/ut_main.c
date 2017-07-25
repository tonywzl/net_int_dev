#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

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

/**
 * TEST DESCRIPTION
	1, Update 9G file data to read cache;
	2, Start a random update thread for update read cache randomly;
	3, At the same time, read read cache and read the 9G file block by block, and compare them.
 */

#define UT_RC_READ_DEVICE	"/rc_device"
#define UT_GEN_FILE		"/tmp/testdata"
#define UT_GEN_FILE_BS		"1M"
#define UT_GEN_FILE_COUNT	"9000"
#define UT_UPDATE_RC_BLK_SZ	(1024*1024LU)
#define UT_UPDATE_RC_File_SZ	(1024*1024*9000LU)

static char ut_fnm[1024];
static struct rc_interface          ut_rc;
static void*                        ut_rc_handle;
static struct allocator_interface   ut_rc_al;
static struct brn_interface         ut_rc_brn;
static struct pp_interface          ut_rc_pp;
static struct srn_interface         ut_rc_srn;
static struct fpn_interface         ut_rc_fpn;
static struct fpn_interface         ut_rc_fpn_data;
static struct fpc_interface         ut_rc_fpc;

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
    printf("Clean read buffer device: %s \n", UT_RC_READ_DEVICE);
    sprintf(cmd, "dd if=/dev/zero of=%s bs=1M count=64 oflag=sync",
            UT_RC_READ_DEVICE);
    system(cmd);
}

static void
ut_rc_gen_data ()
{
	struct stat fstat;
	char cmd[1024];
	sprintf(ut_fnm, "%s%s%s", UT_GEN_FILE, UT_GEN_FILE_BS, UT_GEN_FILE_COUNT);
	printf("Generate file: %s \n", ut_fnm);
	int ret = stat(ut_fnm, &fstat);
	if (ret == -1) {
		if (errno == ENOENT ) {
		    // File not exist, create one.
		    sprintf(cmd, "touch %s", ut_fnm);
		    system(cmd);
		    sprintf(cmd, "dd if=/dev/zero of=%s bs=%s count=%s oflag=sync", ut_fnm, UT_GEN_FILE_BS, UT_GEN_FILE_COUNT);
		    system(cmd);
		} else {
		    assert(0);
		}
	}
	printf("Generate file: %s Success\n", ut_fnm);
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

static int
openfile(char *path)
{
    mode_t mode;
    int flags;

    mode = 0x644;
    flags = O_RDONLY;
    flags |= O_SYNC;
    return open(path, flags, mode);
}

static void
ut_rc_update_file_to_readcache()
{
	int fd = openfile(ut_fnm);
	char bk[UT_UPDATE_RC_BLK_SZ];
	int n;
	off_t off=0;
	struct list_head fp_head;
        INIT_LIST_HEAD(&fp_head);

	while ((n=read(fd, bk, UT_UPDATE_RC_BLK_SZ)) > 0) {
		assert(n == UT_UPDATE_RC_BLK_SZ);
		ut_rc_fpc.fpc_op->fpc_bcalculate(&ut_rc_fpc, &ut_rc_fpn_data, bk, n, 4096, &fp_head);
		ut_rc.rc_op->rc_update(&ut_rc, ut_rc_handle, bk, n, off, &fp_head);
		off += n;
	}
	close(fd);
}

static void
ut_rc_cache_read(char *buf, off_t off, size_t len)
{
	struct sub_request_node sreq, *sreq1, *sreq2;
	struct list_head res_head;
	int n;

	INIT_LIST_HEAD(&res_head);
	sreq.sr_len = len;
	sreq.sr_buf = buf;
	sreq.sr_offset = off;

	ut_rc.rc_op->rc_search(&ut_rc, ut_rc_handle, &sreq, &res_head);
	if (!list_empty(&res_head)) {
		list_for_each_entry_safe(sreq1, sreq2, struct sub_request_node, &res_head, sr_list) {
			int fd = openfile(ut_fnm);
			lseek(fd, sreq1->sr_offset, SEEK_SET);
			n = read(fd, sreq1->sr_buf, sreq1->sr_len);
			close(fd);
			assert(((uint32_t)n) == sreq1->sr_len);
			list_del(&sreq1->sr_list);
			ut_rc_srn.sr_op->sr_put_node(&ut_rc_srn, sreq1);
		}
	}
}

static void
ut_rc_read(char *buf, off_t off, size_t len)
{
	int n;
	int fd = openfile(ut_fnm);
	lseek(fd, off, SEEK_SET);
	n = read(fd, buf, len);
	assert(((size_t)n) == len);
	close(fd);
}

static void
ut_rc_read_and_compare()
{

	char bk1[UT_UPDATE_RC_BLK_SZ];
	char bk2[UT_UPDATE_RC_BLK_SZ];
	off_t off = 0;
	while (off < (long)UT_UPDATE_RC_File_SZ) {
		ut_rc_cache_read(bk1, off, UT_UPDATE_RC_BLK_SZ);
		ut_rc_read(bk2, off, UT_UPDATE_RC_BLK_SZ);
		if (memcmp(bk1, bk2, UT_UPDATE_RC_BLK_SZ) != 0) {
			printf("\nError, data mismatch, offset: %ld len:%ld\n\n", off, UT_UPDATE_RC_BLK_SZ);
			fflush(stdout);
			assert(0);
		}
		off += UT_UPDATE_RC_BLK_SZ;
	}
}

static volatile int UT_STOP = 0;

static void *
ut_rc_update_thread(void *p)
{
	(void)p;
	int fd = openfile(ut_fnm);
	char bk[UT_UPDATE_RC_BLK_SZ];
	int n;
	off_t off=0;
	struct list_head fp_head;
	INIT_LIST_HEAD(&fp_head);
	off_t totalbk = UT_UPDATE_RC_File_SZ / UT_UPDATE_RC_BLK_SZ;

	while ( !UT_STOP ) {
		off = (rand() % totalbk) * UT_UPDATE_RC_BLK_SZ;
		lseek(fd, off, SEEK_SET);
		n=read(fd, bk, UT_UPDATE_RC_BLK_SZ);
		assert(n == UT_UPDATE_RC_BLK_SZ);
		ut_rc_fpc.fpc_op->fpc_bcalculate(&ut_rc_fpc, &ut_rc_fpn_data, bk, n, 4096, &fp_head);
		ut_rc.rc_op->rc_update(&ut_rc, ut_rc_handle, bk, n, off, &fp_head);
		usleep(1000);
	}
	close(fd);
	return NULL;
}

static void
ut_rc_update_file_to_readcache_random_thread() {
	pthread_t tid;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_create(&tid, &attr, ut_rc_update_thread, NULL);

}

int
main()
{
	nid_log_open();
	printf("rc ut module start ...\n");

	printf("rc ut init ENV ...\n"); fflush(stdout);

	ut_rc_init_env();

	printf("rc ut init DATA ...\n"); fflush(stdout);
	ut_rc_gen_data();

	printf("\nStep 1: rc_update %s file to the read cache.\n", ut_fnm); fflush(stdout);
	ut_rc_update_file_to_readcache();

	printf("\nStep 2: Start random update read cache thread.\n"); fflush(stdout);
	ut_rc_update_file_to_readcache_random_thread();

	printf("\nStep 3: Read and compare read cache data, from original file %s and read cache data.\n", ut_fnm); fflush(stdout);
	ut_rc_read_and_compare();

	__sync_add_and_fetch(&UT_STOP, 1);
	printf("\nrc ut6 module test success and end...\n"); fflush(stdout);
	nid_log_close();

	return 0;
}
