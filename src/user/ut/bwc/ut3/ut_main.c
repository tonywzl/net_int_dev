/**
 * UT description:
 * 1, Random generate UT_BWC_REQ_NUM write requests with same offset, and write the request data to ut_bwc_write_data_dev;
 * 2, Call bwc->write_list function send those requests, go through the bwc module then write to /dev/sdb;
 * 3, Check /dev/sdb data and ut_bwc_write_data_dev data if match or not.
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
#include "pp2_if.h"
#include "srn_if.h"
#include "rw_if.h"
#include "bwc_if.h"
#include "bfp_if.h"
#include "wc_if.h"
#include "nid_shared.h"
#include "tp_if.h"
#include "ds_if.h"
#include "sac_if.h"
#include "lstn_if.h"
#include "nid.h"
#include "ut_main.h"

#define UT_BWC_READ_DEVICE           	"/rc_device"
#define UT_BWC_WRITE_DEVICE           	"/wc_device"
#define UT_BWC_DRW_DEVICE		"/dev/sdb"
#define BWC_UT_BWC_UUID			"UUID_BWC_123"
#define UT_BWC_WC_PPAGE_SZ		8
#define UT_BWC_SEG_SZ 			1044480
#define UT_BWC_MAX_OFFSET 		(1024 * 1024 * 100L)
#define UT_BWC_MIN_BK_LEN 		(10)
#define UT_BWC_MAX_BK_LEN 		(UT_BWC_SEG_SZ / 4096)
#define UT_BWC_REQ_NUM 			1000
#define UT_BWC_REPEAT_WRITE_TIME	1

struct ut_bwc_write_req {
	off_t 		off;
	uint32_t 	len;
	void		*buf;
};

volatile int 				ut_bwc_write_req_num = UT_BWC_REQ_NUM;
struct ut_bwc_write_req 		*ut_bwc_req;
volatile int				ut_bwc_respnose_left = 0;
volatile int				ut_bwc_flush_left = 0;
char					*ut_bwc_write_data_dev = "/ut_bwc_write_data_dev";
int 					ut_bwc_write_req_max_num = UT_BWC_REQ_NUM + UT_BWC_WC_PPAGE_SZ;
static volatile uint64_t 		ut_bwc_write_seq = 0;

static struct bwc_interface		*ut_bwc_p;
static void*                        	ut_bwc_handle;
static struct rc_interface          	ut_bwc_rc;
static void*                        	ut_bwc_rc_handle;
static struct allocator_interface   	ut_bwc_al;
static struct brn_interface         	ut_bwc_brn;
static struct pp_interface          	ut_bwc_rc_pp;
static struct pp_interface          	ut_bwc_wc_pp;
static struct srn_interface         	ut_bwc_srn;
static struct fpn_interface         	ut_bwc_fpn;
static struct fpn_interface         	ut_bwc_fpn_data;
static struct fpc_interface         	ut_bwc_fpc;
static struct rw_interface	    	ut_bwc_rw;
static struct lstn_interface		ut_bwc_lstn;
static void* 			    	ut_bwc_rw_handle;
struct io_interface 			ut_bwc_io;
static void*				ut_bwc_io_handle;
static struct rc_wcn_interface	    	ut_bwc_bwcn;
static struct rc_wc_cbn_interface	ut_bwc_bwc_cbn;
static struct tp_interface		ut_bwc_tp;
static struct wc_interface		ut_bwc_wc;
static void*				ut_bwc_wc_handle;
struct ds_interface			ut_bwc_ds;
int					ut_bwc_in_ds;
static int				ut_bwc_out_ds;
static uint32_t				ut_bwc_ds_pagesz;
static struct sac_interface		ut_bwc_sac;

int	ut_bwc_write_total_cross_page = 4;
struct ut_bwc_page_blocks	*ut_bwc_cross_page_offset;
char *ut_bwc_validate_buf, *ut_bwc_validate_buf1;
struct ut_bwc_write_req 		*ut_bwc_req;

static void
auto_generate_cross_page_offset(int dataStreamMB, int targetDataMB)
{
	ut_bwc_write_total_cross_page = dataStreamMB / UT_BWC_WC_PPAGE_SZ;
	int segsize = UT_BWC_WC_PPAGE_SZ << 20;
	int unit = segsize >> 12;
	int off_range = (targetDataMB << 20 >> 12) - (UT_BWC_SEG_SZ >> 12);
	int i, j, off_num, pre_off, pre_len, left_len;
	time_t t;

	ut_bwc_cross_page_offset = malloc(sizeof(struct ut_bwc_page_blocks) *
			ut_bwc_write_total_cross_page);
	time(&t);
	srand((unsigned int)t);
	printf("Data pattern:\nstruct ut_bwc_page_blocks 		ut_bwc_cross_page_offset_default[] = {\n");
	for (i=0; i < ut_bwc_write_total_cross_page; i++) {
		off_num = (rand() % UT_BWC_MAX_OFFSET + 1) / 3 * 3;
		off_num = off_num == 0 ? 3 : off_num;
		pre_off = -1; pre_len = -1; left_len = unit;
		for (j=0; j<off_num; j+=3) {
			if (rand()%10 != 1 && pre_off != -1) {
				pre_off = pre_off + pre_len;
				if (pre_off >= off_range) pre_off = off_range - 1;
				pre_len = rand() % 32  + 1;
				left_len -= pre_len;
				if (pre_off + pre_len > off_range) pre_len = 1;
			} else {
				pre_off = rand() % (off_range - left_len);
				pre_len = rand() % 32  + 1;
				left_len -= pre_len;
				if (pre_off + pre_len > off_range) pre_len = 1;
			}
			left_len -= (1+pre_len);
			if (left_len < 0) {
				off_num = j;
				break;
			}
			ut_bwc_cross_page_offset[i].offsets[j] = pre_off;
			ut_bwc_cross_page_offset[i].lens[j] = pre_len;

			ut_bwc_cross_page_offset[i].offsets[j+1] = pre_off + 1;
			ut_bwc_cross_page_offset[i].lens[j+1] = 2;

			ut_bwc_cross_page_offset[i].offsets[j+2] = pre_off + 1;
			ut_bwc_cross_page_offset[i].lens[j+2] = pre_len;

		}
		ut_bwc_cross_page_offset[i].offset_num = off_num;
		printf("		{\n");
		printf("				.offset_num = %d,\n"
			"				.offsets = {", off_num);
		for (j=0; j<off_num; j++) {
			printf("%d", ut_bwc_cross_page_offset[i].offsets[j]);
			if (j != off_num-1) printf(", ");
		}
		printf("},\n				.lens = {");
		for (j=0; j<off_num; j++) {
			printf("%d", ut_bwc_cross_page_offset[i].lens[j]);
			if (j != off_num-1) printf(", ");
		}
		printf("},\n");
		printf("		},\n");
	}
	printf("};\n\n");

}

static void
auto_generate_cross_page_offset2(int dataStreamMB)
{
	ut_bwc_write_total_cross_page = dataStreamMB / UT_BWC_WC_PPAGE_SZ;
	int i, j;
	int off_num;
	int cur_off = 0, write_off;
	ut_bwc_cross_page_offset = malloc(sizeof(struct ut_bwc_page_blocks) *
			ut_bwc_write_total_cross_page);

	printf("Data pattern:\nstruct ut_bwc_page_blocks 		ut_bwc_cross_page_offset_default[] = {\n");

	/* off and len are in 4K */
	for (i=0; i<ut_bwc_write_total_cross_page; i++) {
		off_num  = 3;
		for (j=0; j<off_num; j++) {
			switch (j) {
			case 0:
				write_off = cur_off;
				ut_bwc_cross_page_offset[i].offsets[j] = write_off;
				ut_bwc_cross_page_offset[i].lens[j] = 32;
				cur_off = write_off + ut_bwc_cross_page_offset[i].lens[j];
				break;

			case 1:
				write_off = cur_off;
				ut_bwc_cross_page_offset[i].offsets[j] = write_off;
				ut_bwc_cross_page_offset[i].lens[j] = 1;
				cur_off = write_off + ut_bwc_cross_page_offset[i].lens[j];
				break;

			case 2:
				write_off = cur_off - 32;
				ut_bwc_cross_page_offset[i].offsets[j] = write_off;
				ut_bwc_cross_page_offset[i].lens[j] = 32;
				cur_off = write_off + ut_bwc_cross_page_offset[i].lens[j];
				break;

			default:
				break;
			}
		}
		ut_bwc_cross_page_offset[i].offset_num = off_num;
		printf("		{\n");
		printf("				.offset_num = %d,\n"
			"				.offsets = {", off_num);
		for (j=0; j<off_num; j++) {
			printf("%d", ut_bwc_cross_page_offset[i].offsets[j]);
			if (j != off_num-1) printf(", ");
		}
		printf("},\n				.lens = {");
		for (j=0; j<off_num; j++) {
			printf("%d", ut_bwc_cross_page_offset[i].lens[j]);
			if (j != off_num-1) printf(", ");
		}
		printf("},\n");
		printf("		},\n");
	}
	printf("};\n\n");

}

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
ut_bwc_gen_write_request_node(char *buf, off_t off, uint32_t len)
{
	struct request_node *rn_p;
	uint32_t to_read, to_read2;
	char *ds_buf;

	rn_p = calloc(sizeof(*rn_p), 1);
	rn_p->r_ds = &ut_bwc_ds;
	rn_p->r_ds_index = ut_bwc_in_ds;
	rn_p->r_sac = &ut_bwc_sac;
	rn_p->r_ir.cmd = NID_REQ_WRITE;
	rn_p->r_ir.len = len;
	rn_p->r_ir.offset = off;
	rn_p->r_len = len;
	rn_p->r_offset = off;

	assert(len<=ut_bwc_ds_pagesz);

	ds_buf = ut_bwc_ds.d_op->d_get_buffer(&ut_bwc_ds, ut_bwc_in_ds, &to_read, &ut_bwc_io);
	if (to_read >= len) {
		memcpy(ds_buf, buf, len);
		ut_bwc_ds.d_op->d_confirm(&ut_bwc_ds, ut_bwc_in_ds, len);
		rn_p->r_resp_buf_1 = ds_buf;
	} else {
		memcpy(ds_buf, buf, to_read);
		ut_bwc_ds.d_op->d_confirm(&ut_bwc_ds, ut_bwc_in_ds, to_read);

		ds_buf = ut_bwc_ds.d_op->d_get_buffer(&ut_bwc_ds, ut_bwc_in_ds, &to_read2, &ut_bwc_io);
		assert(to_read2 >= len-to_read);
		memcpy(ds_buf, buf+to_read, len-to_read);
		ut_bwc_ds.d_op->d_confirm(&ut_bwc_ds, ut_bwc_in_ds, len-to_read);
	}

	rn_p->r_ir.dseq = ut_bwc_write_seq;
	rn_p->r_seq = rn_p->r_ir.dseq ;
	rn_p->r_io_handle = ut_bwc_wc_handle;
	gettimeofday(&rn_p->r_recv_time, NULL);
	ut_bwc_write_seq += len;

	return rn_p;
}

