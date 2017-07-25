/*
 * drw_if.h
 * 	Interface of Device Read Write Module
 */
#ifndef NID_DRW_IF_H
#define NID_DRW_IF_H

#include "rw_if.h"
#include "nid_shared.h"

struct drw_interface;
struct iovec;
struct drw_operations {
	void*	(*rw_create_worker)(struct drw_interface *drw_p, char *, char, char);

	ssize_t	(*rw_pread)(struct drw_interface *, void *, void *, size_t, off_t);
	ssize_t	(*rw_pwrite)(struct drw_interface *, void *, void *, size_t, off_t);
	ssize_t (*rw_pwritev)(struct drw_interface *, void *, struct iovec *, int, off_t);

	ssize_t (*rw_pread_fp)(struct drw_interface *, void *, void *, size_t, off_t, struct list_head*);
	ssize_t (*rw_pwrite_fp)(struct drw_interface *, void *, void *, size_t, off_t, struct list_head*);
	ssize_t (*rw_pwritev_fp)(struct drw_interface *, void *, struct iovec *, int, off_t, struct list_head*);

	void	(*rw_sync)(struct drw_interface *, void *);

	void	(*rw_pwrite_async_fp)(struct drw_interface *, void *rw_handle, void *, size_t, off_t, rw_callback_fn, struct rw_callback_arg *);
	void	(*rw_pwritev_async_fp)(struct drw_interface *, void *, struct iovec *, int, off_t, rw_callback_fn, struct rw_callback_arg *);
	void	(*rw_pread_async_fp)(struct drw_interface *, void *rw_handle, void *, size_t, off_t, rw_callback_fn, struct rw_callback_arg *arg);

	uint8_t	(*rw_get_device_ready)(struct drw_interface *);
	void	(*rw_do_device_ready)(struct drw_interface *);

	int	(*rw_close)(struct drw_interface *, void *);
	int	(*rw_stop)(struct drw_interface *);
	void	(*rw_destroy)(struct drw_interface *);
	int	(*rw_trim)(void *, off_t, size_t);
};

struct drw_interface {
	void			*rw_private;
	struct drw_operations	*rw_op;
};

struct drw_setup {
	char			name[NID_MAX_DEVNAME];
	char			exportname[NID_MAX_PATH];
	int			simulate_async;
	int			simulate_delay;			// 0 for non-delay, 1 for read delay, 2 for write delay, 3 for read-write delay
	int			simulate_delay_min_gap;
	int			simulate_delay_max_gap;
	int			simulate_delay_time_us;
	struct fpn_interface 	*fpn_p;
	int			device_provision;
};

extern int drw_initialization(struct drw_interface *, struct drw_setup *);
#endif
