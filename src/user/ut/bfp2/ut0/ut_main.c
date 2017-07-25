#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bfp2_if.h"

static struct bfp_setup bfp_setup;
static struct bfp_interface ut_bfp;

int
main()
{
	nid_log_open();
	nid_log_info("bfp ut module start ...");

	bfp2_initialization(&ut_bfp, &bfp_setup);

	nid_log_info("bfp ut module end...");
	nid_log_close();

	return 0;
}