static int
ut_bwc_open_write_device(char *path)
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

static inline int
ut_bwc_get_total_request_num ()
{
	int i, j, len;
	int num = 0;
	uint64_t pglen;

	for (i=0; i<ut_bwc_write_total_cross_page; i++) {
		num += ut_bwc_cross_page_offset[i].offset_num;
		len = 0;
		for (j=0; j<ut_bwc_cross_page_offset[i].offset_num; j++) {
			len += ut_bwc_cross_page_offset[i].lens[j];
		}
		pglen = ((uint64_t)len) << 12;
		if (pglen < UT_BWC_WC_PPAGE_SZ << 20) {
			pglen = (UT_BWC_WC_PPAGE_SZ << 20) - pglen;
			num += pglen / UT_BWC_SEG_SZ;
			num += pglen % UT_BWC_SEG_SZ == 0 ? 0 : 1;
		} else if (pglen > UT_BWC_WC_PPAGE_SZ << 20) {
			assert(0);
		}
	}
	return num;
}

static inline void
ut_bwc_gen_random_data(char *buf, uint32_t len)
{
	uint32_t i;
	for (i=0; i<len; i++) {
		if ((i+1)%64 == 0) buf[i] = '\n';
		else buf[i] = 'A' + rand()%48;
	}
}

