#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "allocator_if.h"

static struct allocator_setup allocator_setup;
static struct allocator_interface ut_allocator;

struct lck_interface;
struct lck_setup;
struct lck_node;

int
lck_initialization(struct lck_interface *lck_p, struct lck_setup *setup)
{
	assert(lck_p && setup);
	return 0;
}

int
lck_node_init(struct lck_node *ln_p)
{
	assert(ln_p);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("allocator ut module start ...");

	allocator_initialization(&ut_allocator, &allocator_setup);

	nid_log_info("allocator ut module end...");
	nid_log_close();

	return 0;
}
