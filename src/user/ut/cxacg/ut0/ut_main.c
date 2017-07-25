#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
#include "cxacg_if.h"

int
main()
{
	struct cxacg_setup cxacg_setup;
	struct cxacg_interface *cxacg_p;

	nid_log_open();
	nid_log_info("cxacg ut module start ...");

	memset(&cxacg_setup, 0, sizeof(cxacg_setup));
	cxacg_p = (struct cxacg_interface *)calloc(1, sizeof(*cxacg_p));

	cxacg_initialization(cxacg_p, &cxacg_setup);

	nid_log_info("cxacg ut module end...");
	nid_log_close();

	return 0;
}
