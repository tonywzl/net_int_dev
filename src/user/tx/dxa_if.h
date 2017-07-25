/*
 * dxa_if.h
 * 	Interface of Data Exchange Active Module
 */

#ifndef NID_DXA_IF_H
#define NID_DXA_IF_H

#include <sys/socket.h>
#include "nid_shared.h"
#include <stdint.h>

#define DXA_TYPE_WRONG		0
#define DXA_TYPE_MREPLICATION	1

struct dxa_interface;
struct dxt_interface;
struct dxa_operations {
	int	(*dx_make_control_connection)(struct dxa_interface *);
	int	(*dx_make_data_connection)(struct dxa_interface *, u_short);
	void	(*dx_do_channel)(struct dxa_interface *);
	int	(*dx_cleanup)(struct dxa_interface *);
	void	(*dx_mark_recover_dxt)(struct dxa_interface *);
	int     (*dx_housekeeping)(struct dxa_interface *);
	char*	(*dx_get_server_ip)(struct dxa_interface *);
	char*	(*dx_get_chan_uuid)(struct dxa_interface *);
	struct dxt_interface*   (*dx_get_dxt)(struct dxa_interface *);
	int     (*dx_start)(struct dxa_interface *);
};

struct dxa_interface {
	void			*dx_private;
	struct dxa_operations	*dx_op;
};

struct dxag_interface;
struct dxtg_interface;
struct dxa_setup {
	char	uuid[NID_MAX_UUID];	// channel uuid
	char	peer_uuid[NID_MAX_UUID];
	char	ip[NID_MAX_IP];
	char	dxt_name[NID_MAX_UUID];
	void	*handle;
	struct dxag_interface	*dxag;
	struct pp2_interface	*pp2;
	struct dxtg_interface	*dxtg;
	struct umpk_interface	*umpk;
};

extern int dxa_initialization(struct dxa_interface *, struct dxa_setup *);

#endif
