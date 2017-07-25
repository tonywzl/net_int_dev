/*
 * rgdev.c
 * 	Implementation of Regular Device Module
 */

#include <stdlib.h>
#include <string.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "rgdev_if.h"


struct rgdev_private {
	char	p_dev_name[NID_MAX_UUID];
	int	p_fd;
};

static int
rgdev_open(struct rgdev_interface *rgdev_p)
{
	struct rgdev_private *priv_p = ( struct rgdev_private *)rgdev_p->d_private;
	return priv_p->p_fd;;
}

static ssize_t
rgdev_pread(struct rgdev_interface *rgdev_p, void *buf, size_t count, off_t offset)
{
	struct rgdev_private *priv_p = ( struct rgdev_private *)rgdev_p->d_private;
	int fd = priv_p->p_fd;
	return pread(fd, buf, count, offset);
}

static ssize_t
rgdev_pwrite(struct rgdev_interface *rgdev_p,const void *buf, size_t count, off_t offset)
{
	struct rgdev_private *priv_p = ( struct rgdev_private *)rgdev_p->d_private;
	int fd = priv_p->p_fd;
	return pwrite(fd, buf, count, offset);
}

static char *
rgdev_get_name(struct rgdev_interface *rgdev_p)
{
	struct rgdev_private *priv_p = ( struct rgdev_private *)rgdev_p->d_private;
	return priv_p->p_dev_name;
}

struct rgdev_operations rgdev_op = {
	.d_open = rgdev_open,
	.d_pread = rgdev_pread,
	.d_pwrite = rgdev_pwrite,
	.d_get_name = rgdev_get_name,
};

int
rgdev_initialization(struct rgdev_interface *rgdev_p, struct rgdev_setup *setup)
{
	char *log_header = "rgdev_initialization";
	struct rgdev_private *priv_p;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);
	rgdev_p->d_op = &rgdev_op;
	priv_p = calloc(1, sizeof(*priv_p));
	rgdev_p->d_private = priv_p;

	priv_p->p_fd = -1;
	strcpy(priv_p->p_dev_name, setup->dev_name);

	return 0;
}
