#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rtn_if.h"

static struct rtn_setup rtn_setup;
static struct rtn_interface ut_rtn;

int
main()
{
	struct rtree_node *np[1025];
	int i;

	nid_log_open();
	nid_log_info("rtn ut module start ...");

	rtn_initialization(&ut_rtn, &rtn_setup);

	for (i = 0; i < 1025; i++) {
		np[i] = ut_rtn.rt_op->rt_get_node(&ut_rtn);
		assert(np[i]);
	}
	for (i = 0; i < 1025; i++) {
		ut_rtn.rt_op->rt_put_node(&ut_rtn, np[i]);
	}

	nid_log_info("rtn ut module end...");
	nid_log_close();

	return 0;
}
