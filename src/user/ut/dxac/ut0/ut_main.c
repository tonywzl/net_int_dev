#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dxac_if.h"

static struct dxac_interface ut_dxac;

struct umessage_tx_hdr;

char*
decode_tx_hdr(char *p, uint32_t *len, struct umessage_tx_hdr *msghdr_p)
{
        assert(p && len && msghdr_p);
        return NULL;
}


int
main()
{
	struct hs_interface;
	struct dxac_setup setup;
	
	nid_log_open();
	nid_log_info("dxac start ...");

	memset(&setup, 0, sizeof(setup));
	dxac_initialization(&ut_dxac, &setup);

	nid_log_info("dxac end...");
	nid_log_close();

	return 0;
}
