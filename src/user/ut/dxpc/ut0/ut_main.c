#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dxpc_if.h"

static struct dxpc_interface ut_dxpc;

struct umessage_tx_hdr;
char* decode_tx_hdr(char *p, uint32_t *len, struct umessage_tx_hdr *msghdr_p)
{
	assert(p && len && msghdr_p);
	return NULL;
}

int util_nw_read_n(int fd, void *buf, int n)
{
	assert(fd && buf && n);
	return 0;
}


int
main()
{
	struct hs_interface;
	struct dxpc_setup setup;
	
	nid_log_open();
	nid_log_info("dxpc start ...");

	memset(&setup, 0, sizeof(setup));
	dxpc_initialization(&ut_dxpc, &setup);

	nid_log_info("dxpc end...");
	nid_log_close();

	return 0;
}
