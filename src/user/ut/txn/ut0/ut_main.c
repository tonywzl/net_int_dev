#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "txn_if.h"

static struct txn_setup txn_setup;
static struct txn_interface ut_txn;

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
	nid_log_info("txn ut module start ...");

	txn_initialization(&ut_txn, &txn_setup);

	nid_log_info("txn ut module end...");
	nid_log_close();

	return 0;
}
