/*
 * dsm.c
 * 	Implementation Deterministic State Machine Module
 */

#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
#include "asc_if.h"
#include "dsm_if.h"

struct dsm_private {
	uint8_t	p_state;
	uint8_t	p_tt[DSM_MAX_STATE][DSM_MAX_INPUT];	// transition table
	struct dsm_state_operations p_ot[DSM_MAX_STATE];// operations table
	void *p_data;
};

static struct dsm_state_transition transition_table[] = {
	{DSM_STATE_NO,		0,	DSM_STATE_INIT},
	{DSM_STATE_NO,		1,	DSM_STATE_INIT},
	{DSM_STATE_NO,		2,	DSM_STATE_INIT},	// don't accept delete
	{DSM_STATE_INIT,	0,	DSM_STATE_IWI},
	{DSM_STATE_INIT,	1,	DSM_STATE_INIT},
	{DSM_STATE_INIT,	2,	DSM_STATE_DELETE},	// accept delete
	{DSM_STATE_IWI,		0,	DSM_STATE_IWS},
	{DSM_STATE_IWI,		1,	DSM_STATE_IWI},
	{DSM_STATE_IWI,		2,	DSM_STATE_IWI},		// don't accept delete
	{DSM_STATE_IWS,		0,	DSM_STATE_WORK},
	{DSM_STATE_IWS,		1,	DSM_STATE_IWS},
	{DSM_STATE_IWS,		2,	DSM_STATE_IWS},		// don't accept delete
	{DSM_STATE_WORK,	0,	DSM_STATE_WORK},
	{DSM_STATE_WORK,	1,	DSM_STATE_RECOVER},
	{DSM_STATE_WORK,	2,	DSM_STATE_DELETE},	// accept delete
	{DSM_STATE_RECOVER,	0,	DSM_STATE_RW},
	{DSM_STATE_RECOVER,	1,	DSM_STATE_RECOVER},
	{DSM_STATE_RECOVER,	2,	DSM_STATE_DELETE},	// accept delete
	{DSM_STATE_SLEEP,	0,	0},
	{DSM_STATE_RW,		0,	DSM_STATE_WORK},
	{DSM_STATE_RW,		1,	DSM_STATE_RW},
	{DSM_STATE_RW,		2,	DSM_STATE_RW},		// don't accept delete
	{DSM_STATE_DELETE,	0,	DSM_STATE_NO},
	{DSM_STATE_DELETE,	1,	DSM_STATE_DELETE},
	{DSM_STATE_DELETE,	2,	DSM_STATE_DELETE},	// don't accept another delete
	{DSM_INVALID_STATE,	0,	0}
};

#define INIT	DSM_STATE_INIT
#define IWI	DSM_STATE_IWI
#define	IWS	DSM_STATE_IWS
#define	W	DSM_STATE_WORK
#define	RW	DSM_STATE_RW
#define	R	DSM_STATE_RECOVER
#define	D	DSM_STATE_DELETE
#define INV	DSM_INVALID_STATE	

struct state_operation_view {
	uint8_t state;
	int	do_it[DSM_OP_MAX];
};

/*
 * hk:		do_housekeeping
 * x_dd:	do_xdev_delete_device
 * x_dc:	do_xdev_delete_channel
 * x_id:	do_xdev_init_device 
 * x_sc:	do_xdev_start_channel 
 * x_ie:	do_xdev_ioerror_device 
 * x_ka:	do_xdev_keepalive_channel
 * r_dd:	do_rdev_delete_device 
 * r_dc:	do_rdev_delete_channel
 * r_ie:	do_rdev_ioerror_device
 * r_ka:	do_rdev_keepalive	
 * r_sc:	do_rdev_start_channel
 * r_rd:	do_rdev_recover_channel
*/

struct state_operation_view op_view[DSM_MAX_STATE] = {
/* s 	hk	x_dd	x_id	x_sc	x_ie	x_ka	r_dd	r_ie	r_ka	r_sc	r_rd */
{INIT,	{1,	1,	1,	0,	0,	0,	0,	0,	0,	0,	0}},
{IWI,	{1,	0,	0,	1,	0,	0,	0,	0,	0,	0,	0}},
{IWS,	{1,	0,	0,	0,	0,	0,	0,	0,	0,	1,	0}},
{W,	{1,	1,	0,	0,	0,	1,	0,	0,	1,	0,	1}},
{R,	{1,	1,	0,	1,	1,	0,	0,	1,	0,	0,	0}},
{RW,	{1,	0,	0,	0,	0,	0,	0,	0,	0,	1,	0}},
{D,	{0,	0,	0,	0,	0,	0,	1,	0,	0,	0,	0}},
{INV,	{0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0}},
};

