/*
 * bfp.c
 * 	Implementation of Bio Flushing Policy Module
 */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <string.h>

#include "nid_log.h"
#include "ini_if.h"
#include "bfp_if.h"
#include "bfp1_if.h"
#include "bfp2_if.h"
#include "bfp3_if.h"
#include "bfp4_if.h"
#include "bfp5_if.h"

struct bfp_private {
	int			p_type;
	void			*p_real_bfp;
};

static int
bfp_get_flush_num(struct bfp_interface *bfp_p)
{
	char *log_header = "bfp_flush_fuction";
	struct bfp_private *priv_p = (struct bfp_private *)bfp_p->fp_private;
	struct bfp1_interface *bfp1_p;
	struct bfp2_interface *bfp2_p;
	struct bfp3_interface *bfp3_p;
	struct bfp4_interface *bfp4_p;
	struct bfp5_interface *bfp5_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case BFP_POLICY_BFP1:
		bfp1_p = (struct bfp1_interface *)priv_p->p_real_bfp;
		rc = bfp1_p->fp_op->bf_get_flush_num(bfp1_p);
		break;

	case BFP_POLICY_BFP2:
		bfp2_p = (struct bfp2_interface *)priv_p->p_real_bfp;
		rc = bfp2_p->fp_op->bf_get_flush_num(bfp2_p);
		break;

	case BFP_POLICY_BFP3:
		bfp3_p = (struct bfp3_interface *)priv_p->p_real_bfp;
		rc = bfp3_p->fp_op->bf_get_flush_num(bfp3_p);
		break;

	case BFP_POLICY_BFP4:
		bfp4_p = (struct bfp4_interface *)priv_p->p_real_bfp;
		rc = bfp4_p->fp_op->bf_get_flush_num(bfp4_p);
		break;

	case BFP_POLICY_BFP5:
		bfp5_p = (struct bfp5_interface *)priv_p->p_real_bfp;
		rc = bfp5_p->fp_op->bf_get_flush_num(bfp5_p);
		break;

	default:
		nid_log_error("%s: got wrong flush policy %d", log_header, priv_p->p_type);
		rc = -1;
	}

	return rc;
}

static int
bfp_upadte_water_mark(struct bfp_interface *bfp_p, int low_water_mark, int high_water_mark)
{
	char *log_header = "bfp_flush_fuction";
	struct bfp_private *priv_p = (struct bfp_private *)bfp_p->fp_private;
	struct bfp1_interface *bfp1_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case BFP_POLICY_BFP1:
		bfp1_p = (struct bfp1_interface *)priv_p->p_real_bfp;
		rc = bfp1_p->fp_op->bf_update_water_mark(bfp1_p, low_water_mark, high_water_mark);
		break;

	default:
		nid_log_error("%s: got wrong flush policy %d", log_header, priv_p->p_type);
		rc = -1;
	}
	return rc;

}

struct bfp_operations bfp_op = {
	.bf_get_flush_num = bfp_get_flush_num,
	.bf_update_water_mark = bfp_upadte_water_mark,
};

