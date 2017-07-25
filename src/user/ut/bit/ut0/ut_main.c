#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bit_if.h"

static struct bit_setup bit_setup;
static struct bit_interface ut_bit;

int
main()
{
	nid_log_open();
	nid_log_info("bit ut module start ...");

	bit_initialization(&ut_bit, &bit_setup);

	nid_log_info("bit ut module end...");
	nid_log_close();

	return 0;
}
