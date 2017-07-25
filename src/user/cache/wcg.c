/*
 * wcg.c
 * 	Implementation of write cache  Guardian  Module
 */

#include <stdlib.h>

#include "nid_log.h"
#include "twcg_if.h"
#include "bwcg_if.h"
#include "wcg_if.h"
#include "wc_if.h"

struct wcg_private {
	struct ini_interface	*p_ini;
	struct bwcg_interface	*p_bwcg;
	struct twcg_interface	*p_twcg;
};

static struct bwcg_interface *
wcg_get_bwcg(struct wcg_interface *wcg_p)
{
	struct wcg_private *priv_p = (struct wcg_private *)wcg_p->wg_private;

	return priv_p->p_bwcg;
}

static struct twcg_interface *
wcg_get_twcg(struct wcg_interface *wcg_p)
{
	struct wcg_private *priv_p = (struct wcg_private *)wcg_p->wg_private;

	return priv_p->p_twcg;
}

static struct wc_interface *
wcg_search(struct wcg_interface *wcg_p, char *wc_name)
{
	struct wcg_private *priv_p = (struct wcg_private *)wcg_p->wg_private;
	struct twcg_interface *twcg_p = priv_p->p_twcg;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	struct wc_interface *wc_p = NULL;

	if (!wc_p && bwcg_p)
		wc_p = bwcg_p->wg_op->wg_search_bwc(bwcg_p, wc_name);


	if (!wc_p && twcg_p)
		wc_p = twcg_p->wg_op->wg_search_twc(twcg_p, wc_name);

	return wc_p;
}

static struct wc_interface *
wcg_search_and_create_wc(struct wcg_interface *wcg_p, char *wc_name)
{
	struct wcg_private *priv_p = (struct wcg_private *)wcg_p->wg_private;
	struct twcg_interface *twcg_p = priv_p->p_twcg;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;
	struct wc_interface *wc_p = NULL;

	if (bwcg_p) {
		wc_p = bwcg_p->wg_op->wg_search_and_create_bwc(bwcg_p, wc_name);
		if (wc_p){
			return wc_p;
		}
	}

	if (twcg_p) {
		wc_p = twcg_p->wg_op->wg_search_and_create_twc(twcg_p, wc_name);
		if (wc_p){
			return wc_p;
		}
	}

	return NULL;
}

static void
wcg_recover_all_wc(struct wcg_interface *wcg_p)
{
	struct wcg_private *priv_p = (struct wcg_private *)wcg_p->wg_private;
	struct twcg_interface *twcg_p = priv_p->p_twcg;
	struct bwcg_interface *bwcg_p = priv_p->p_bwcg;

	if (bwcg_p){
		nid_log_warning( "wcg_recover_all_wc: recover_all_bwc ");
		bwcg_p->wg_op->wg_recover_all_bwc(bwcg_p);
	}
	if (twcg_p) {
		nid_log_warning( "wcg_recover_all_wc: wg_recover_all_twc ");
		twcg_p->wg_op->wg_recover_all_twc(twcg_p);
	}

}
struct wcg_operations wcg_op = {
	.wg_get_bwcg = wcg_get_bwcg,
	.wg_get_twcg = wcg_get_twcg,
	.wg_search = wcg_search,
	.wg_search_and_create_wc = wcg_search_and_create_wc,
	.wg_recover_all_wc = wcg_recover_all_wc,
};

int
wcg_initialization(struct wcg_interface *wcg_p, struct wcg_setup *setup)
{
	char *log_header = "wcg_initialization";
	struct wcg_private *priv_p;
	struct bwcg_setup bwcg_setup;
	struct bwcg_interface *bwcg_p;
	struct twcg_setup twcg_setup;
	struct twcg_interface *twcg_p;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);

	wcg_p->wg_op = &wcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	wcg_p->wg_private = priv_p;

	priv_p->p_ini = setup->ini;

	if (setup->support_bwc) {
		bwcg_p = x_calloc(1, sizeof(*bwcg_p));
		priv_p->p_bwcg = bwcg_p;
		bwcg_setup.ini = &priv_p->p_ini;
		bwcg_setup.sdsg = setup->sdsg;
		bwcg_setup.srn = setup->srn;
		bwcg_setup.allocator = setup->allocator;
		bwcg_setup.lstn = setup->lstn;
		bwcg_setup.io = setup->io;
		bwcg_setup.tpg = setup->tpg;
		bwcg_setup.mqtt = setup->mqtt;
		bwcg_initialization(bwcg_p, &bwcg_setup);
	}

	if (setup->support_twc) {
		twcg_p = x_calloc(1, sizeof(*twcg_p));
		priv_p->p_twcg = twcg_p;
		twcg_setup.ini = &priv_p->p_ini;
		twcg_setup.sdsg = setup->sdsg;
		twcg_setup.srn = setup->srn;
		twcg_setup.allocator = setup->allocator;
		twcg_setup.tpg = setup->tpg;
		twcg_initialization(twcg_p, &twcg_setup);
	}

	return 0;
}
