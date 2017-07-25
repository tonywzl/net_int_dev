
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

#include "list.h"
#include "fpn_if.h"
#include "fpc_if.h"
#include "allocator_if.h"


#define DEFAULT_FPC_FILE_SZ (1024*1024*100)
size_t FPC_FILE_SZ = DEFAULT_FPC_FILE_SZ;
#define FPC_FILE_PATH1 "/tmp/fpcdata1"
#define FPC_FILE_PATH2 "/tmp/fpcdata2"

static char *fpc_ut_buffer1;
static char *fpc_ut_buffer2;

static struct fpc_interface fpc;
static struct fpn_interface fpn;

static int
open_file(char *path)
{
    mode_t mode;
    int flags;

    mode = 0x644;
    flags = O_RDWR;
    flags |= O_SYNC;
    return open(path, flags, mode);
}

static void
load_data(char *path, char*buffer, size_t len) {
    int fd = open_file(path);
    read(fd, buffer, len);
    close(fd);
}

static int
file_exist(char *path) {
    struct stat fstat;
    int ret = stat(path, &fstat);
    if (ret == 0) {
        FPC_FILE_SZ = fstat.st_size;
    }
    return ret == -1 ? 0 : 1;
}

static void
generate_file(char *path, long filesz)
{
    char cmd[1024];
    long blksz = filesz >> 12;
    if (!file_exist(path)) {
        // File not exist, create one.
        sprintf(cmd, "dd if=/dev/urandom of=%s bs=4096 count=%ld oflag=sync",
                path, blksz);
        system(cmd);
    }
}

void
initData() {
    char cmd[1024];
    generate_file(FPC_FILE_PATH1, FPC_FILE_SZ);
    if (!file_exist(FPC_FILE_PATH2)) {
        sprintf(cmd, "cp %s %s",
                FPC_FILE_PATH1, FPC_FILE_PATH2);
        system(cmd);
    }
    fpc_ut_buffer1 = malloc(FPC_FILE_SZ);
    fpc_ut_buffer2 = malloc(FPC_FILE_SZ);
    load_data(FPC_FILE_PATH1, fpc_ut_buffer1, FPC_FILE_SZ);
    load_data(FPC_FILE_PATH2, fpc_ut_buffer2, FPC_FILE_SZ);
}

struct not_match_fp_nodes {
    struct list_head    list;
    int                 offset;
    char                fp1[32];
    char                fp2[32];
};

void printfp_string(char *fp) {
    int i;
    for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", fp[i]);
    }
}

static void
fpc_convert(void *data,
    ssize_t len, struct list_head *ag_fp_head)
{
    int iov_counter = len >> 12;
    struct iovec iov[iov_counter];
    int i;
    char *databuf;

    databuf = (char *)data;
    assert((len & (4096-1)) == 0);
    printf("iov_counter : %d \n", iov_counter);
    for (i=0; i < iov_counter; i++) {
        iov[i].iov_base = databuf;
        iov[i].iov_len = 4096;
        databuf += 4096;
    }
    fpc.fpc_op->fpc_bcalculatev(&fpc, &fpn, iov, iov_counter,  4096, ag_fp_head);
}

int main(void) {
	struct fpn_setup fpn_setup;
	struct fpc_setup setup;
	struct allocator_interface al;
	struct allocator_setup al_setup;
	int i, j;

	struct list_head fpn_head1;
	struct list_head fpn_head2;
	struct list_head fpn_not_match;
	struct fp_node *fpnd1, *fpnd2;
	struct not_match_fp_nodes *nmfn;

	INIT_LIST_HEAD(&fpn_head1);
	INIT_LIST_HEAD(&fpn_head2);
	INIT_LIST_HEAD(&fpn_not_match);

	allocator_initialization(&al, &al_setup);

	setup.fpc_algrm = FPC_SHA256;
	fpc_initialization(&fpc, &setup);

	fpn_setup.allocator = &al;
	fpn_setup.fp_size = 32;
	fpn_setup.seg_size = 512;
	fpn_setup.set_id = 0;
	fpn_initialization(&fpn, &fpn_setup);

	initData();

	printf("\n");

	fpc.fpc_op->fpc_bcalculate(&fpc, &fpn, fpc_ut_buffer1, (size_t)FPC_FILE_SZ, 4096, &fpn_head1);
	fpc_convert(fpc_ut_buffer2, FPC_FILE_SZ, &fpn_head2);

	j = 0;
	while (!list_empty(&fpn_head1)) {
	    fpnd1 = list_first_entry(&fpn_head1, struct fp_node, fp_list);
	    fpnd2 = list_first_entry(&fpn_head2, struct fp_node, fp_list);
	    for (i=0; i<32; i++) {
	        if (fpnd1->fp[i] != fpnd2->fp[i]) {
	            nmfn = malloc(sizeof(*nmfn));
	            nmfn->offset = j;
	            memcpy(nmfn->fp1, fpnd1->fp, 32);
	            memcpy(nmfn->fp2, fpnd2->fp, 32);
	            list_add_tail(&nmfn->list, &fpn_not_match);
	            break;
	        }
	    }
	    list_del(&fpnd1->fp_list);
	    list_del(&fpnd2->fp_list);
	    fpn.fp_op->fp_put_node(&fpn, fpnd1);
	    fpn.fp_op->fp_put_node(&fpn, fpnd2);
	    j ++;
	}

	if (list_empty(&fpn_not_match)) {
	    printf("Test success!!!\n");
	} else {
	    int failedcnt = 0;
	    printf("Test failed! Below offset fp not match:\n");
	    while (!list_empty(&fpn_not_match)) {
	        nmfn = list_first_entry(&fpn_not_match, struct not_match_fp_nodes, list);
	        printf("Not match index: %d\n", nmfn->offset);
	        printfp_string(nmfn->fp1);
	        printf("\n");
	        printfp_string(nmfn->fp2);
	        printf("\n\n");
	        list_del(&nmfn->list);
	        failedcnt++;
	        free(nmfn);
	    }
	    printf("Total not match FPs:%d, Total FPs:%lu\n", failedcnt, FPC_FILE_SZ>>12);
	}
	return EXIT_SUCCESS;
}
