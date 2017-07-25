/*
 * rio_if.h
 * 	Interface of Resource IO Module
 */
#ifndef NID_RIO_IF_H
#define NID_RIO_IF_H

#define RIO_MAX_CHANNELS	128

struct rio_interface;
struct rw_interface;
struct io_channel_info;
struct rio_operations {
	void*	(*r_create_channel)(struct rio_interface *, struct io_channel_info *, char *, int *);
	ssize_t	(*r_pread)(struct rio_interface *, void *, void *, size_t, off_t);
	ssize_t (*r_pwrite)(struct rio_interface *, void *, void *, size_t, off_t);
	int	(*r_trim)(struct rio_interface *, void *, off_t, size_t);
	int	(*r_close)(struct rio_interface *,  void *);
	int	(*r_stop)(struct rio_interface *);
	void	(*r_end_page)(struct rio_interface *,void *, void *page_p);
};

struct rio_interface {
	void			*r_private;
	struct rio_operations	*r_op;
};

struct pp_interface;
struct rio_setup {
	struct pp_interface	*pp;
};

extern int rio_initialization(struct rio_interface *, struct rio_setup *);
#endif
