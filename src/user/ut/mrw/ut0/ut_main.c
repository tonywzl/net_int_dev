#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <sys/uio.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "nid_log.h"
#include "mrw_if.h"
#include "fpn_if.h"
#include "allocator_if.h"
#include "fpc_if.h"

#define DRW_TS_BLK_SZ 8192

static struct mrw_interface ut_mrw;
static void* ut_mrw_worker;
static struct fpn_interface fpn;
static struct fpc_interface fpc;
static struct allocator_interface al;

static void
generate_buffer(char *buf, long sz)
{
    long i;
    for (i=0; i<sz; i++) {
        buf[i] = rand() & 255;
    }
}

static off_t sb_offsets[] = {0, 8196, 32768, 98304, 163840, 229376,
            294912, 819200, 884736, 1605632, 2654208, 4096000};
#define sb_off_sz 12
static char sb_buf[DRW_TS_BLK_SZ * sb_off_sz];
static char sb_nwrite;
static pthread_mutex_t sb_lck;
static pthread_cond_t sb_cond;

void
prepare_env()
{
    printf("Prepare ENV...\n\n");
    ut_mrw_worker = ut_mrw.rw_op->rw_create_worker(&ut_mrw, "1", 0, 0);
    pthread_mutex_init(&sb_lck, NULL);
    pthread_cond_init(&sb_cond, NULL);
    sb_nwrite = 0;
}

////////////////////////////////SIMULATE WRITE AND READ PARTITION TABLE/////////////////////////////////////////

struct swdpt_cb_arg {
    struct rw_callback_arg  super;
    char*                   write_buf;
    int                     blk_index;
};

struct srdpt_cb_arg {
    struct rw_callback_arg  super;
    int                     blk_index;
    char                    buf[DRW_TS_BLK_SZ];
};

void simulate_read_disk_partition_table_cb (int errorcode, struct rw_callback_arg *arg)
{
    struct srdpt_cb_arg *cbr_arg = (struct srdpt_cb_arg *) arg;

    int i;
    char *buf = &sb_buf[cbr_arg->blk_index * DRW_TS_BLK_SZ];

    printf("Read offset %ld result: %d\n", sb_offsets[cbr_arg->blk_index], errorcode);
    fflush(stdout);
    assert(errorcode == 0);

    for (i=0; i<DRW_TS_BLK_SZ; i++) {
        if (cbr_arg->buf[i] != buf[i]) {
            printf("Write not match Read on offset: %ld\n", sb_offsets[cbr_arg->blk_index] + (off_t)i);
            fflush(stdout);
            assert(0);
        }
    }

    pthread_mutex_lock(&sb_lck);
    if ( --sb_nwrite == 0) {
        pthread_cond_signal(&sb_cond);
    }
    pthread_mutex_unlock(&sb_lck);
}