static void
ut_bwc_gen_data()
{
	int i, j, k, fd;
	size_t total = 0, len;
	time_t t;
	uint32_t pglen;
	ssize_t ret;

	time(&t);
	srand((unsigned int)t);
	fd = ut_bwc_open_write_device(ut_bwc_write_data_dev);
	k = 0;
	for (i = 0; i < ut_bwc_write_total_cross_page; i++) {
		pglen = 0;
		for (j = 0; j < ut_bwc_cross_page_offset[i].offset_num; j++) {
			ut_bwc_req[k].off = (1024 * 1024) + (((off_t)ut_bwc_cross_page_offset[i].offsets[j]) << 12);
			ut_bwc_req[k].len = ((uint32_t)ut_bwc_cross_page_offset[i].lens[j]) << 12;
			// ensure request len is bigger than 0
//			if(ut_bwc_req[k].len == 0)
//				continue;
			pglen += ut_bwc_req[k].len;
			if (!ut_bwc_req[k].buf)
				posix_memalign((void **)&ut_bwc_req[k].buf, getpagesize(), ut_bwc_req[k].len);
			ut_bwc_gen_random_data(ut_bwc_req[k].buf, ut_bwc_req[k].len);
			ret = pwrite(fd, ut_bwc_req[k].buf, ut_bwc_req[k].len, ut_bwc_req[k].off);
			assert((uint32_t)ret == ut_bwc_req[k].len);
			total += ut_bwc_req[k].len;
			k++;

		}

		// Fill page
		if (pglen < (UT_BWC_WC_PPAGE_SZ << 20)) {
			pglen = (UT_BWC_WC_PPAGE_SZ << 20) - pglen;
			while (pglen) {
				len = pglen > UT_BWC_SEG_SZ? UT_BWC_SEG_SZ : pglen;
//				assert(len);
				ut_bwc_req[k].off = 0;
				ut_bwc_req[k].len = len;
				if (!ut_bwc_req[k].buf)
					posix_memalign((void **)&ut_bwc_req[k].buf, getpagesize(), ut_bwc_req[k].len);
				ut_bwc_gen_random_data(ut_bwc_req[k].buf, ut_bwc_req[k].len);
				ret = pwrite(fd, ut_bwc_req[k].buf, ut_bwc_req[k].len, ut_bwc_req[k].off);
				assert((uint32_t)ret == ut_bwc_req[k].len);
				total += ut_bwc_req[k].len;
				k++;
				pglen -= len;
			}
		}

	}

	printf("Total size: %lu\n\n", total);
	ut_bwc_flush_left = total / (UT_BWC_WC_PPAGE_SZ << 20);
	close(fd);
}


