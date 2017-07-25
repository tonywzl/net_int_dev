#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "nid_log.h"
#include "dxag_if.h"

static struct dxag_setup dxag_setup;
static struct dxag_interface ut_dxag;

struct dxp_interface;
struct dxp_setup;
int
dxa_initialization(struct dxa_interface *dxa_p, struct dxa_setup *setup)
{
        assert(dxa_p && setup);
        return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("dxag ut module start ...");

	memset(&dxag_setup, 0, sizeof(dxag_setup));
	dxag_initialization(&ut_dxag, &dxag_setup);

	nid_log_info("dxag ut module end...");
	nid_log_close();

	return 0;
}
