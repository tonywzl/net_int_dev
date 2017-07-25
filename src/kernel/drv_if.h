/*
 * drv_if.h
 * 	Interface of Driver Module
 */
#ifndef NID_DRV_IF_H
#define NID_DRV_IF_H

struct nid_message;
struct drv_interface;
struct gendisk;
struct request;
struct nid_request;
struct drv_operations {
	int			(*dr_msg_process_thread)(void *data);
	struct kobject*		(*dr_probe_device)(struct drv_interface *, int);
	void			(*dr_add_device)(struct drv_interface *, void *, int);
	void			(*dr_start)(struct drv_interface *);
	void			(*dr_dev_purge)(struct drv_interface *, int);
	void			(*dr_start_channel)(struct drv_interface *, int, int, int, int, int);
	void			(*dr_stop)(struct drv_interface *);
	void			(*dr_unset_msg_exit)(struct drv_interface *);
	int			(*dr_get_stat)(struct drv_interface *, unsigned long);
	void			(*dr_wait_ioctl_cmd)(struct drv_interface *);
	void			(*dr_wakeup_ioctl_cmd)(struct drv_interface *);
	void			(*dr_wait_ioctl_cmd_done)(struct drv_interface *);
	void			(*dr_done_ioctl_cmd)(struct drv_interface *);
	struct nid_message*	(*dr_get_ioctl_message)(struct drv_interface *);
	void			(*dr_send_message)(struct drv_interface *, struct nid_message *, int *);
	void			(*dr_cleanup)(struct drv_interface *);
	void			(*dr_handle_request)(struct drv_interface *, struct request *, int);
	void			(*dr_drop_request)(struct drv_interface *, struct request *, int);
	void			(*dr_reply_request)(struct drv_interface *, int, struct request *);
};

struct drv_interface {
	void			*dr_private;
	struct drv_operations	*dr_op;
};

struct drv_setup {
	int		size;
	uint16_t	part_shift;
	int		max_devices;
	int		max_channels;
};

extern int drv_initialization(struct drv_interface *, struct drv_setup *);
#endif
