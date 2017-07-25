#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "nse_if.h"
#include "cse_if.h"
#include "fpn_if.h"
#include "cdn_if.h"
#include "blksn_if.h"
#include "dsmgr_if.h"
#include "dsrec_if.h"
#include "crc_if.h"

static struct crc_setup crc_setup;
static struct crc_interface ut_crc;

int
nse_initialization(struct nse_interface *nse_p, struct nse_setup *setup)
{
	assert(nse_p && setup);
	return 0;
}

int
fpn_initialization(struct fpn_interface *fpn_p, struct fpn_setup *setup)
{
	assert(fpn_p && setup);
	return 0;
}

int
cdn_initialization(struct cdn_interface *cdn_p, struct cdn_setup *setup)
{
	assert(cdn_p && setup);
	return 0;
}

int
blksn_initialization(struct blksn_interface *blksn_p, struct blksn_setup *setup)
{
	assert(blksn_p && setup);
	return 0;
}

int
cse_initialization(struct cse_interface *cse_p, struct cse_setup *setup)
{
	assert(cse_p && setup);
	return 0;
}

int
dsmgr_initialization(struct dsmgr_interface *dsmgr_p, struct dsmgr_setup *setup)
{
	assert(dsmgr_p && setup);
	return 0;
}

int
dsrec_initialization(struct dsrec_interface *dsrec_p, struct dsrec_setup *setup)
{
	assert(dsrec_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("crc ut module start ...");

	crc_initialization(&ut_crc, &crc_setup);

	nid_log_info("crc ut module end...");
	nid_log_close();

	return 0;
}
