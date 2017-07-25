#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bsn_if.h"

static struct bsn_setup bsn_setup;
static struct bsn_interface ut_bsn;

struct node_interface;
struct node_setup;
int
node_initialization(struct node_interface *node_p, struct node_setup *setup)
{
	assert(node_p && setup);
	return 0;
}



int
main()
{
	nid_log_open();
	nid_log_info("bsn ut module start ...");

	bsn_initialization(&ut_bsn, &bsn_setup);

	nid_log_info("bsn ut module end...");
	nid_log_close();

	return 0;
}
