/*
 * dev_if.h
 * 	Interface of Device Module 
 */
#ifndef NID_DEV_IF_H
#define NID_DEV_IF_H

struct dev_interface;
struct gendisk;
struct nid_message;
struct request;
struct dev_operations {
	int	(*dv_init_device)(struct dev_interface *, uint8_t, uint64_t, uint32_t, char *);
	struct kobject*
		(*dv_nid_probe)(struct dev_interface *);
	struct kobject*
		(*dv_get_kobj)(struct dev_interface *);
	int	(*dv_start_channel)(struct dev_interface *, int, int, int);
	void	(*dv_purge)(struct dev_interface *);
	void	(*dv_nid_to_delete)(struct dev_interface *);
	void	(*dv_nid_set_ioerror)(struct dev_interface *);
	int	(*dv_upgrade_channel)(struct dev_interface *, char);
	void	(*dv_recover_channel)(struct dev_interface *);
	int	(*dv_send_keepalive_channel)(struct dev_interface *);
	int	(*dv_get_minor)(struct dev_interface *);
	int	(*dv_get_stat)(struct dev_interface *, struct nid_message *);
	void	(*dv_cleanup)(struct dev_interface *);
	void	(*dv_drop_request)(struct dev_interface *, struct request *);
	void	(*dv_reply_request)(struct dev_interface *, struct request *);
};

struct dev_interface {
	void			*dv_private;
	struct dev_operations	*dv_op;
};

struct mutex;
struct ddg_interface;
struct dev_setup {
	int			idx;
	int			minor;
	struct ddg_interface	*ddg;
	int			part_shift;
	struct mutex		*mutex;
};

extern int dev_initialization(struct dev_interface *, struct dev_setup *);
#endif
