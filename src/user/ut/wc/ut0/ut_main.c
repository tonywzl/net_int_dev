#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bwc_if.h"
#include "wc_if.h"

static struct wc_setup wc_setup;
static struct wc_interface ut_wc;

int
bwc_initialization(struct bwc_interface *bwc_p, struct bwc_setup *setup)
{
	assert(bwc_p && setup);
	return 0;
}

struct bwc_interface;
struct bwc_setup;
int
bwc_initialization(struct bwc_interface *bwc_p, struct bwc_setup *setup)
{
	assert(bwc_p && setup);
	return 0;
}
struct twc_interface;
struct twc_setup;
int
twc_initialization(struct twc_interface *twc_p, struct twc_setup *setup)
{
	assert(twc_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("wc ut module start ...");

	wc_initialization(&ut_wc, &wc_setup);

	nid_log_info("wc ut module end...");
	nid_log_close();

	return 0;
}
