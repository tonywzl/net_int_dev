/*
 * asc_if.h
 * 	Interface of Agent Service Channel Module
 */
#ifndef NID_ASC_IF_H
#define NID_ASC_IF_H

#include <stdint.h>
#include "list.h"
#include "nid_shared.h"

struct agent_job;
struct asc_description {
	struct agent_job	*as_job;
	struct list_head	as_list;
	char			as_uuid[NID_MAX_UUID];
	char			as_devname[NID_MAX_DEVNAME];
	char			as_ip[NID_MAX_IP];
	char			as_busy;	// don't touch me
};

struct asc_stat {
	char		*as_uuid;
	char		*as_devname;
	char		*as_sip;
	char		as_state;
	char		as_alevel;
	uint64_t	as_read_counter;
	uint64_t	as_write_counter;
	uint64_t	as_keepalive_counter;
	uint64_t	as_read_resp_counter;
	uint64_t	as_write_resp_counter;
	uint64_t	as_keepalive_resp_counter;
	uint64_t	as_recv_sequence;
	uint64_t	as_wait_sequence;
	time_t 		as_ts;
};

struct asc_interface;
struct asc_operations {
	int	(*as_housekeeping)(struct asc_interface *);
	void	(*as_op_get)(struct asc_interface *);
	void	(*as_op_put)(struct asc_interface *);
	int	(*as_ex_lock)(struct asc_interface *);
	void	(*as_ex_unlock)(struct asc_interface *);
	int	(*as_op_busy)(struct asc_interface *);
	int	(*as_upgrade_alevel)(struct asc_interface *);
	char*	(*as_get_uuid)(struct asc_interface *);
	char*	(*as_get_devname)(struct asc_interface *);
	int	(*as_get_minor)(struct asc_interface *);
	void	(*as_cleanup)(struct asc_interface *);
	void	(*as_get_stat)(struct asc_interface *, struct asc_stat *);

	/*
	 * sending requests to devices
	 */
	int	(*as_xdev_init_device)(struct asc_interface *);
	int	(*as_xdev_keepalive_channel)(struct asc_interface *);
	int	(*as_xdev_start_channel)(struct asc_interface *);
	int	(*as_xdev_delete_device)(struct asc_interface *);
	int	(*as_xdev_ioerror_device)(struct asc_interface *);
	int	(*as_xdev_upgrade)(struct asc_interface *, char);

	/*
	 * processng messages from a devices
	 */
	int	(*as_rdev_keepalive)(struct asc_interface *, uint64_t, uint64_t, uint32_t);
	int	(*as_rdev_recover_channel)(struct asc_interface *);
	int	(*as_rdev_init_device)(struct asc_interface *, uint8_t);
	int	(*as_rdev_start_channel)(struct asc_interface *);
	int	(*as_rdev_delete_device)(struct asc_interface *);
	int	(*as_rdev_ioerror_device)(struct asc_interface *);
	int	(*as_rdev_upgrade)(struct asc_interface *, char);

	int	(*as_ok_delete_device)(struct asc_interface *);
};

struct asc_interface {
	void			*as_private;
	struct asc_operations	*as_op;
};

struct mpk_interface;
struct tp_interface;
struct ash_interface;
struct ascg_interface;
struct asc_setup {
	int			type;
	int			chan_id;
	struct mpk_interface	*mpk;
	struct tp_interface	*tp;
	struct ascg_interface	*ascg;
	struct pp2_interface	*pp2;
	struct pp2_interface	*dyn_pp2;
	void			*ascg_chan;
	char			*uuid;
	char			*devname;
	char			*sip;
	char			*access;
	int			wr_timewait;
	char			*iw_hook;
	char			*wr_hook;
	char			*rw_hook;
	char			*up_hook;
};

extern int asc_initialization(struct asc_interface *, struct asc_setup *);
extern int asc_sm_i_housekeeping(void *);
extern int asc_sm_w_housekeeping(void *);
extern int asc_sm_r_housekeeping(void *);
extern int asc_sm_rw_housekeeping(void *);
extern int asc_sm_iw_housekeeping(void *);
extern int asc_sm_xdev_delete_device(void *);
extern int asc_sm_xdev_ioerror_device(void *);
extern int asc_sm_xdev_init_device(void *);
extern int asc_sm_xdev_start_channel(void *);
extern int asc_sm_xdev_keepalive_channel(void *);
extern int asc_sm_rdev_keepalive(void *);
extern int asc_sm_rdev_iw_start_channel(void *);
extern int asc_sm_rdev_rw_start_channel(void *);
extern int asc_sm_rdev_recover_channel(void *);
extern int asc_sm_rdev_delete_device(void *);
extern int asc_sm_rdev_ioerror_device(void *);

#endif