static void
ut_bwc_write_data()
{
	struct request_node *rn_p;
	struct list_head ut_write_head,
		*ut_write_head_p = &ut_write_head;
	int ut_data_counter = 0;
	int i;

	INIT_LIST_HEAD(ut_write_head_p);
	for (i=0; i < ut_bwc_write_req_num; i++) {
		rn_p = ut_bwc_gen_write_request_node(ut_bwc_req[i].buf, ut_bwc_req[i].off, ut_bwc_req[i].len);
		list_add_tail(&rn_p->r_write_list, ut_write_head_p);
		ut_data_counter++;
		__sync_add_and_fetch(&ut_bwc_respnose_left, 1);
	}
	ut_bwc_p->bw_op->bw_write_list(ut_bwc_p, ut_bwc_handle, ut_write_head_p, ut_data_counter);
}

#if 1
static void
ut_validate_write_data()
{
	int i, fd, fd1, rc;
	char buf[UT_BWC_MAX_BK_LEN*4096], buf1[UT_BWC_MAX_BK_LEN*4096];
	fd = ut_bwc_open_write_device(UT_BWC_DRW_DEVICE);
	fd1 = ut_bwc_open_write_device(ut_bwc_write_data_dev);

	for (i=0; i<ut_bwc_write_req_num; i++) {
		memset(buf, 0, sizeof(buf));
		memset(buf1, 0, sizeof(buf1));
		rc = lseek(fd, ut_bwc_req[i].off,  SEEK_SET);
		if(rc == -1) {
			printf("error in seek %s, errno=%d\n", UT_BWC_DRW_DEVICE, errno);
			assert(0);
		}
		rc = read(fd, buf, ut_bwc_req[i].len);
		if(rc == -1) {
			printf("error in read %s, errno=%d\n", UT_BWC_DRW_DEVICE, errno);
			assert(0);
		}

		lseek(fd1, ut_bwc_req[i].off,  SEEK_SET);
		read(fd1, buf1, ut_bwc_req[i].len);
		printf("Compare index:%d, compare offset:%ld, compare len:%u...",
				i, ut_bwc_req[i].off, ut_bwc_req[i].len);
		fflush(stdout);
		assert(memcmp(buf, buf1, ut_bwc_req[i].len) == 0);
		free(ut_bwc_req[i].buf);
		printf("Success.\n");
	}
	close(fd);
	close(fd1);
	free(ut_bwc_req);

}

