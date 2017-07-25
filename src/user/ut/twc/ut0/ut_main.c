#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "twc_if.h"
#include "bse_if.h"
#include "bre_if.h"
#include "ddn_if.h"
#include "srn_if.h"

static struct twc_setup twc_setup;
static struct twc_interface ut_twc;

void
twc_init_setup(struct twc_interface *p_twc, struct twc_setup *p_setup) {
	memset(p_twc, 0, sizeof(*p_twc));
	memset(p_setup, 0, sizeof(*p_setup));
	strcpy(p_setup->uuid, "uuid_twc_ut");
}

int
lck_node_init(struct lck_node *ln_p)
{
	assert(ln_p);
	return 0;
}

int
lck_initialization(struct lck_interface *lck_p, struct lck_setup *setup)
{
	assert(lck_p && setup);
	return 0;
}

struct d2an_interface;
struct d2an_setup;
int
d2an_initialization(struct d2an_interface *d2an_p, struct d2an_setup *setup)
{
	assert(d2an_p && setup);
	return 0;
}

struct d2bn_interface;
struct d2bn_setup;
int
d2bn_initialization(struct d2bn_interface *d2bn_p, struct d2bn_setup *setup)
{
	assert(d2bn_p && setup);
	return 0;
}

int
srn_initialization(struct srn_interface *srn_p, struct srn_setup *setup)
{
	assert(srn_p && setup);
	return 0;
}

int
bse_initialization(struct bse_interface *bse_p, struct bse_setup *setup)
{
	assert(bse_p && setup);
	return 0;
}

int
bre_initialization(struct bre_interface *bre_p, struct bre_setup *setup)
{
	assert(bre_p && setup);
	return 0;
}

int
ddn_initialization(struct ddn_interface *ddn_p, struct ddn_setup *setup)
{
	assert(ddn_p && setup);
	return 0;
}

struct node_interface;
struct node_setup;
int
node_initialization(struct node_interface *node_p, struct node_setup *setup)
{
	assert(node_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("twc ut module start ...");

	twc_init_setup(&ut_twc, &twc_setup);
	twc_initialization(&ut_twc, &twc_setup);

	nid_log_info("twc ut module end...");
	nid_log_close();

	return 0;
}
