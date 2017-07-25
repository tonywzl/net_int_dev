#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "rtree_if.h"
#include "srn_if.h"
#include "btn_if.h"
#include "nid.h"
#include "bse_if.h"

static struct bse_setup bse_setup;
static struct bse_interface ut_bse;

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
rtree_initialization(struct rtree_interface *rtree_p, struct rtree_setup *setup)
{
	assert(rtree_p && setup);
	return 0;
}

int
btn_initialization(struct btn_interface *btn_p, struct btn_setup *setup)
{
	assert(btn_p && setup);
	return 0;
}

int
srn_initialization(struct srn_interface *srn_p, struct srn_setup *setup)
{
	assert(srn_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("bse ut module start ...");

	bse_initialization(&ut_bse, &bse_setup);

	nid_log_info("bse ut module end...");
	nid_log_close();

	return 0;
}
