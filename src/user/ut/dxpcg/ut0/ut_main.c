#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
#include "dxpcg_if.h"

struct umessage_tx_hdr;
char* decode_tx_hdr(char *p, uint32_t *len, struct umessage_tx_hdr *msghdr_p)
{
        assert(p && len && msghdr_p);
        return NULL;
}

struct dxpc_interface;
struct dxpc_setup;
int dxpc_initialization(struct dxpc_interface *dxpc_p, struct dxpc_setup *setup)
{
	assert(dxpc_p && setup);
	return 0;
}

int
main()
{
	struct dxpcg_setup dxpcg_setup;
	struct dxpcg_interface *dxpcg_p;

	nid_log_open();
	nid_log_info("dxpcg ut module start ...");

	memset(&dxpcg_setup, 0, sizeof(dxpcg_setup));
	dxpcg_p = (struct dxpcg_interface *)calloc(1, sizeof(*dxpcg_p));

	dxpcg_initialization(dxpcg_p, &dxpcg_setup);

	nid_log_info("dxpcg ut module end...");
	nid_log_close();

	return 0;
}
