#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "wccg_if.h"

static struct wccg_setup wccg_setup;
static struct wccg_interface ut_wccg;

struct wcc_interface;
struct wcc_setup;
int
wcc_initialization(struct wcc_interface *wcc_p, struct wcc_setup *setup)
{
	assert(wcc_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("wccg ut module start ...");

	wccg_initialization(&ut_wccg, &wccg_setup);

	nid_log_info("wccg ut module end...");
	nid_log_close();

	return 0;
}
