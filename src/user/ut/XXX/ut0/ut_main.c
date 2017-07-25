#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "XXX_if.h"

static struct XXX_setup XXX_setup;
static struct XXX_interface ut_XXX;

int
main()
{
	nid_log_open();
	nid_log_info("XXX ut module start ...");

	XXX_initialization(&ut_XXX, &XXX_setup);

	nid_log_info("XXX ut module end...");
	nid_log_close();

	return 0;
}
