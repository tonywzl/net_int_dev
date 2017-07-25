#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "d2cn_if.h"

static struct d2cn_setup d2cn_setup;
static struct d2cn_interface ut_d2cn;

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
	nid_log_info("d2cn ut module start ...");

	d2cn_initialization(&ut_d2cn, &d2cn_setup);

	nid_log_info("d2cn ut module end...");
	nid_log_close();

	return 0;
}
