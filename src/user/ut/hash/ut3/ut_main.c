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
#include "lstn_if.h"
#include "nid.h"
#include "hash_if.h"
#include "server.h"

//#define MAX_PAGE_NUM	1024*1024*1024
#define MAX_PAGE_NUM  1024*1024
#define PAGE_SIZE	4096
static struct fpc_interface fpc;
static struct fpn_interface fpn;

static struct hash_setup hash_setup;
static struct hash_interface ut_hash;

void
hash_init_setup(struct hash_interface *p_hash, struct hash_setup *p_setup) {
	memset(p_hash, 0, sizeof(*p_hash));
	memset(p_setup, 0, sizeof(*p_setup));
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
	struct list_head conf_key_set;
	struct lstn_interface *lstn_p;
	struct lstn_setup lstn_setup;
	struct list_node *conf_key_node;
	struct ini_key_desc *conf_key;

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

        /*
         * Create lstn
         */
        lstn_setup.allocator = &al;
        lstn_setup.set_id = ALLOCATOR_SET_SERVER_LSTN;
        lstn_setup.seg_size = 128;
        lstn_p = calloc(1, sizeof(*lstn_p));
        lstn_initialization(lstn_p, &lstn_setup);

	INIT_LIST_HEAD(&conf_key_set);
	server_generate_conf_tmplate(&conf_key_set, lstn_p);
        list_for_each_entry(conf_key_node, struct list_node, &conf_key_set, ln_list) {
                conf_key = (struct ini_key_desc *)conf_key_node->ln_data;
                while (conf_key->k_name) {
                        if (!conf_key->k_description) {
                                printf("key: %s has no description.\n", conf_key->k_name);
                                return 0;
                        }
                        conf_key++;
                }
        }

	memset(&ini_setup, 0, sizeof(ini_setup));
	ini_setup.path = NID_CONF_SERVICE;
	ini_setup.key_set = conf_key_set;
	ini_p = calloc(1, sizeof(*ini_p));
	ini_initialization(ini_p, &ini_setup);
	ini_p->i_op->i_parse(ini_p);
	global_sect = ini_p->i_op->i_search_section(ini_p, "global");
	assert(global_sect);

	hash_init_setup(&ut_hash, &hash_setup);
	hash_setup.ini = &ini_p;
	hash_setup.hash_mod = 1024;
	hash_initialization(&ut_hash, &hash_setup);

	char dbuf[PAGE_SIZE], fp_buf[64];
        char *ds = "/dev/urandom";
        int fd = open(ds, O_RDWR, 0x600);
        if(fd < 0) {
                printf("fail to open %s", ds);
                return -1;
        }
	uint8_t fp_len = fpc.fpc_op->fpc_get_fp_len(&fpc);
        uint32_t i;
        for(i = 0; i < MAX_PAGE_NUM; i++) {
                read(fd, dbuf, sizeof(dbuf));
		fpc.fpc_op->fpc_calculate(&fpc, dbuf, sizeof(dbuf), (unsigned char *)fp_buf);
                hash_p->s_op->hash_calc(hash_p, (const char *)fp_buf, fp_len);
                if(i%10000 == 0) {
                        printf("count %d times\n", i);
                }
        }

	// get hash performance
	nid_log_set_level(7);
	hash_p->s_op->hash_perf(&ut_hash);

	printf("Test success!!!\n");

	return EXIT_SUCCESS;
}
