
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
#include <sys/time.h>    // for gettimeofday()

#include "list.h"
#include "fpn_if.h"
#include "fpc_if.h"
#include "allocator_if.h"


#define DEFAULT_FPC_FILE_SZ (1024*1024*128)
size_t FPC_FILE_SZ = DEFAULT_FPC_FILE_SZ;
#define FPC_FILE_PATH1 "/tmp/fpcdataperformance"

static char *fpc_ut_buffer1;

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
    generate_file(FPC_FILE_PATH1, FPC_FILE_SZ);
    fpc_ut_buffer1 = malloc(FPC_FILE_SZ);
    load_data(FPC_FILE_PATH1, fpc_ut_buffer1, FPC_FILE_SZ);
}

void printfp_string(char *fp) {
    int i;
    for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", fp[i]);
    }
}

int main(void) {
	struct fpn_setup fpn_setup;
	struct fpc_setup setup;
	struct allocator_interface al;
	struct allocator_setup al_setup;

	struct list_head fpn_head1;

	INIT_LIST_HEAD(&fpn_head1);

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

	struct timeval start, end;
	gettimeofday( &start, NULL );
	printf("start : %ld.%ld\nTotal calculate fps:%d\n", start.tv_sec, start.tv_usec, DEFAULT_FPC_FILE_SZ/4096);

	fpc.fpc_op->fpc_bcalculate(&fpc, &fpn, fpc_ut_buffer1, (size_t)FPC_FILE_SZ, 4096, &fpn_head1);

	gettimeofday( &end, NULL );
	printf("end   : %ld.%ld\n", end.tv_sec, end.tv_usec);

	long int costs, costus;
	costus = end.tv_usec - start.tv_usec;
	costs = costus < 0 ? end.tv_sec-start.tv_sec - 1: end.tv_sec-start.tv_sec;
	costus = costus < 0 ? end.tv_usec + 1000000 - start.tv_usec: costus;
	printf("Cost: %ld.%ld\n", costs, costus);

	printf("Test success!!!\n");

	return EXIT_SUCCESS;
}
