
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
#include <openssl/sha.h>

#include "list.h"
#include "fpn_if.h"
#include "fpc_if.h"
#include "allocator_if.h"
#include "fpn_if.h"
#include "list.h"
#include "nid.h"
#include "hash_if.h"
#include "server.h"

#define DEFAULT_FPC_FILE_SZ (1024*1024*1024)
size_t FPC_FILE_SZ = DEFAULT_FPC_FILE_SZ;
#define FPC_FILE_PATH1 "/tmp/fpcdataperformance"

static char *fpc_ut_buffer1;

static struct fpc_interface fpc;
static struct fpn_interface fpn;

static struct hash_setup hash_setup;
static struct hash_interface ut_hash;

void
hash_init_setup(struct hash_interface *p_hash, struct hash_setup *p_setup) {
	memset(p_hash, 0, sizeof(*p_hash));
	memset(p_setup, 0, sizeof(*p_setup));
}


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
    for (i=0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", fp[i]);
    }
}

int main(void) {
	struct fpn_setup fpn_setup;
	struct fpc_setup setup;
	struct allocator_interface al;
	struct allocator_setup al_setup;
	struct hash_interface *hash_p = &ut_hash;
	struct list_head fpn_head1;
	struct ini_setup ini_setup;
	struct ini_interface *ini_p;
	struct ini_section_content *global_sect;

	INIT_LIST_HEAD(&fpn_head1);
	al_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(&al, &al_setup);

	setup.fpc_algrm = FPC_SHA256;
	fpc_initialization(&fpc, &setup);

	fpn_setup.allocator = &al;
	fpn_setup.fp_size = 32;
	fpn_setup.seg_size = 512;
	fpn_setup.set_id = 0;
	fpn_initialization(&fpn, &fpn_setup);

	memset(&ini_setup, 0, sizeof(ini_setup));
	ini_setup.path = NID_CONF_SERVICE;
	ini_setup.keys_desc = server_keys;
	ini_p = calloc(1, sizeof(*ini_p));
	ini_initialization(ini_p, &ini_setup);
	ini_p->i_op->i_parse(ini_p);
	global_sect = ini_p->i_op->i_search_section(ini_p, "global");
	assert(global_sect);

	hash_init_setup(&ut_hash, &hash_setup);
	hash_setup.ini = &ini_p;
	hash_setup.hash_mod = 1024;
	hash_initialization(&ut_hash, &hash_setup);

	initData();

	printf("\n");

	fpc.fpc_op->fpc_bcalculate(&fpc, &fpn, fpc_ut_buffer1, (size_t)FPC_FILE_SZ, 4096, &fpn_head1);

	struct fp_node *fpp;
	uint8_t fp_len = fpc.fpc_op->fpc_get_fp_len(&fpc);
	list_for_each_entry(fpp, struct fp_node, &fpn_head1, fp_list) {
    		hash_p->s_op->hash_calc(hash_p, (uint8_t *)fpp->fp, fp_len);
	}

	// get hash performance
	nid_log_set_level(7);
	hash_p->s_op->hash_perf(&ut_hash);

	printf("Test success!!!\n");

	return EXIT_SUCCESS;
}
