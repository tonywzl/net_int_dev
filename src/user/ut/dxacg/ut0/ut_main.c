#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
#include "dxacg_if.h"

struct umessage_tx_hdr;

char*
decode_tx_hdr(char *p, uint32_t *len, struct umessage_tx_hdr *msghdr_p)
{
        assert(p && len && msghdr_p);
        return NULL;
}

struct dxac_interface;
struct dxac_setup;

int
dxac_initialization(struct dxac_interface *dxac_p, struct dxac_setup *setup)
{
	assert(dxac_p && setup);
	return 0;
}

int
main()
{
	struct dxacg_setup dxacg_setup;
	struct dxacg_interface *dxacg_p;

	nid_log_open();
	nid_log_info("dxacg ut module start ...");

	memset(&dxacg_setup, 0, sizeof(dxacg_setup));
	dxacg_p = (struct dxacg_interface *)calloc(1, sizeof(*dxacg_p));

	dxacg_initialization(dxacg_p, &dxacg_setup);

	nid_log_info("dxacg ut module end...");
	nid_log_close();

	return 0;
}
