#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rsa_if.h"

static struct rsa_setup rsa_setup;
static struct rsa_interface ut_rsa;

int
main()
{
	nid_log_open();
	nid_log_info("rsa ut module start ...");

	rsa_initialization(&ut_rsa, &rsa_setup);

	nid_log_info("rsa ut module end...");
	nid_log_close();

	return 0;
}
