#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "bwc_if.h"
#include "bse_if.h"
#include "bfp_if.h"
#include "bfe_if.h"
#include "bre_if.h"
#include "ddn_if.h"
#include "srn_if.h"
#include "drw_if.h"
#include "allocator_if.h"
#include "pp_if.h"

#define UT_BWC_WC_PPAGE_SZ         8


static struct bwc_setup bwc_setup;
static struct bwc_interface ut_bwc;

struct rc_wc_cbn_interface;
struct rc_wc_cbn_setup;
int
rc_wc_cbn_initialization(struct rc_wc_cbn_interface *rc_bwc_cbn_p, struct rc_wc_cbn_setup *setup)
{
	assert(rc_bwc_cbn_p && setup);
	return 0;
}

struct rc_wcn_interface;
struct rc_wcn_setup;
int
rc_wcn_initialization(struct rc_wcn_interface *rc_bwcn_p, struct rc_wcn_setup *setup)
{
	assert(rc_bwcn_p && setup);
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
bfp_initialization(struct bfp_interface *bfp_p, struct bfp_setup *setup)
{
	assert(bfp_p && setup);
	return 0;
}

struct bfp2_interface;
struct bfp2_setup;
int
bfp2_initialization(struct bfp2_interface *bfp2_p, struct bfp2_setup *setup)
{
	assert(bfp2_p && setup);
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

struct node_interface;
struct node_setup;
int
node_initialization(struct node_interface *node_p, struct node_setup *setup)
{
	assert(node_p && setup);
	return 0;
}

int
drw_initialization(struct drw_interface *drw_p, struct drw_setup *setup)
{
	assert(drw_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("bwc ut module start ...");

	struct allocator_interface ut_bwc_al;
	struct allocator_setup ut_bwc_al_setup;
	ut_bwc_al_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(&ut_bwc_al, &ut_bwc_al_setup);

	struct pp_interface ut_bwc_wc_pp;
	struct pp_setup wc_pp_setup;
	wc_pp_setup.pool_size = 1024;
	wc_pp_setup.page_size = UT_BWC_WC_PPAGE_SZ;
	wc_pp_setup.allocator =  &ut_bwc_al;
	wc_pp_setup.set_id = ALLOCATOR_SET_SCG_PP;
	strcpy(wc_pp_setup.pp_name, "WC_TEST_PP");
	pp_initialization(&ut_bwc_wc_pp, &wc_pp_setup);
	bwc_setup.pp = &ut_bwc_wc_pp;
	bwc_setup.write_delay_first_level = 300;
	bwc_setup.write_delay_second_level = 100;

	bwc_initialization(&ut_bwc, &bwc_setup);

	nid_log_info("bwc ut module end...");
	nid_log_close();

	return 0;
}
