/*
 * dev_if.h
 * 	Interface of Device Module
 */
#ifndef NID_DEV_IF_H
#define NID_DEV_IF_H

#include <unistd.h>

#define DEV_TYPE_RG	1	// regular
#define DEV_TYPE_DA	2	// dynamic arranged
#define DEV_TYPE_MAX	3

struct dev_interface;
struct dev_operations {
	void*		(*d_create_worker)(struct dev_interface *, int, char *);
	int		(*d_open)(struct dev_interface *);
	ssize_t		(*d_pread)(struct dev_interface *,  void *, size_t, off_t);
	ssize_t		(*d_pwrite)(struct dev_interface *, const void *buf, size_t count, off_t offset);
};

struct dev_interface {
	void			*d_private;
	struct dev_operations	*d_op;
};

struct dev_setup {
	int	type;
	void	*real_dev;
};

extern int dev_initialization(struct dev_interface *, struct dev_setup *);
#endif
