#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bfe0_if.h"

static struct bfe0_setup bfe0_setup;
static struct bfe0_interface ut_bfe0;

struct node_interface;
struct node_setup;
int
node_initialization(struct node_interface *node_p, struct node_setup *setup)
{
	assert(node_p && setup);
	return 0;
}

struct d1an_interface;
struct d1an_setup;
int
d1an_initialization(struct d1an_interface *d1an_p, struct d1an_setup *setup)
{
	assert(d1an_p && setup);
	return 0;
}

struct d2cn_interface;
struct d2cn_setup;
int
d2cn_initialization(struct d2cn_interface *d2cn_p, struct d2cn_setup *setup)
{
	assert(d2cn_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("bfe0 ut module start ...");

	bfe0_initialization(&ut_bfe0, &bfe0_setup);

	nid_log_info("bfe0 ut module end...");
	nid_log_close();

	return 0;
}