#else

static void
ut_validate_write_data()
{
	int i, fd, fd1;

	ssize_t ret;

	fd = ut_bwc_open_write_device(UT_BWC_DRW_DEVICE);
	fd1 = ut_bwc_open_write_device(ut_bwc_write_data_dev);

	for (i=0; i<ut_bwc_write_req_num; i++) {
		ret = pread(fd, ut_bwc_validate_buf, ut_bwc_req[i].len, ut_bwc_req[i].off);
		assert((uint32_t)ret == ut_bwc_req[i].len);
		ret = pread(fd1, ut_bwc_validate_buf1, ut_bwc_req[i].len, ut_bwc_req[i].off);
		assert((uint32_t)ret == ut_bwc_req[i].len);

		printf("Compare index:%d, compare offset:%ld, compare len:%u...",
				i, ut_bwc_req[i].off, ut_bwc_req[i].len);
		fflush(stdout);
		assert(memcmp(ut_bwc_validate_buf, ut_bwc_validate_buf1, ut_bwc_req[i].len) == 0);
		free(ut_bwc_req[i].buf);
		printf("Success.\n");
	}
	close(fd);
	close(fd1);
	free(ut_bwc_req);

}
#endif

struct ut_wc_private {
	char				p_name[NID_MAX_UUID];
	int				p_type;
	void				*p_real_wc;
	struct allocator_interface	*p_allocator;
	int				p_recovered;
};

