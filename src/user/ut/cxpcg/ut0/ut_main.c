#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
#include "cxpcg_if.h"

int
main()
{
	struct cxpcg_setup cxpcg_setup;
	struct cxpcg_interface *cxpcg_p;

	nid_log_open();
	nid_log_info("cxpcg ut module start ...");

	memset(&cxpcg_setup, 0, sizeof(cxpcg_setup));
	cxpcg_p = (struct cxpcg_interface *)calloc(1, sizeof(*cxpcg_p));

	cxpcg_initialization(cxpcg_p, &cxpcg_setup);

	nid_log_info("cxpcg ut module end...");
	nid_log_close();

	return 0;
}
