/*
 * rw_if.h
 * 	Interface of Read Write Module
 */
#ifndef NID_RW_IF_H
#define NID_RW_IF_H

#include <stdbool.h>
#include "node_if.h"

#define RW_TYPE_NONE	0
#define RW_TYPE_DEVICE	1
#define RW_TYPE_MSERVER	2
#define RW_TYPE_MAX	3

enum rw_cb_arg_type {
	RW_CB_TYPE_NORMAL,
	RW_CB_TYPE_ALIGNMENT
};

struct rw_callback_arg {
	struct node_header	ag_nd_header;
	struct list_head	ag_fp_head;		// For store returned finger print. The list node is 'struct fp_node'!!
	void			*ag_request;
	struct list_head	ag_coalesce_head; 	// list of pages which has a request coalesced by me
	struct list_head	ag_overwritten_head;	// list of requests which trying to overwrite me
	enum rw_cb_arg_type	ag_type;		// For identify type of the callback argument.
};

typedef void (*rw_callback_fn)(int errorcode, struct rw_callback_arg *arg);
typedef void (*rw_callback2_fn)(int errorcode, void *arg);

/**
 * If the write bytes is not alignment, but we do need alignment,
 * We have to package the origin write to a alignment write.
 */
struct rw_alignment_write_arg {
	struct rw_callback_arg   arg_this;
	rw_callback_fn           arg_callback_packaged;
	struct rw_callback_arg  *arg_packaged;
	void                    *arg_buffer_align;
	size_t                   arg_count_align;
	size_t                   arg_offset_align;
};

/**
 * If the read bytes is not alignment, but we do need alignment,
 * We have to package the origin read to a alignment read.
 */
struct rw_alignment_read_arg {
	struct rw_callback_arg   arg_this;
	rw_callback_fn           arg_callback_packaged;
	struct rw_callback_arg  *arg_packaged;
	void                    *arg_buffer_align;
	void                    *arg_buf;
	size_t                   arg_count;
	size_t                   arg_count_align;
	off_t                    arg_offset;
	off_t                    arg_offset_align;
};

struct rw_interface;
struct iovec;
struct rw_operations {
	void*	(*rw_create_worker)(struct rw_interface *, char *, char, char, int, int);
	int	(*rw_close)(struct rw_interface *, void *);
	int	(*rw_trim)(struct rw_interface *, void *, off_t, size_t);
	void	(*rw_destroy)(struct rw_interface *);

	ssize_t	(*rw_pread)(struct rw_interface *, void *, void *, size_t, off_t); 
	ssize_t	(*rw_pwrite)(struct rw_interface *, void *, void *, size_t, off_t);
	ssize_t	(*rw_pwritev)(struct rw_interface *, void *, struct iovec *, int, off_t);

	void    (*rw_pwrite_async)(struct rw_interface *, void *, void *, size_t, off_t, rw_callback_fn, struct rw_callback_arg *);
	void    (*rw_pwritev_async)(struct rw_interface *, void *, struct iovec *, int, off_t, rw_callback_fn, struct rw_callback_arg *);
	void    (*rw_pread_async)(struct rw_interface *, void *, void *, size_t, off_t, rw_callback_fn, struct rw_callback_arg *);

	ssize_t (*rw_pread_fp)(struct rw_interface *, void *, void *, size_t, off_t, struct list_head*, void **align_buf, size_t *count_align, off_t *offset_align);
	ssize_t (*rw_pwrite_fp)(struct rw_interface *, void *, void *, size_t, off_t, struct list_head*, void **align_buf, size_t *count_align, off_t *offset_align);
	ssize_t (*rw_pwritev_fp)(struct rw_interface *, void *, struct iovec *, int, off_t, struct list_head*, void **align_buf, size_t *count_align, off_t *offset_align);

	void    (*rw_pwrite_async_fp)(struct rw_interface *, void *, void *, size_t, off_t, rw_callback_fn, struct rw_callback_arg *);
	void    (*rw_pwritev_async_fp)(struct rw_interface *, void *, struct iovec *, int, off_t, rw_callback_fn, struct rw_callback_arg *);
	void    (*rw_pread_async_fp)(struct rw_interface *, void *, void *, size_t, off_t, rw_callback_fn, struct rw_callback_arg *);

	void	(*rw_sync)(struct rw_interface *, void *);

	void	(*rw_fetch_fp)(struct rw_interface *rw_p, void *rw_handle, off_t voff, size_t len, rw_callback2_fn func, void *arg, void *fpout, bool *fpFound);
	void	(*rw_fetch_data)(struct rw_interface *rw_p, struct iovec* vec, size_t count, void *fp, rw_callback2_fn func, void *arg);

	int	(*rw_get_type)(struct rw_interface *);
	char*	(*rw_get_name)(struct rw_interface *);
	char*	(*rw_get_exportname)(struct rw_interface *);
	void* 	(*rw_get_rw_obj)(struct rw_interface *);
	void	(*rw_ms_flush)(struct rw_interface *);
	struct fpn_interface* (*rw_get_fpn_p)(struct rw_interface *);

	uint8_t	(*rw_get_device_ready)(struct rw_interface *);
	void	(*rw_do_device_ready)(struct rw_interface *);
};

struct rw_interface {
	void			*rw_private;
	struct rw_operations	*rw_op;
};

struct rw_setup {
	char				*name;
	char				*exportname;
	struct allocator_interface	*allocator;
	struct fpn_interface 		*fpn_p;
	int				type;
	int				device_provision;
	int				simulate_async;
	int				simulate_delay;			// 0 for non-delay, 1 for read delay, 2 for write delay, 3 for read-write delay
	int				simulate_delay_min_gap;
	int				simulate_delay_max_gap;
	int				simulate_delay_time_us;
};

extern int rw_initialization(struct rw_interface *, struct rw_setup *);
#endif
