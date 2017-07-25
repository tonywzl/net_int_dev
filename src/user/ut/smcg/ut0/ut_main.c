#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "smcg_if.h"

static struct smcg_setup smcg_setup;
static struct smcg_interface ut_smcg;

struct smc_interface;
struct smc_setup;
int
smc_initialization(struct smc_interface *smc_p, struct smc_setup *setup)
{
	assert(smc_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("smcg ut module start ...");

	smcg_initialization(&ut_smcg, &smcg_setup);

	nid_log_info("smcg ut module end...");
	nid_log_close();

	return 0;
}
