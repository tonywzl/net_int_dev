#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "nid.h"
#include "nid_log.h"
#include "ini_if.h"
#include "hash_if.h"
#include "server.h"

static struct hash_setup hash_setup;
static struct hash_interface ut_hash;

void
hash_init_setup(struct hash_interface *p_hash, struct hash_setup *p_setup) {
	memset(p_hash, 0, sizeof(*p_hash));
	memset(p_setup, 0, sizeof(*p_setup));
}

static int
hash_test(struct hash_interface *hash_p) {
	char *ds = "/dev/urandom";
	int fd = open(ds, O_RDWR, 0x600);
	if(fd < 0) {
		nid_log_error("fail to open %s", ds);
		return -1;
	}

	uint32_t i;
	uint8_t dbuf[40];
	for(i = 0; i < 100000000; i++) {
		read(fd, dbuf, 32);
		hash_p->s_op->hash_calc(hash_p, dbuf, 32);
		if(i%10000 == 0) {
			printf("count %d times\n", i);
		}
	}

	close(fd);
	return 0;
}


int
main()
{
	struct ini_setup ini_setup;
	struct ini_interface *ini_p;
	struct ini_section_content *global_sect;

	nid_log_open();
	nid_log_info("hash bigdata ut module start ...");

	/*
	 * Create ini
	 */
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

	if(hash_test(&ut_hash) == 0) {
		nid_log_set_level(7);
		ut_hash.s_op->hash_perf(&ut_hash);
	}

	nid_log_info("hash bigdata ut module end...");
	nid_log_close();

	return 0;
}
