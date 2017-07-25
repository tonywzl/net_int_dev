#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "allocator_if.h"
#include "pp2_if.h"

static struct pp2_setup pp2_setup;
static struct pp2_interface ut_pp2;
static struct allocator_setup allocator_setup;
static struct allocator_interface allocator;
int nid_log_level = LOG_WARNING;
char *log_header = "pp2 ut2";

int
main()
{
	struct pp2_interface *pp2_p;
	int i;
	char *buf;
#ifdef DEBUG_LEVEL
	nid_log_level = DEBUG_LEVEL;
#endif
	nid_log_open();
	nid_log_info("%s module start ...", log_header);

	allocator_setup.a_role = 1;
	allocator_initialization(&allocator, &allocator_setup);
	pp2_setup.allocator = &allocator;
	pp2_setup.page_size = 1;
	pp2_setup.pool_size = 4;
	pp2_setup.get_zero = 0;
	pp2_setup.put_zero = 0;
	pp2_setup.set_id = 1;
	pp2_p = &ut_pp2;
	pp2_initialization(pp2_p, &pp2_setup);

	for (i = 0; i < 120000; i++) {
		printf("Pre get: %d\n", i);
		buf = pp2_p->pp2_op->pp2_get_zero(pp2_p, 40);
		pp2_p->pp2_op->pp2_put(pp2_p, buf);
		printf("Post put: %d\n", i);
	}

	nid_log_info("%s module end...", log_header);
	nid_log_close();

	return 0;
}
