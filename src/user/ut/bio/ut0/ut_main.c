#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "bio_if.h"
#include "bse_if.h"
#include "bfp_if.h"
#include "bfe_if.h"
#include "bre_if.h"
#include "ddn_if.h"
#include "srn_if.h"
#include "wc_if.h"

static struct bio_setup bio_setup;
static struct bio_interface ut_bio;

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
bfp_initialization(struct bfp_interface *bfp_p, struct bfp_setup *setup)
{
	assert(bfp_p && setup);
	return 0;
}

int
bfe_initialization(struct bfe_interface *bfe_p, struct bfe_setup *setup)
{
	assert(bfe_p && setup);
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

int
wc_initialization(struct wc_interface *wc_p, struct wc_setup *setup)
{
	assert(wc_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("bio ut module start ...");

	bio_initialization(&ut_bio, &bio_setup);

	nid_log_info("bio ut module end...");
	nid_log_close();

	return 0;
}
