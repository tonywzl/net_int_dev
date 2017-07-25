/*
 * ascg_if.h
 * 	Interface of Agent Server Channel Guardian Module
 */
#ifndef NID_ASCG_IF_H
#define NID_ASCG_IF_H

#include <stdint.h>
#include "list.h"

struct ascg_interface;
struct ascg_message {
	struct asc_interface	*m_asc;
	uint8_t			m_req;
	int			m_owner_code;
	int			(*m_callback)(struct ascg_interface *, struct ascg_message *);
};

struct asc_description;
struct ini_interface;
struct list_head;
struct ascg_operations {
	void			(*ag_ioctl_lock)(struct ascg_interface *);
	void			(*ag_ioctl_unlock)(struct ascg_interface *);
	void			(*ag_start)(struct ascg_interface *);
	void			(*ag_failed_to_establish_channel)(struct ascg_interface *, void *);
	void			(*ag_display)(struct ascg_interface *);
	struct ini_interface*	(*ag_get_ini)(struct ascg_interface *);
	void			(*ag_get_stat)(struct ascg_interface *, struct list_head *);
	int			(*ag_update)(struct ascg_interface *);
	void			(*ag_stop)(struct ascg_interface *);
	void			(*ag_set_hook)(struct ascg_interface *, uint32_t);
	uint32_t		(*ag_get_hook)(struct ascg_interface *);
	int			(*ag_delete_device)(struct ascg_interface *, char *, int, int);
	int			(*ag_delete_channel)(struct ascg_interface *, int, int, int);
	int			(*ag_upgrade_device)(struct ascg_interface *, char *, int, char);
	int			(*ag_ioerror_device)(struct ascg_interface *, char *, int);
	void			(*ag_check_driver)(struct ascg_interface *, int);
	void			(*ag_cleanup)(struct ascg_interface *);

	/*
	 * send requests to the device
	 */
	void			(*ag_xdev_init_device)(struct ascg_interface *, int, uint64_t, uint32_t);
	void			(*ag_xdev_start_channel)(struct ascg_interface *, int, uint8_t, int, int);
	void			(*ag_xdev_upgrade_channel)(struct ascg_interface *, int, int, int);
	void			(*ag_xdev_recover_channel)(struct ascg_interface *, int, int);
	void			(*ag_xdev_delete_device)(struct ascg_interface *, int);
	void			(*ag_xdev_delete_channel)(struct ascg_interface *, int);
	void			(*ag_xdev_keepalive_channel)(struct ascg_interface *, int, int);
	void			(*ag_xdev_ioerror_device)(struct ascg_interface *, int);

	/*
	 * processing  messages from device
	 */
	void			(*ag_rdev_keepalive)(struct ascg_interface *, int, int, uint64_t, uint64_t, uint32_t);
	int			(*ag_rdev_recover_channel)(struct ascg_interface *, int, int);
	int			(*ag_rdev_ioerror_device)(struct ascg_interface *, int, char);
	int			(*ag_rdev_init_device)(struct ascg_interface *, int, uint8_t);
	int			(*ag_rdev_start_channel)(struct ascg_interface *, int, int);
	int			(*ag_rdev_delete_device)(struct ascg_interface *, int);
	void			(*ag_rdev_upgrade)(struct ascg_interface *, int, int, char);

	struct asc_interface*	(*ag_get_asc_by_uuid)(struct ascg_interface *, char *);
	int			(*ag_get_chan_id)(struct ascg_interface *, char *);

};

struct ascg_interface {
	void			*ag_private;
	struct ascg_operations	*ag_op;
};

struct drv_interface;
struct mpk_interface;
struct ini_interface;
struct tp_interface;
struct ascg_setup {
	struct drv_interface	*drv;
	struct mpk_interface	*mpk;
	struct ini_interface	*ini;
	struct tp_interface	*tp;
	struct pp2_interface	*pp2;
	struct pp2_interface	*dyn_pp2;
	struct list_head	agent_keys;
	struct acg_interface	*acg;
	int			support_driver;
};

extern int ascg_initialization(struct ascg_interface *, struct ascg_setup *);
#endif
