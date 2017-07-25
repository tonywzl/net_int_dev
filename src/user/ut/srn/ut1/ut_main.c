#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "srn_if.h"

static struct srn_setup srn_setup;
static struct srn_interface ut_srn;

int
main()
{
	struct sub_request_node *np[1025];
	int i;

	nid_log_open();
	nid_log_info("srn ut module start ...");

	srn_initialization(&ut_srn, &srn_setup);

	for (i = 0; i < 1025; i++) {
		np[i] = ut_srn.sr_op->sr_get_node(&ut_srn);
		assert(np[i]);
	}
	for (i = 0; i < 1025; i++) {
		ut_srn.sr_op->sr_put_node(&ut_srn, np[i]);
	}

	nid_log_info("srn ut module end...");
	nid_log_close();

	return 0;
}
