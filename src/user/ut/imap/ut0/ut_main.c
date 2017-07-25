#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "imap_if.h"

static struct imap_setup imap_setup;
static struct imap_interface ut_imap;

int
main()
{
	nid_log_open();
	nid_log_info("imap ut module start ...");

	imap_initialization(&ut_imap, &imap_setup);

	nid_log_info("imap ut module end...");
	nid_log_close();

	return 0;
}
