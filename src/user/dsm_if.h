/*
 * dsm_if.h
 * 	Interface of Deterministic State Machine Module
 */
#ifndef NID_DSM_IF_H
#define NID_DSM_IF_H

#include <stdint.h>

#define DSM_STATE_NO		0
#define DSM_STATE_INIT		1
#define DSM_STATE_WORK		2
#define DSM_STATE_RECOVER	3
#define DSM_STATE_SLEEP		4
#define DSM_STATE_IWI		5
#define DSM_STATE_IWS		6
#define DSM_STATE_RW		7
#define DSM_STATE_DELETE	8
#define	DSM_MAX_STATE		9	
#define	DSM_INVALID_STATE	DSM_MAX_STATE	

static inline char*
nid_state_to_str(int state)
{
	switch (state) {
	case DSM_STATE_NO:		return "no position";
	case DSM_STATE_INIT:		return "initial";
	case DSM_STATE_WORK:		return "working";
	case DSM_STATE_RECOVER:		return "recovering";
	case DSM_STATE_SLEEP:		return "sleeping";
	case DSM_STATE_DELETE:		return "deleting";
	case DSM_STATE_IWI:		return "transition from init to work: IWI";
	case DSM_STATE_IWS:		return "transition from init to work, IWS";
	case DSM_STATE_RW:		return "transition from recover to work";
	default:			return "unknown";
	}
}

#define	DSM_MAX_INPUT		3

/*
 * lookahead 
 */
#define DSM_LA_SUCCESS		0
#define DSM_LA_FAILURE		1
#define DSM_LA_DELETE		2

/*
 * state actions
 */
#define DSM_OP_HOUSEKEEPING	0	// hosekeeping
#define DSM_OP_XDELDEVICE	1	// xdev_delete_device
#define DSM_OP_XINITDEVICE	2	// xdev_init_device
#define DSM_OP_XSTARTCHANNEL	3	// xdev_start_channel
#define DSM_OP_XIOERRDEVICE	4	// xdevv_ioerror_Device
#define DSM_OP_XKEEPALIVE	5	// xdev_keepalive_channel
#define DSM_OP_RDELDEVICE	6	// rdev_delete_device
#define DSM_OP_RIOERRDEVICE	7	// rdev_ioerror_device
#define DSM_OP_RKEEPALIVE	8	// rdev_keepalive
#define DSM_OP_RSTARTCHANNEL	9	// rdev_start_channel
#define DSM_OP_RRCDEVICE	10	// rdev_recover_channel
#define DSM_OP_MAX		11

struct dsm_state_transition {
	uint8_t	st_state;
	uint8_t	st_input;
	uint8_t	st_next_state;
};

/*
 * Don't change the order
 */
struct dsm_state_action {
	uint8_t	sa_state;
	int	(*sa_housekeeping)(void *);
	int	(*sa_xdev_delete_device)(void *);
	int	(*sa_xdev_init_device)(void *);
	int	(*sa_xdev_start_channel)(void *);
	int	(*sa_xdev_ioerror_device)(void *);
	int	(*sa_xdev_keepalive_channel)(void *);
	int	(*sa_rdev_delete_device)(void *);
	int	(*sa_rdev_ioerror_device)(void *);
	int	(*sa_rdev_keepalive)(void *);
	int	(*sa_rdev_start_channel)(void *);
	int	(*sa_rdev_recover_channel)(void *);
};

typedef int (*state_operation_t)(void *);
struct dsm_state_operations {
	uint8_t			so_state;
	state_operation_t	so_ops[DSM_OP_MAX];	
};

struct dsm_interface;
struct dsm_operations {
	void	(*sm_state_transition)(struct dsm_interface *, uint8_t);
	int	(*sm_housekeeping)(struct dsm_interface *);
	int	(*sm_xdev_delete_device)(struct dsm_interface *);
	int	(*sm_xdev_init_device)(struct dsm_interface *);
	int	(*sm_xdev_start_channel)(struct dsm_interface *);
	int	(*sm_xdev_ioerror_device)(struct dsm_interface *);
	int	(*sm_xdev_keepalive_channel)(struct dsm_interface *);
	int	(*sm_rdev_keepalive)(struct dsm_interface *);
	int	(*sm_rdev_start_channel)(struct dsm_interface *);
	int	(*sm_rdev_recover_channel)(struct dsm_interface *);
	int	(*sm_rdev_delete_device)(struct dsm_interface *);
	int	(*sm_rdev_ioerror_device)(struct dsm_interface *);
	int	(*sm_ok_delete_device)(struct dsm_interface *);
	uint8_t	(*sm_get_state)(struct dsm_interface *);
};

struct dsm_interface {
	void			*sm_private;
	struct dsm_operations	*sm_op;
};

struct dsm_setup {
	void				*data;
};

extern int dsm_initialization(struct dsm_interface *, struct dsm_setup *);
#endif