void simulate_write_disk_partition_table_cb (int errorcode, struct rw_callback_arg *arg)
{
    struct mrw_interface *mrwp = &ut_mrw;
    struct swdpt_cb_arg *cbw_arg = (struct swdpt_cb_arg*)arg;
    struct srdpt_cb_arg cbr_arg;
    struct fp_node *fpnd, *fpnd1;

    printf("Write offset %ld result: %d\n", sb_offsets[cbw_arg->blk_index], errorcode);
    fflush(stdout);
    assert(errorcode == 0);
    char *buf = &sb_buf[cbw_arg->blk_index * DRW_TS_BLK_SZ];
    struct list_head fp_head, fp_head1;
    int i;

    for (i=0; i<DRW_TS_BLK_SZ; i++) {
        assert(buf[i] == cbw_arg->write_buf[i]);
    }
    unsigned char fp32buf1[32], fp32buf2[32];
    fpc.fpc_op->fpc_calculate(&fpc, buf, 4096, fp32buf1);
    fpc.fpc_op->fpc_calculate(&fpc, buf, 4096, fp32buf2);
    for (i=0; i<32; i++) {
        assert(fp32buf1[i] == fp32buf2[i]);
    }

    fpc.fpc_op->fpc_bcalculate(&fpc, &fpn, buf, DRW_TS_BLK_SZ, 4096, &fp_head);
    fpc.fpc_op->fpc_bcalculate(&fpc, &fpn, buf, DRW_TS_BLK_SZ, 4096, &fp_head1);

    fpnd1 = list_entry(fp_head.next, struct fp_node, fp_list);
    list_for_each_entry(fpnd, struct fp_node, &fp_head1, fp_list) {
        for (i=0; i<32; i++) {
            assert(fpnd->fp[i] == fpnd1->fp[i]);
        }
        fpnd1 = list_entry(fpnd1->fp_list.next,  struct fp_node, fp_list);
    }

    fpnd1 = list_entry(fp_head.next, struct fp_node, fp_list);
    list_for_each_entry(fpnd, struct fp_node, &cbw_arg->super.ag_fp_head, fp_list) {
        for (i=0; i<32; i++) {
            assert(fpnd->fp[i] == fpnd1->fp[i]);
        }
        fpnd1 = list_entry(fpnd1->fp_list.next,  struct fp_node, fp_list);
    }
//    cbw_arg->super.ag_fp_head

    cbr_arg.blk_index = cbw_arg->blk_index;
    mrwp->rw_op->rw_pread_async_fp(mrwp, ut_mrw_worker, cbr_arg.buf, DRW_TS_BLK_SZ,
            sb_offsets[cbw_arg->blk_index], simulate_read_disk_partition_table_cb, &cbr_arg.super);

}


void
simulate_write_disk_partition_table()
{
    int i;
    struct mrw_interface *mrwp = &ut_mrw;
    struct iovec iov;
    struct swdpt_cb_arg arg[sb_off_sz];
    char *dbuf = sb_buf;

    pthread_mutex_lock(&sb_lck);
    sb_nwrite = 1;
    pthread_mutex_unlock(&sb_lck);

    printf ("Begin do simulate write and read partition table ... \n");
    fflush(stdout);

    for (i=0; i<sb_off_sz; i++) {
        iov.iov_base = dbuf;
        iov.iov_len = DRW_TS_BLK_SZ;

        generate_buffer(dbuf, DRW_TS_BLK_SZ);
        pthread_mutex_lock(&sb_lck);
        sb_nwrite++;
        pthread_mutex_unlock(&sb_lck);
        arg[i].blk_index = i;
        arg[i].write_buf = dbuf;
        mrwp->rw_op->rw_pwritev_async_fp(mrwp, ut_mrw_worker, &iov, 1, sb_offsets[i], simulate_write_disk_partition_table_cb, &arg[i].super);
        dbuf += DRW_TS_BLK_SZ;
    }

    pthread_mutex_lock(&sb_lck);
    if (--sb_nwrite != 0) {
        pthread_cond_wait(&sb_cond, &sb_lck);
    }
    assert(sb_nwrite == 0);
    pthread_mutex_unlock(&sb_lck);

    printf("Test for write and read partition table success!\n");
    fflush(stdout);
}

int
main()
{
    struct mrw_setup mrw_setup;
    struct fpn_setup fpn_setup;
    struct allocator_setup al_setup;
    struct fpc_setup fpc_setup;

	nid_log_open();
	nid_log_info("mrw ut module start ...");

	allocator_initialization(&al, &al_setup);

	fpc_setup.fpc_algrm = FPC_SHA256;
	fpc_initialization(&fpc, &fpc_setup);

	fpn_setup.allocator = &al;
	fpn_setup.fp_size = 32;
	fpn_setup.seg_size = 512;
	fpn_setup.set_id = 0;
	fpn_initialization(&fpn, &fpn_setup);

	mrw_setup.fpn_p = &fpn;
	mrw_initialization(&ut_mrw, &mrw_setup);

	nid_log_info("mrw ut module end...");
	nid_log_close();

	prepare_env();

	simulate_write_disk_partition_table();

	return 0;
}
