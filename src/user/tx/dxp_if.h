/*
 * dxp_if.h
 * 	Interface of Data Exchange Passive Module
 */

#ifndef NID_DXP_IF_H
#define NID_DXP_IF_H

#include <sys/socket.h>
#include <stdint.h>

#include "nid_shared.h"

#define DXP_TYPE_WRONG		0
#define DXP_TYPE_MREPLICATION	1

struct dxp_interface;
struct umessage_tx_hdr;
struct dxt_status;
struct dxp_operations {
	int32_t	(*dx_ack_dport)(struct dxp_interface *, int);
	struct dxt_interface *	(*dx_get_dxt)(struct dxp_interface *);

	int     (*dx_accept)(struct dxp_interface *);
	int     (*dx_cleanup)(struct dxp_interface *);
	int     (*dx_drop_channel)(struct dxp_interface *);
	int     (*dx_init_dport)(struct dxp_interface *);
	char*	(*dx_get_chan_uuid)(struct dxp_interface *);
	void	(*dx_mark_recover_dxt)(struct dxp_interface *);
	void	(*dx_do_channel)(struct dxp_interface *);
	void	(*dx_bind_socket)(struct dxp_interface *, int);
};

struct dxp_interface {
	void			*dx_private;
	struct dxp_operations	*dx_op;
};

struct dxp_setup {
	char	uuid[NID_MAX_UUID];
	char	peer_uuid[NID_MAX_UUID];
	char	dxt_name[NID_MAX_UUID];
	int	csfd;
	int	dsfd;
	struct umpk_interface	*umpk;
	struct dxpg_interface	*dxpg;
	struct dxtg_interface   *dxtg;
};

extern int dxp_initialization(struct dxp_interface *, struct dxp_setup *);
#endif
