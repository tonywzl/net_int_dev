#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "btn_if.h"

static struct btn_setup btn_setup;
static struct btn_interface ut_btn;

struct node_interface;
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
	nid_log_info("btn ut module start ...");

	btn_initialization(&ut_btn, &btn_setup);

	nid_log_info("btn ut module end...");
	nid_log_close();

	return 0;
}