static void
ut_bwc_init_env ()
{
	char cmd[128];
	sprintf(cmd, "rm %s; touch %s;", ut_bwc_write_data_dev, ut_bwc_write_data_dev);
	system(cmd);

	check_device(UT_BWC_READ_DEVICE);
	check_device(UT_BWC_WRITE_DEVICE);

	struct allocator_setup ut_bwc_al_setup;
	ut_bwc_al_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(&ut_bwc_al, &ut_bwc_al_setup);

	struct fpc_setup fpc_setup;
	fpc_setup.fpc_algrm = FPC_SHA256;
	fpc_initialization(&ut_bwc_fpc, &fpc_setup);

	struct fpn_setup fpn_setup;
	fpn_setup.allocator = &ut_bwc_al;
	fpn_setup.fp_size = 32;
	fpn_setup.set_id = ALLOCATOR_SET_SCG_FPN;
	fpn_setup.seg_size = 4096;
	fpn_initialization(&ut_bwc_fpn, &fpn_setup);
	fpn_initialization(&ut_bwc_fpn_data, &fpn_setup);

	struct srn_setup srn_setup;
	srn_setup.allocator = &ut_bwc_al;
	srn_setup.set_id = ALLOCATOR_SET_SCG_SRN;
	srn_setup.seg_size = 1024;
	srn_initialization(&ut_bwc_srn, &srn_setup);

	struct pp_setup rc_pp_setup;
	rc_pp_setup.pool_size = 160;
	rc_pp_setup.page_size = 4;
	rc_pp_setup.allocator =  &ut_bwc_al;
	rc_pp_setup.set_id = ALLOCATOR_SET_SCG_PP;
	strcpy(rc_pp_setup.pp_name, "RC_TEST_PP");
	pp_initialization(&ut_bwc_rc_pp, &rc_pp_setup);

	struct pp_setup wc_pp_setup;
	wc_pp_setup.pool_size = 1024;
	wc_pp_setup.page_size = UT_BWC_WC_PPAGE_SZ;
	wc_pp_setup.allocator =  &ut_bwc_al;
	wc_pp_setup.set_id = ALLOCATOR_SET_SCG_PP;
	strcpy(wc_pp_setup.pp_name, "WC_TEST_PP");
	pp_initialization(&ut_bwc_wc_pp, &wc_pp_setup);

	struct brn_setup brn_setup;
	brn_setup.allocator = &ut_bwc_al;
	brn_setup.set_id = ALLOCATOR_SET_SCG_BRN;
	brn_setup.seg_size = 4096;
	brn_initialization(&ut_bwc_brn, &brn_setup);

	struct rc_setup rc_setup;
	rc_setup.allocator = &ut_bwc_al;
	rc_setup.brn = &ut_bwc_brn;
	rc_setup.cachedev = UT_BWC_READ_DEVICE;

	// magneto use MB instead of GB as size unit
	rc_setup.cachedevsz = 4096;
	rc_setup.fpn = &ut_bwc_fpn;
	rc_setup.pp = &ut_bwc_rc_pp;
	rc_setup.srn = &ut_bwc_srn;
	rc_setup.type = RC_TYPE_CAS;
	rc_setup.uuid = "123456";
	rc_initialization(&ut_bwc_rc, &rc_setup);

	ut_bwc_rc.rc_op->rc_set_recover_state(&ut_bwc_rc, RC_RECOVER_DOING);
	ut_bwc_rc.rc_op->rc_recover(&ut_bwc_rc);
	ut_bwc_rc.rc_op->rc_set_recover_state(&ut_bwc_rc, RC_RECOVER_DONE);

	struct rw_setup rw_setup;
	rw_setup.allocator = &ut_bwc_al;
	rw_setup.fpn_p = &ut_bwc_fpn;
	rw_setup.type = RW_TYPE_DEVICE;
	rw_setup.name = "RW_UT";
	rw_setup.exportname = UT_BWC_DRW_DEVICE;
	rw_setup.simulate_async = 0;
	rw_initialization(&ut_bwc_rw, &rw_setup);
	ut_bwc_rw_handle = ut_bwc_rw.rw_op->rw_create_worker(&ut_bwc_rw, UT_BWC_DRW_DEVICE, 1, 1, 0, 4096);
	ut_bwc_rw.rw_op->rw_do_device_ready(&ut_bwc_rw);

	struct rc_wcn_setup rmsetup;
	rmsetup.allocator = &ut_bwc_al;
	rmsetup.seg_size = 16;
	rmsetup.set_id = ALLOCATOR_SET_BWC_RCBWC;
	rc_wcn_initialization(&ut_bwc_bwcn, &rmsetup);

	struct rc_wc_cbn_setup rmc_setup;
	rmc_setup.allocator = &ut_bwc_al;
	rmc_setup.seg_size = 128;
	rmc_setup.set_id = ALLOCATOR_SET_BWC_RCBWC_CB;
	rc_wc_cbn_initialization(&ut_bwc_bwc_cbn, &rmc_setup);

	struct rc_channel_info cinfo;
	cinfo.rc_rw = &ut_bwc_rw;

	int new_worker;
	ut_bwc_rc_handle = ut_bwc_rc.rc_op->rc_create_channel(&ut_bwc_rc, NULL, &cinfo, UT_BWC_READ_DEVICE, &new_worker);

	struct pp2_setup pp2_setup;
	struct pp2_interface *pp2_p;
	pp2_p = calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "server_pp2";
	pp2_setup.allocator = &ut_bwc_al;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_PP2;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 16;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 1;
	pp2_initialization(pp2_p, &pp2_setup);

	struct tp_setup tp_setup;
	tp_setup.pp2 = pp2_p;
	tp_setup.delay = 0;
	tp_setup.extend = 2;
	tp_setup.min_workers = 4;
	tp_setup.max_workers = 8;
	strcpy(tp_setup.name, "BWC_UT3_TP");
	tp_initialization(&ut_bwc_tp, &tp_setup);

	sac_initialization(&ut_bwc_sac, NULL);

	struct lstn_setup lstn_setup;
	lstn_setup.allocator = &ut_bwc_al;
	lstn_setup.seg_size = 128;
	lstn_setup.set_id = 0;
	lstn_initialization(&ut_bwc_lstn, &lstn_setup);

	struct wc_setup wc_setup;
	wc_setup.allocator = &ut_bwc_al;
	wc_setup.bfp_type = BFP_POLICY_BFP1;
	wc_setup.bufdevice = UT_BWC_WRITE_DEVICE;
	wc_setup.bufdevicesz = 4096;
	wc_setup.coalesce_ratio = 0.0;
	wc_setup.do_fp = 1;
	wc_setup.lstn = &ut_bwc_lstn;
	wc_setup.pp = &ut_bwc_wc_pp;
	wc_setup.rw_sync = 0;
	wc_setup.srn = &ut_bwc_srn;
	wc_setup.tp = &ut_bwc_tp;
	wc_setup.two_step_read = 0;
	wc_setup.type = WC_TYPE_NONE_MEMORY;
	wc_setup.uuid = "BWC_UT3";
	wc_setup.write_delay_first_level = 300;
	wc_setup.write_delay_second_level = 100;
	wc_setup.max_flush_size = 2;
	wc_initialization(&ut_bwc_wc, &wc_setup);
	ut_bwc_wc.wc_op->wc_post_initialization(&ut_bwc_wc);

	struct wc_channel_info wc_info;
	wc_info.wc_rw = &ut_bwc_rw;
	wc_info.wc_rc = &ut_bwc_rc;
	wc_info.wc_rc_handle = ut_bwc_rc_handle;
	wc_info.wc_rw_exportname = "/dev/sdc";
	ut_bwc_wc.wc_op->wc_recover(&ut_bwc_wc);
	ut_bwc_wc_handle = ut_bwc_wc.wc_op->wc_create_channel(&ut_bwc_wc, &ut_bwc_sac, &wc_info, BWC_UT_BWC_UUID, &new_worker);

	ut_bwc_p = ((struct ut_wc_private*)ut_bwc_wc.wc_private)->p_real_wc;
	ut_bwc_handle = ut_bwc_wc_handle;

	struct ds_setup ds_setup;
	ds_setup.name = "UT_DS";
	ds_setup.pagenrshift = 2;
	ds_setup.pageszshift = 23;
	ds_setup.pool = &ut_bwc_wc_pp;
	ds_setup.rc_name = rc_setup.uuid;
	ds_setup.type = DS_TYPE_SPLIT;
	ds_setup.wc_name = wc_setup.uuid;
	ds_initialization(&ut_bwc_ds, &ds_setup);

	struct io_setup io_setup;
	io_setup.ini = NULL;
	io_setup.io_type = IO_TYPE_BUFFER;
	io_setup.pool =  &ut_bwc_wc_pp;
	io_setup.tp = &ut_bwc_tp;
	io_initialization(&ut_bwc_io, &io_setup);

	struct io_channel_info io_info;
	io_info.io_rc = &ut_bwc_rc;
	io_info.io_rc_handle = ut_bwc_rc_handle;
	io_info.io_rw = &ut_bwc_rw;
	io_info.io_rw_handle = ut_bwc_rw_handle;
	io_info.io_wc = &ut_bwc_wc;
	io_info.io_wc_handle = ut_bwc_wc_handle;

	ut_bwc_io_handle = ut_bwc_io.io_op->io_create_worker(&ut_bwc_io, NULL, &io_info, UT_BWC_DRW_DEVICE, &new_worker);

	struct ds_interface *ds_p = &ut_bwc_ds;
	ut_bwc_in_ds = ds_p->d_op->d_create_worker(ds_p, ut_bwc_io_handle, 1, &ut_bwc_io);
	if (ut_bwc_in_ds < 0) {
		assert(0);
	}
	ut_bwc_out_ds = ds_p->d_op->d_create_worker(ds_p, ut_bwc_io_handle, 0, &ut_bwc_io);
	if (ut_bwc_out_ds < 0) {
		assert(0);
	}
	ut_bwc_ds_pagesz = ds_p->d_op->d_get_pagesz(ds_p, ut_bwc_out_ds);

	ut_bwc_write_req_num = ut_bwc_get_total_request_num();
	ut_bwc_req = calloc(sizeof(*ut_bwc_req), ut_bwc_write_req_max_num);
	printf("Total request number: %d\n", ut_bwc_write_req_num); fflush(stdout);
	posix_memalign((void **)&ut_bwc_validate_buf, getpagesize(), UT_BWC_WC_PPAGE_SZ << 20);
	posix_memalign((void **)&ut_bwc_validate_buf1, getpagesize(), UT_BWC_WC_PPAGE_SZ << 20);
}

