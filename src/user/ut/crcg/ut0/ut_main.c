#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "crcg_if.h"

static struct crcg_setup crcg_setup;
static struct crcg_interface ut_crcg;

int
main()
{
	nid_log_open();
	nid_log_info("crcg ut module start ...");

	crcg_initialization(&ut_crcg, &crcg_setup);

	nid_log_info("crcg ut module end...");
	nid_log_close();

	return 0;
}