static void
_init_op_table(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = priv_p->p_ot;

	ot_p = &priv_p->p_ot[DSM_STATE_INIT];
	ot_p->so_state = DSM_STATE_INIT;
	ot_p->so_ops[DSM_OP_HOUSEKEEPING] = asc_sm_i_housekeeping;
	ot_p->so_ops[DSM_OP_XDELDEVICE] = asc_sm_xdev_delete_device;
	ot_p->so_ops[DSM_OP_XINITDEVICE] = asc_sm_xdev_init_device;

	ot_p = &priv_p->p_ot[DSM_STATE_WORK];
	ot_p->so_state = DSM_STATE_WORK;
	ot_p->so_ops[DSM_OP_HOUSEKEEPING] = asc_sm_w_housekeeping;
	ot_p->so_ops[DSM_OP_XDELDEVICE] = asc_sm_xdev_delete_device;
	ot_p->so_ops[DSM_OP_XKEEPALIVE] = asc_sm_xdev_keepalive_channel;
	ot_p->so_ops[DSM_OP_RKEEPALIVE] = asc_sm_rdev_keepalive;
	ot_p->so_ops[DSM_OP_RRCDEVICE] = asc_sm_rdev_recover_channel;

	ot_p = &priv_p->p_ot[DSM_STATE_IWI];
	ot_p->so_state = DSM_STATE_IWI;
	ot_p->so_ops[DSM_OP_HOUSEKEEPING] = asc_sm_iw_housekeeping;
	ot_p->so_ops[DSM_OP_XSTARTCHANNEL] = asc_sm_xdev_start_channel;

	ot_p = &priv_p->p_ot[DSM_STATE_IWS];
	ot_p->so_state = DSM_STATE_IWS;	
	ot_p->so_ops[DSM_OP_HOUSEKEEPING] = asc_sm_iw_housekeeping;
	ot_p->so_ops[DSM_OP_RSTARTCHANNEL] = asc_sm_rdev_iw_start_channel;

	ot_p = &priv_p->p_ot[DSM_STATE_RW];
	ot_p->so_state = DSM_STATE_RW;
	ot_p->so_ops[DSM_OP_HOUSEKEEPING] = asc_sm_rw_housekeeping;
	ot_p->so_ops[DSM_OP_RSTARTCHANNEL] = asc_sm_rdev_rw_start_channel;

	ot_p = &priv_p->p_ot[DSM_STATE_RECOVER];
	ot_p->so_state = DSM_STATE_RECOVER;
	ot_p->so_ops[DSM_OP_HOUSEKEEPING] = asc_sm_r_housekeeping;
	ot_p->so_ops[DSM_OP_XDELDEVICE] = asc_sm_xdev_delete_device;
	ot_p->so_ops[DSM_OP_XSTARTCHANNEL] = asc_sm_xdev_start_channel;
	ot_p->so_ops[DSM_OP_XIOERRDEVICE] = asc_sm_xdev_ioerror_device;
	ot_p->so_ops[DSM_OP_RIOERRDEVICE] = asc_sm_rdev_ioerror_device;

	ot_p = &priv_p->p_ot[DSM_STATE_DELETE];
	ot_p->so_state = DSM_STATE_DELETE;
	ot_p->so_ops[DSM_OP_RDELDEVICE] = asc_sm_rdev_delete_device;
}

int
_op_table_checking(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p;
	struct state_operation_view *view_p = op_view;
	int i;

	while (view_p->state != DSM_INVALID_STATE) {
		ot_p = &priv_p->p_ot[view_p->state];
		for (i = 0; i < DSM_OP_MAX; i++) {
			if ((ot_p->so_ops[i] && !view_p->do_it[i]) ||
			    (!ot_p->so_ops[i] && view_p->do_it[i])) {
				nid_log_error("_op_table_checking: state:%d, i:%d, op:%p, do_it:%d",
					view_p->state, i, ot_p->so_ops[i], view_p->do_it[i]);
				return -1;
			}
		}
		view_p++;
	}
	return 0;
}

static void 
dsm_state_transition(struct dsm_interface *dsm_p, uint8_t input)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	assert(input < DSM_MAX_INPUT);
	nid_log_debug("dsm_state_transition: %d -> %d", 
		priv_p->p_state, priv_p->p_tt[priv_p->p_state][input]);
	priv_p->p_state = priv_p->p_tt[priv_p->p_state][input];
}

static int 
dsm_housekeeping(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_HOUSEKEEPING;
	int rc = 0;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	return rc;
}

static int 
dsm_xdev_delete_device(struct dsm_interface *dsm_p)
{
	char *log_header = "dsm_xdev_delete_device"; 
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_XDELDEVICE;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("%s: state:%d does not support this op",
			log_header, priv_p->p_state);
	return rc;
}

static int 
dsm_xdev_init_device(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_XINITDEVICE;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("dsm_xdev_init_device: state:%d does not support this op",
			priv_p->p_state);
	return rc;
}

