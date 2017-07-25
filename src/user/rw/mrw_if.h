/*
 * mrw_if.h
 * 	Interface of Meta Server Read Write Module
 */
#ifndef NID_MRW_IF_H
#define NID_MRW_IF_H

#include "rw_if.h"
#include "nid_shared.h"
#include <stdbool.h>

struct mrw_stat;
struct mrw_interface;
struct iovec;
struct mrw_operations {
	void*		(*rw_create_worker)(struct mrw_interface *mrw_p, char *, int);

	ssize_t		(*rw_pread)(struct mrw_interface *, void *, void *, size_t, off_t);
	ssize_t		(*rw_pwrite)(struct mrw_interface *, void *, void *, size_t, off_t);
	ssize_t 	(*rw_pwritev)(struct mrw_interface *, void *, struct iovec *, int, off_t);

	ssize_t 	(*rw_pread_fp)(struct mrw_interface *, void *, void *, size_t, off_t, struct list_head*);
	ssize_t 	(*rw_pwrite_fp)(struct mrw_interface *, void *, void *, size_t, off_t, struct list_head*);
	ssize_t 	(*rw_pwritev_fp)(struct mrw_interface *, void *, struct iovec *, int, off_t, struct list_head*);

	void		(*rw_pwrite_async_fp)(struct mrw_interface *, void *rw_handle, void *, size_t, off_t, rw_callback_fn, struct rw_callback_arg *arg);
	void		(*rw_pwritev_async_fp)(struct mrw_interface *, void *rw_handle, struct iovec *, int, off_t, rw_callback_fn, struct rw_callback_arg *arg);
	void		(*rw_pread_async_fp)(struct mrw_interface *, void *rw_handle, void *, size_t, off_t, rw_callback_fn, struct rw_callback_arg *arg);

	void		(*rw_fetch_fp)(struct mrw_interface *mrw_p, void *rw_handle, off_t voff, size_t len, rw_callback2_fn func, void *arg, void *fpout, bool *fpFound);
	void		(*rw_fetch_data)(struct iovec* vec, size_t count, void *fp, rw_callback2_fn func, void *arg);

	int		(*rw_close)(struct mrw_interface *, void *);
	int		(*rw_stop)(struct mrw_interface *);
	void		(*rw_get_status)(struct mrw_interface *, struct mrw_stat *);
	void		(*rw_ms_flush)(struct mrw_interface *);
};

struct mrw_stat {
	int sent_wop_num;
	int	sent_wfp_num;
};

struct mrw_interface {
	void			*rw_private;
	struct mrw_operations	*rw_op;
};

struct mrw_setup {
	char				name[NID_MAX_DEVNAME];
	struct allocator_interface	*allocator;
	struct fpn_interface 		*fpn_p;
};

extern int mrw_initialization(struct mrw_interface *, struct mrw_setup *);
#endif
