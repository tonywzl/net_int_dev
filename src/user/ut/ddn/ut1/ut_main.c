#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "ddn_if.h"

static struct ddn_setup ddn_setup;
static struct ddn_interface ut_ddn;

int
main()
{
	struct data_description_node *np[1025];
	int i;

	nid_log_open();
	nid_log_info("ddn ut module start ...");

	ddn_initialization(&ut_ddn, &ddn_setup);

	for (i = 0; i < 18; i++) {
		np[i] = ut_ddn.d_op->d_get_node(&ut_ddn);
		assert(np[i]);
	}
	for (i = 0; i < 18; i++) {
		ut_ddn.d_op->d_put_node(&ut_ddn, np[i]);
	}

	nid_log_info("ddn ut module end...");
	nid_log_close();

	return 0;
}