static int 
dsm_xdev_start_channel(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_XSTARTCHANNEL;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("dsm_xdev_start_channel: state:%d does not support this op",
			priv_p->p_state);
	return rc;
}

/*
 * Return:
 * 	 1: expecting rdev_ioerro to finish
 * 	-1: wrong state
 */
static int 
dsm_xdev_ioerror_device(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_XIOERRDEVICE;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("dsm_xdev_ioerror_device: state:%d does not support this op",
			priv_p->p_state);
	return rc;
}

static int 
dsm_xdev_keepalive_channel(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_XKEEPALIVE;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("dsm_xdev_keepalive_channel: state:%d does not support this op",
			priv_p->p_state);
	return rc;
}

static int 
dsm_rdev_keepalive(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_RKEEPALIVE;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("dsm_rdev_keepalive: state:%d does not support this op",
			priv_p->p_state);
	return rc;
}

static int
dsm_rdev_start_channel(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_RSTARTCHANNEL;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("dsm_rdev_start_channel: state:%d does not support this op",
			priv_p->p_state);
	return rc;
}

static int 
dsm_rdev_delete_device(struct dsm_interface *dsm_p)
{
	char *log_header = "dsm_rdev_delete_device";
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_RDELDEVICE;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("%s: state:%d does not support this op",
			log_header, priv_p->p_state);
	return rc;
}

/*
 * Return:
 * 	 0: done 
 * 	-1: wrong state
 */
static int 
dsm_rdev_ioerror_device(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_RIOERRDEVICE;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("dsm_rdev_ioerror_device: state:%d does not support this op",
			priv_p->p_state);
	return rc;
}

static int 
dsm_rdev_recover_channel(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_RRCDEVICE;
	int rc = -1;
	if (ot_p->so_ops[op_code])
		rc = ot_p->so_ops[op_code](priv_p->p_data);
	else
		nid_log_error("dsm_rdev_recover_channel: state:%d does not support this op",
			priv_p->p_state);
	return rc;
}

static int
dsm_ok_delete_device(struct dsm_interface *dsm_p)
{
	char *log_header = "dsm_ok_delete_device";
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	struct dsm_state_operations *ot_p = &priv_p->p_ot[priv_p->p_state];
	int op_code = DSM_OP_XDELDEVICE;
	if (ot_p->so_ops[op_code])
		return 1;	// ok to delete

	nid_log_notice("%s: not ok to delete at state:%d", log_header, priv_p->p_state);
	return 0;		// not ok to delete

}

static uint8_t
dsm_get_state(struct dsm_interface *dsm_p)
{
	struct dsm_private *priv_p = (struct dsm_private *)dsm_p->sm_private;
	return priv_p->p_state;
}

struct dsm_operations dsm_op = {
	.sm_state_transition = dsm_state_transition,
	.sm_housekeeping = dsm_housekeeping,
	.sm_xdev_delete_device = dsm_xdev_delete_device,
	.sm_xdev_init_device = dsm_xdev_init_device,
	.sm_xdev_start_channel = dsm_xdev_start_channel,
	.sm_xdev_ioerror_device = dsm_xdev_ioerror_device,
	.sm_xdev_keepalive_channel = dsm_xdev_keepalive_channel,
	.sm_rdev_keepalive = dsm_rdev_keepalive,
	.sm_rdev_delete_device = dsm_rdev_delete_device,
	.sm_rdev_ioerror_device = dsm_rdev_ioerror_device,
	.sm_rdev_start_channel = dsm_rdev_start_channel,
	.sm_rdev_recover_channel = dsm_rdev_recover_channel,
	.sm_ok_delete_device = dsm_ok_delete_device,
	.sm_get_state = dsm_get_state,
};

int
dsm_initialization(struct dsm_interface *dsm_p, struct dsm_setup *setup)
{
	struct dsm_private *priv_p;
	struct dsm_state_transition *tran_p;

	nid_log_info("dsm_initialization start ...");
	if (!setup)
		return -1;

	priv_p = x_calloc(1, sizeof(*priv_p));
	dsm_p->sm_private = priv_p;
	dsm_p->sm_op = &dsm_op;
	priv_p->p_state = DSM_STATE_NO;
	priv_p->p_data = setup->data;
	tran_p = transition_table;
	while (tran_p->st_state != DSM_INVALID_STATE) {
		nid_log_debug("dsm_initialization: transition table got state %d", tran_p->st_state);
		priv_p->p_tt[tran_p->st_state][tran_p->st_input] = tran_p->st_next_state;
		tran_p++;
	}

	_init_op_table(dsm_p);
	if (_op_table_checking(dsm_p)) {
		nid_log_error("dsm_initialization: _op_table_checking failed");
		assert(0);
	}


	return 0;
}
