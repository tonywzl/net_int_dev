/**
 * UT description:
 * 1, Random generate UT_TWC_REQ_NUM write requests, and write the request data to ut_twc_write_data_dev;
 * 2, Call twc->write_list function send those requests, go through the twc module then write to /dev/sdb;
 * 3, Check /dev/sdb data and ut_twc_write_data_dev data if match or not.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#include "nid_log.h"
#include "crc_if.h"
#include "rc_if.h"
#include "allocator_if.h"
#include "brn_if.h"
#include "fpn_if.h"
#include "fpc_if.h"
#include "pp_if.h"
#include "pp2_if.h"
#include "srn_if.h"
#include "rw_if.h"
#include "twc_if.h"
#include "rc_twcn.h"
#include "rc_twc_cbn.h"
#include "wc_if.h"
#include "nid_shared.h"
#include "tp_if.h"
#include "ds_if.h"
#include "sac_if.h"
#include "lstn_if.h"
#include "nid.h"

#define UT_TWC_READ_DEVICE           	"/rc_device"
#define UT_TWC_DRW_DEVICE		"/dev/sdb"
#define TWC_UT_TWC_UUID			"UUID_TWC_123"
#define UT_TWC_WC_PPAGE_SZ		8
#define UT_TWC_SEG_SZ 			1044480
#define UT_TWC_MAX_OFFSET 		(1024 * 1024 * 10L)
#define UT_TWC_MIN_BK_LEN 		(10)
#define UT_TWC_MAX_BK_LEN 		(UT_TWC_SEG_SZ / 4096)
#define UT_TWC_REQ_NUM 			2000
#define UT_TWC_REPEAT_WRITE_TIME	4

struct ut_twc_write_req {
	off_t 		off;
	uint32_t 	len;
	void		*buf;
};

volatile int 				ut_twc_write_req_num = UT_TWC_REQ_NUM;
struct ut_twc_write_req 		*ut_twc_req;
volatile int				ut_twc_respnose_left = 0;
volatile int				ut_twc_flush_left = 0;
char					*ut_twc_write_data_dev = "/ut_twc_write_data_dev";
int 					ut_twc_write_req_max_num = UT_TWC_REQ_NUM + UT_TWC_WC_PPAGE_SZ;
static volatile uint64_t 		ut_twc_write_seq = 0;

static struct twc_interface		*ut_twc_p;
static void*                        	ut_twc_handle;
static struct rc_interface          	ut_twc_rc;
static void*                        	ut_twc_rc_handle;
static struct allocator_interface   	ut_twc_al;
static struct brn_interface         	ut_twc_brn;
static struct pp_interface          	ut_twc_rc_pp;
static struct pp_interface          	ut_twc_wc_pp;
static struct srn_interface         	ut_twc_srn;
static struct fpn_interface         	ut_twc_fpn;
static struct fpn_interface         	ut_twc_fpn_data;
static struct fpc_interface         	ut_twc_fpc;
static struct rw_interface	    	ut_twc_rw;
static struct lstn_interface		ut_twc_lstn;
static void* 			    	ut_twc_rw_handle;
struct io_interface 			ut_twc_io;
static void*				ut_twc_io_handle;
static struct rc_twcn_interface	    	ut_twc_twcn;
static struct rc_twc_cbn_interface	ut_twc_twc_cbn;
static struct tp_interface		ut_twc_tp;
static struct wc_interface		ut_twc_wc;
static void*				ut_twc_wc_handle;
struct ds_interface			ut_twc_ds;
int					ut_twc_in_ds;
static int				ut_twc_out_ds;
static uint32_t				ut_twc_ds_pagesz;
static struct sac_interface		ut_twc_sac;

static void
check_device(char *path)
{
    struct stat fstat;
    char cmd[1024];
    int ret = stat(path, &fstat);

    if (ret == -1) {
        if (errno == ENOENT ) {
            // File not exist, create one.
            sprintf(cmd, "touch %s", path);
            system(cmd);
        } else {
            assert(0);
        }
    }
    printf("rc ut clean read buffer device: %s \n", path);
    sprintf(cmd, "dd if=/dev/zero of=%s bs=1M count=64 oflag=sync",
		    path);
    system(cmd);
}

static struct request_node *
ut_twc_gen_write_request_node(char *buf, off_t off, uint32_t len)
{
	struct request_node *rn_p;
	uint32_t to_read, to_read2;
	char *ds_buf;

	rn_p = calloc(sizeof(*rn_p), 1);
	rn_p->r_ds = &ut_twc_ds;
	rn_p->r_ds_index = ut_twc_in_ds;
	rn_p->r_sac = &ut_twc_sac;
	rn_p->r_ir.cmd = NID_REQ_WRITE;
	rn_p->r_ir.len = len;
	rn_p->r_ir.offset = off;
	rn_p->r_len = len;
	rn_p->r_offset = off;

	assert(len<=ut_twc_ds_pagesz);

	ds_buf = ut_twc_ds.d_op->d_get_buffer(&ut_twc_ds, ut_twc_in_ds, &to_read, &ut_twc_io);
	if (to_read >= len) {
		memcpy(ds_buf, buf, len);
		ut_twc_ds.d_op->d_confirm(&ut_twc_ds, ut_twc_in_ds, len);
		rn_p->r_resp_buf_1 = ds_buf;
	} else {
		memcpy(ds_buf, buf, to_read);
		ut_twc_ds.d_op->d_confirm(&ut_twc_ds, ut_twc_in_ds, to_read);

		ds_buf = ut_twc_ds.d_op->d_get_buffer(&ut_twc_ds, ut_twc_in_ds, &to_read2, &ut_twc_io);
		assert(to_read2 >= len-to_read);
		memcpy(ds_buf, buf+to_read, len-to_read);
		ut_twc_ds.d_op->d_confirm(&ut_twc_ds, ut_twc_in_ds, len-to_read);
	}

	rn_p->r_ir.dseq = ut_twc_write_seq;
	rn_p->r_seq = rn_p->r_ir.dseq ;
	rn_p->r_io_handle = ut_twc_wc_handle;
	gettimeofday(&rn_p->r_recv_time, NULL);
	ut_twc_write_seq += len;

	return rn_p;
}

static int
ut_twc_open_write_device(char *path)
{
	struct stat fstat;
	char cmd[1024];
	int ret = stat(path, &fstat);
	if (ret == -1) {
		if (errno == ENOENT ) {
		    // File not exist, create one.
		    sprintf(cmd, "touch %s", path);
		    system(cmd);
		} else {
		    assert(0);
		}
	}
	mode_t mode;
	int flags, fd;
	mode = 0x600;
	flags = O_RDWR;
	flags |= O_SYNC;
	fd = open(path, flags, mode);
	if (fd < 0) {
		printf("Cannot open %s\n", path);
		assert(0);
	}
	return fd;
}

static void
ut_twc_gen_data()
{
	ut_twc_req = malloc(sizeof(*ut_twc_req) * ut_twc_write_req_max_num);
	int i, fd;
	size_t total = 0, len;
	time_t t;
	time(&t);
	srand((unsigned int)t);
	fd = ut_twc_open_write_device(ut_twc_write_data_dev);
	for (i=0; i<ut_twc_write_req_num; i++) {
		ut_twc_req[i].off = ((rand() % UT_TWC_MAX_OFFSET) & (~(4096 - 1)));

		assert(ut_twc_req[i].off < UT_TWC_MAX_OFFSET);
		ut_twc_req[i].len = (UT_TWC_MIN_BK_LEN + (rand() % (UT_TWC_MAX_BK_LEN - UT_TWC_MIN_BK_LEN))) * 4096;
		ut_twc_req[i].buf = malloc(ut_twc_req[i].len);
		lseek(fd, ut_twc_req[i].off,  SEEK_SET);
		write(fd, ut_twc_req[i].buf, ut_twc_req[i].len);
		total += ut_twc_req[i].len;
		printf("Generate request:index:%d offset:%ld length:%u\n",
				i, ut_twc_req[i].off, ut_twc_req[i].len);
		fflush(stdout);
	}
	// Fill data to page size
	while ((len = (UT_TWC_WC_PPAGE_SZ << 20) - (total % (UT_TWC_WC_PPAGE_SZ << 20))) != (UT_TWC_WC_PPAGE_SZ << 20) ) {
		len = len > UT_TWC_SEG_SZ? UT_TWC_SEG_SZ : len;
		ut_twc_req[i].off = (rand() % UT_TWC_MAX_OFFSET) & (~(4096 - 1));
		ut_twc_req[i].len = len;
		ut_twc_req[i].buf = malloc(ut_twc_req[i].len);

		lseek(fd, ut_twc_req[i].off,  SEEK_SET);
		write(fd, ut_twc_req[i].buf, ut_twc_req[i].len);

		printf("Generate request:index:%d offset:%ld length:%u\n",
				i, ut_twc_req[i].off, ut_twc_req[i].len);
		fflush(stdout);
		total += ut_twc_req[i].len;

		i++;
		ut_twc_write_req_num ++;
	}
	printf("Total size: %lu\n\n", total);
	ut_twc_flush_left = total / (UT_TWC_WC_PPAGE_SZ << 20);
	close(fd);
}


static void
ut_twc_write_data()
{
	struct request_node *rn_p;
	struct list_head ut_write_head,
		*ut_write_head_p = &ut_write_head;
	int ut_data_counter = 0;
	int i;

	INIT_LIST_HEAD(ut_write_head_p);
	for (i=0; i<ut_twc_write_req_num; i++) {
		rn_p = ut_twc_gen_write_request_node(ut_twc_req[i].buf, ut_twc_req[i].off, ut_twc_req[i].len);
		list_add_tail(&rn_p->r_write_list, ut_write_head_p);
		ut_data_counter ++;
		__sync_add_and_fetch(&ut_twc_respnose_left, 1);
	}
	ut_twc_p->tw_op->tw_write_list(ut_twc_p, ut_twc_handle, ut_write_head_p, ut_data_counter);
}

static void
ut_validate_write_data()
{
	int i, fd, fd1;
	char buf[UT_TWC_MAX_BK_LEN*4096], buf1[UT_TWC_MAX_BK_LEN*4096];
	fd = ut_twc_open_write_device(UT_TWC_DRW_DEVICE);
	fd1 = ut_twc_open_write_device(ut_twc_write_data_dev);

	for (i=0; i<ut_twc_write_req_num; i++) {
		lseek(fd, ut_twc_req[i].off,  SEEK_SET);
		read(fd, buf, ut_twc_req[i].len);

		lseek(fd1, ut_twc_req[i].off,  SEEK_SET);
		read(fd1, buf1, ut_twc_req[i].len);
		printf("Compare index:%d, compare offset:%ld, compare len:%u...",
				i, ut_twc_req[i].off, ut_twc_req[i].len);
		fflush(stdout);
		assert(memcmp(buf, buf1, ut_twc_req[i].len) == 0);
		free(ut_twc_req[i].buf);
		printf("Success.\n");
	}
	close(fd);
	close(fd1);
	free(ut_twc_req);

}

struct ut_wc_private {
	char				p_name[NID_MAX_UUID];
	int				p_type;
	void				*p_real_wc;
	struct allocator_interface	*p_allocator;
	int				p_recovered;
};

static void
ut_twc_init_env ()
{
	char cmd[128];
	sprintf(cmd, "rm %s; touch %s;", ut_twc_write_data_dev, ut_twc_write_data_dev);
	system(cmd);

	check_device(UT_TWC_READ_DEVICE);

	struct allocator_setup ut_twc_al_setup;
	ut_twc_al_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(&ut_twc_al, &ut_twc_al_setup);

	struct fpc_setup fpc_setup;
	fpc_setup.fpc_algrm = FPC_SHA256;
	fpc_initialization(&ut_twc_fpc, &fpc_setup);

	struct fpn_setup fpn_setup;
	fpn_setup.allocator = &ut_twc_al;
	fpn_setup.fp_size = 32;
	fpn_setup.set_id = ALLOCATOR_SET_SCG_FPN;
	fpn_setup.seg_size = 4096;
	fpn_initialization(&ut_twc_fpn, &fpn_setup);
	fpn_initialization(&ut_twc_fpn_data, &fpn_setup);

	struct srn_setup srn_setup;
	srn_setup.allocator = &ut_twc_al;
	srn_setup.set_id = ALLOCATOR_SET_SCG_SRN;
	srn_setup.seg_size = 1024;
	srn_initialization(&ut_twc_srn, &srn_setup);

	struct pp_setup rc_pp_setup;
	rc_pp_setup.pool_size = 160;
	rc_pp_setup.page_size = 4;
	rc_pp_setup.allocator =  &ut_twc_al;
	rc_pp_setup.set_id = ALLOCATOR_SET_SCG_PP;
	strcpy(rc_pp_setup.pp_name, "RC_TEST_PP");
	pp_initialization(&ut_twc_rc_pp, &rc_pp_setup);

	struct pp_setup wc_pp_setup;
	wc_pp_setup.pool_size = 1024;
	wc_pp_setup.page_size = UT_TWC_WC_PPAGE_SZ;
	wc_pp_setup.allocator =  &ut_twc_al;
	wc_pp_setup.set_id = ALLOCATOR_SET_SCG_PP;
	strcpy(wc_pp_setup.pp_name, "WC_TEST_PP");
	pp_initialization(&ut_twc_wc_pp, &wc_pp_setup);

	struct brn_setup brn_setup;
	brn_setup.allocator = &ut_twc_al;
	brn_setup.set_id = ALLOCATOR_SET_SCG_BRN;
	brn_setup.seg_size = 4096;
	brn_initialization(&ut_twc_brn, &brn_setup);

	struct rc_setup rc_setup;
	rc_setup.allocator = &ut_twc_al;
	rc_setup.brn = &ut_twc_brn;
	rc_setup.cachedev = UT_TWC_READ_DEVICE;
	rc_setup.cachedevsz = 4;
	rc_setup.fpn = &ut_twc_fpn;
	rc_setup.pp = &ut_twc_rc_pp;
	rc_setup.srn = &ut_twc_srn;
	rc_setup.type = RC_TYPE_CAS;
	rc_setup.uuid = "123456";

	rc_initialization(&ut_twc_rc, &rc_setup);

	struct rw_setup rw_setup;
	rw_setup.allocator = &ut_twc_al;
	rw_setup.fpn_p = &ut_twc_fpn;
	rw_setup.type = RW_TYPE_DEVICE;
	rw_setup.name = "RW_UT";
	rw_setup.exportname = UT_TWC_DRW_DEVICE;
	rw_initialization(&ut_twc_rw, &rw_setup);
	ut_twc_rw_handle = ut_twc_rw.rw_op->rw_create_worker(&ut_twc_rw, UT_TWC_DRW_DEVICE, 1, 1, 4096);

	struct rc_twcn_setup rmsetup;
	rmsetup.allocator = &ut_twc_al;
	rmsetup.seg_size = 16;
	rmsetup.set_id = ALLOCATOR_SET_TWC_RCTWC;
	rc_twcn_initialization(&ut_twc_twcn, &rmsetup);

	struct rc_twc_cbn_setup rmc_setup;
	rmc_setup.allocator = &ut_twc_al;
	rmc_setup.seg_size = 128;
	rmc_setup.set_id = ALLOCATOR_SET_TWC_RCTWC_CB;
	rc_twc_cbn_initialization(&ut_twc_twc_cbn, &rmc_setup);

	struct rc_channel_info cinfo;
	cinfo.rc_rw = &ut_twc_rw;
	cinfo.rc_rw_handle = ut_twc_rw_handle;

	int new_worker;
	ut_twc_rc_handle = ut_twc_rc.rc_op->rc_create_channel(&ut_twc_rc, NULL, &cinfo, UT_TWC_READ_DEVICE, &new_worker);

	struct pp2_setup pp2_setup;
	struct pp2_interface *pp2_p;
	pp2_p = calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "server_pp2";
	pp2_setup.allocator = &ut_twc_al;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_PP2;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 16;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 1;
	pp2_initialization(pp2_p, &pp2_setup);

	struct tp_setup tp_setup;
	tp_setup.delay = 0;
	tp_setup.extend = 2;
	tp_setup.min_workers = 4;
	tp_setup.max_workers = 8;
	tp_setup.name = "TWC_UT1_TP";
	tp_setup.pp2 = pp2_p;
	tp_initialization(&ut_twc_tp, &tp_setup);

	sac_initialization(&ut_twc_sac, NULL);

	struct lstn_setup lstn_setup;
	lstn_setup.allocator = &ut_twc_al;
	lstn_setup.seg_size = 128;
	lstn_setup.set_id = 0;
	lstn_initialization(&ut_twc_lstn, &lstn_setup);

	struct wc_setup wc_setup;
	wc_setup.allocator = &ut_twc_al;
	wc_setup.do_fp = 1;
	wc_setup.lstn = &ut_twc_lstn;
	wc_setup.pp = &ut_twc_wc_pp;
	wc_setup.rw_sync = 1;
	wc_setup.srn = &ut_twc_srn;
	wc_setup.tp = &ut_twc_tp;
	wc_setup.type = WC_TYPE_TWC;
	wc_setup.uuid = "TWC_UT1";
	wc_initialization(&ut_twc_wc, &wc_setup);

	struct wc_channel_info wc_info;
	wc_info.wc_rw = &ut_twc_rw;
	wc_info.wc_rw_handle = ut_twc_rw_handle;
	wc_info.wc_rc = &ut_twc_rc;
	wc_info.wc_rc_handle = ut_twc_rc_handle;
	ut_twc_wc.wc_op->wc_recover(&ut_twc_wc);
	ut_twc_wc_handle = ut_twc_wc.wc_op->wc_create_channel(&ut_twc_wc, &ut_twc_sac, &wc_info, TWC_UT_TWC_UUID, &new_worker);

	ut_twc_p = ((struct ut_wc_private*)ut_twc_wc.wc_private)->p_real_wc;
	ut_twc_handle = ut_twc_wc_handle;

	struct ds_setup ds_setup;
	ds_setup.name = "UT_DS";
	ds_setup.pagenrshift = 2;
	ds_setup.pageszshift = 23;
	ds_setup.pool = &ut_twc_wc_pp;
	ds_setup.rc_name = rc_setup.uuid;
	ds_setup.type = DS_TYPE_SPLIT;
	ds_setup.wc_name = wc_setup.uuid;
	ds_initialization(&ut_twc_ds, &ds_setup);

	struct io_setup io_setup;
	io_setup.ini = NULL;
	io_setup.io_type = IO_TYPE_BUFFER;
	io_setup.pool =  &ut_twc_wc_pp;
	io_setup.tp = &ut_twc_tp;
	io_initialization(&ut_twc_io, &io_setup);

	struct io_channel_info io_info;
	io_info.io_rc = &ut_twc_rc;
	io_info.io_rc_handle = ut_twc_rc_handle;
	io_info.io_rw = &ut_twc_rw;
	io_info.io_rw_handle = ut_twc_rw_handle;
	io_info.io_wc = &ut_twc_wc;
	io_info.io_wc_handle = ut_twc_wc_handle;

	ut_twc_io_handle = ut_twc_io.io_op->io_create_worker(&ut_twc_io, NULL, &io_info, UT_TWC_DRW_DEVICE, &new_worker);

	struct ds_interface *ds_p = &ut_twc_ds;
	ut_twc_in_ds = ds_p->d_op->d_create_worker(ds_p, ut_twc_io_handle, 1, &ut_twc_io);
	if (ut_twc_in_ds < 0) {
		assert(0);
	}
	ut_twc_out_ds = ds_p->d_op->d_create_worker(ds_p, ut_twc_io_handle, 0, &ut_twc_io);
	if (ut_twc_out_ds < 0) {
		assert(0);
	}
	ut_twc_ds_pagesz = ds_p->d_op->d_get_pagesz(ds_p, ut_twc_out_ds);
}

int
main()
{
	nid_log_open();
	printf("twc ut module start ...\n");

	printf("twc ut init ENV ...\n"); fflush(stdout);

	ut_twc_init_env();

	printf("twc ut init DATA ...\n"); fflush(stdout);
	ut_twc_gen_data();

	printf("twc ut write DATA ...\n"); fflush(stdout);
	int i;
	for (i=0; i<UT_TWC_REPEAT_WRITE_TIME; i++) {
		ut_twc_write_data();
		usleep(1000);
	}

	while(ut_twc_respnose_left) {
		printf("Waiting response & flush!! ut_twc_respnose_left:%d\nPress any key to continue...\n\n", ut_twc_respnose_left);
		usleep(1000000);
	}
	printf("ut_twc_respnose_left:%d\n", ut_twc_respnose_left);
	fflush(stdout);

	printf("twc ut validate DATA ...\n"); fflush(stdout);
	ut_validate_write_data();

	printf("\ntwc ut module end...\n"); fflush(stdout);
	nid_log_close();

	return 0;
}
