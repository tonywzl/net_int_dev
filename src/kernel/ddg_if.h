/*
 * ddg_if.h
 * 	Interface of Driver Device Guardian Moulde
 */

#ifndef _DDG_IF_H
#define _DDG_IF_H

#include <linux/list.h>

struct ddg_interface;
struct rex_interface;
struct dev_interface;
struct nid_message;
struct request;
struct nid_request;
struct ddg_operations {
	struct kobject*		(*dg_probe_device)(struct ddg_interface *, int);
	int			(*dg_add_device)(struct ddg_interface *, struct dev_interface *dev_p);
	int			(*dg_do_start_channel)(struct ddg_interface *, int, int, int, int);
	void			(*dg_purge)(struct ddg_interface *, int);
	void			(*dg_start_channel)(struct ddg_interface *);
	void			(*dg_send_keepalive_channel)(struct ddg_interface *, int);
	void			(*dg_got_keepalive_channel)(struct ddg_interface *, int, struct nid_request *);
	void			(*dg_init_device)(struct ddg_interface *, int, uint8_t, uint64_t, uint32_t, char *);
	void			(*dg_set_ioerror_device)(struct ddg_interface *, int);
	void			(*dg_delete_device)(struct ddg_interface *, int);
	void			(*dg_recover_channel)(struct ddg_interface *, int);
	void			(*dg_upgrade_channel)(struct ddg_interface *, int, int);
	int			(*dg_get_stat_device)(struct ddg_interface *, int, struct nid_message *);
	void			(*dg_cleanup)(struct ddg_interface *ddg_p);
	void			(*dg_init_device_done)(struct ddg_interface *, int, uint8_t, uint8_t);
	void			(*dg_start_channel_done)(struct ddg_interface *, int);
	void			(*dg_delete_device_done)(struct ddg_interface *, int);
	void			(*dg_recover_channel_done)(struct ddg_interface *, int);
	void			(*dg_upgrade_channel_done)(struct ddg_interface *, int, char);
	void			(*dg_set_ioerror_device_done)(struct ddg_interface *, int, int);

	void			(*dg_drop_request)(struct ddg_interface *, struct request *, int);
	void			(*dg_reply_request)(struct ddg_interface *, struct request *, int);
};

struct ddg_interface {
	void			*dg_private;
	struct ddg_operations	*dg_op;
};

struct drv_interface;
struct ddg_setup {
	struct drv_interface	*drv;
	uint16_t		part_shift;
	uint16_t		max_devices;
};

extern int ddg_initialization(struct ddg_interface *, struct ddg_setup *);

#endif
