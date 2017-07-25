#include <string.h>

#include "nid_log.h"
#include "hash_if.h"

static struct hash_setup hash_setup;
static struct hash_interface ut_hash;

void
hash_init_setup(struct hash_interface *p_hash, struct hash_setup *p_setup) {
	memset(p_hash, 0, sizeof(*p_hash));
	memset(p_setup, 0, sizeof(*p_setup));
}


int
main()
{
	nid_log_open();
	nid_log_info("hash ut module start ...");

	hash_init_setup(&ut_hash, &hash_setup);
	hash_initialization(&ut_hash, &hash_setup);

	nid_log_info("hash ut module end...");
	nid_log_close();

	return 0;
}
