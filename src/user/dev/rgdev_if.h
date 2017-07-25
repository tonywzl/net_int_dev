/*
 * rgdev_if.h
 * 	Interface of Regular Device Module
 */
#ifndef NID_RGDEV_IF_H
#define NID_RGDEV_IF_H

#include <unistd.h>
#include "nid_shared.h"

struct rgdev_interface;
struct rgdev_operations {
	int		(*d_open)(struct rgdev_interface *);
	ssize_t		(*d_pread)(struct rgdev_interface *, void *, size_t, off_t);
	ssize_t		(*d_pwrite)(struct rgdev_interface *, const void *buf, size_t count, off_t offset);
	char*		(*d_get_name)(struct rgdev_interface *);
};

struct rgdev_interface {
	void			*d_private;
	struct rgdev_operations	*d_op;
};

struct rgdev_setup {
	int 	dev_type;
	char 	dev_name[NID_MAX_UUID];
};

extern int rgdev_initialization(struct rgdev_interface *, struct rgdev_setup *);
#endif