int
main()
{
	nid_log_open();
	nid_log_set_level(7);
	printf("bwc ut module start ...\n");

	printf("bwc ut generate random offset table ...\n"); fflush(stdout);
	int pattern_option = 0;
	int generateDataLenMB = 100;
	switch (pattern_option) {
	case 0:
		ut_bwc_write_total_cross_page = generateDataLenMB / UT_BWC_WC_PPAGE_SZ;
		ut_bwc_cross_page_offset = ut_bwc_cross_page_offset_default2;
		break;

	case 1:
		auto_generate_cross_page_offset(generateDataLenMB, 1024);
		break;

	case 2:
		auto_generate_cross_page_offset2(generateDataLenMB);
		break;

	default:
		printf("invalid pattern option, quit ut ...\n"); fflush(stdout);
		goto out;
	}

	printf("bwc ut init ENV ...\n"); fflush(stdout);

	nid_log_open();
	ut_bwc_init_env();

	printf("bwc ut init DATA ...\n"); fflush(stdout);
	ut_bwc_gen_data();

	printf("bwc ut write DATA ...\n"); fflush(stdout);
	int i;
	for (i=0; i < UT_BWC_REPEAT_WRITE_TIME; i++) {
		ut_bwc_write_data();
		usleep(1000000);
	}

	while(ut_bwc_respnose_left) {
		printf("Waiting response & flush!! ut_bwc_respnose_left:%d\n\n", ut_bwc_respnose_left);
		sleep(1);
	}
	printf("flush to wc done\n");
	fflush(stdout);

	ut_bwc_wc.wc_op->wc_fast_flush(&ut_bwc_wc, NULL);
	printf("flush to disk start\n");
	while(1) {
		uint32_t noccuipied = ut_bwc_wc.wc_op->wc_get_block_occupied(&ut_bwc_wc);
		sleep(1);
		printf("block occupied %u\n", noccuipied);
		fflush(stdout);
		if(noccuipied == 0)
			break;
	}
	printf("flush to disk done\n");

	printf("bwc ut validate DATA ...\n"); fflush(stdout);
	ut_validate_write_data();

out:
	printf("\nbwc ut module end...\n"); fflush(stdout);
	nid_log_close();

	return 0;
}