int
bfp_initialization(struct bfp_interface *bfp_p, struct bfp_setup *setup)
{
	char *log_header = "bfp_initialization";
	struct bfp_private *priv_p;

	struct bfp1_interface *bfp1_p;
	struct bfp1_setup bfp1_setup;
	struct bfp2_interface *bfp2_p;
	struct bfp2_setup bfp2_setup;
	struct bfp3_interface *bfp3_p;
	struct bfp3_setup bfp3_setup;
	struct bfp4_interface *bfp4_p;
	struct bfp4_setup bfp4_setup;
	struct bfp5_interface *bfp5_p;
	struct bfp5_setup bfp5_setup;

	nid_log_notice("%s: start ...", log_header);
	assert(setup);
	bfp_p->fp_op = &bfp_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bfp_p->fp_private = priv_p;

	priv_p->p_type = setup->type;

	switch(priv_p->p_type) {
	case BFP_POLICY_BFP1:
		nid_log_notice("%s: initialize bfp1 flush policy", log_header);
		bfp1_p = (struct bfp1_interface *)x_calloc(1, sizeof(*bfp1_p));
		priv_p->p_real_bfp = bfp1_p;
		bfp1_setup.wc = setup->wc;
		bfp1_setup.pp = setup->pp;
		bfp1_setup.pagesz = setup->pagesz;
		bfp1_setup.buffersz = setup->buffersz;
		bfp1_setup.low_water_mark = setup->low_water_mark;
		bfp1_setup.high_water_mark = setup->high_water_mark;
		bfp1_initialization(bfp1_p, &bfp1_setup);
		break;

	case BFP_POLICY_BFP2:
		nid_log_notice("%s: initialize bfp2 flush policy", log_header);
		bfp2_p = (struct bfp2_interface *)x_calloc(1, sizeof(*bfp2_p));
		priv_p->p_real_bfp = bfp2_p;
		bfp2_setup.wc = setup->wc;
		bfp2_setup.pp = setup->pp;
		bfp2_setup.pagesz = setup->pagesz;
		bfp2_setup.buffersz = setup->buffersz;
		bfp2_setup.flush_delay_ctl = setup->flush_delay_ctl;
		bfp2_setup.load_ctrl_level = setup->load_ctrl_level;
		bfp2_setup.load_ratio_max = setup->load_ratio_max;
		bfp2_setup.load_ratio_min = setup->load_ratio_min;

		bfp2_initialization(bfp2_p, &bfp2_setup);
		break;

	case BFP_POLICY_BFP3:
		nid_log_notice("%s: initialize bfp3 flush policy", log_header);
		bfp3_p = (struct bfp3_interface *)x_calloc(1, sizeof(*bfp3_p));
		priv_p->p_real_bfp = bfp3_p;
		bfp3_setup.wc = setup->wc;
		bfp3_setup.pp = setup->pp;
		bfp3_setup.pagesz = setup->pagesz;
		bfp3_setup.buffersz = setup->buffersz;
		bfp3_setup.coalesce_ratio = setup->coalesce_ratio;
		bfp2_setup.flush_delay_ctl = setup->flush_delay_ctl;
		bfp2_setup.load_ctrl_level = setup->load_ctrl_level;
		bfp2_setup.load_ratio_max = setup->load_ratio_max;
		bfp2_setup.load_ratio_min = setup->load_ratio_min;

		bfp3_initialization(bfp3_p, &bfp3_setup);
		break;

	case BFP_POLICY_BFP4:
		nid_log_notice("%s: initialize bfp4 flush policy", log_header);
		bfp4_p = (struct bfp4_interface *)x_calloc(1, sizeof(*bfp4_p));
		priv_p->p_real_bfp = bfp4_p;
		bfp4_setup.wc = setup->wc;
		bfp4_setup.pp = setup->pp;
		bfp4_setup.pagesz = setup->pagesz;
		bfp4_setup.buffersz = setup->buffersz;
		bfp4_setup.flush_delay_ctl = setup->flush_delay_ctl;
		bfp4_setup.load_ctrl_level = setup->load_ctrl_level;
		bfp4_setup.load_ratio_max = setup->load_ratio_max;
		bfp4_setup.load_ratio_min = setup->load_ratio_min;
		bfp4_setup.throttle_ratio = setup->throttle_ratio;

		bfp4_initialization(bfp4_p, &bfp4_setup);
		break;

	case BFP_POLICY_BFP5:
		nid_log_notice("%s: initialize bfp5 flush policy", log_header);
		bfp5_p = (struct bfp5_interface *)x_calloc(1, sizeof(*bfp5_p));
		priv_p->p_real_bfp = bfp5_p;
		bfp5_setup.wc = setup->wc;
		bfp5_setup.pp = setup->pp;
		bfp5_setup.pagesz = setup->pagesz;
		bfp5_setup.buffersz = setup->buffersz;
		bfp5_setup.coalesce_ratio = setup->coalesce_ratio;
		bfp5_setup.flush_delay_ctl = setup->flush_delay_ctl;
		bfp5_setup.load_ctrl_level = setup->load_ctrl_level;
		bfp5_setup.load_ratio_max = setup->load_ratio_max;
		bfp5_setup.load_ratio_min = setup->load_ratio_min;
		bfp5_setup.throttle_ratio = setup->throttle_ratio;

		bfp5_initialization(bfp5_p, &bfp5_setup);
		break;

	}

	return 0;
}
