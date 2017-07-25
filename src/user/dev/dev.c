/*
 * dev.c
 * 	Implementation of  dev Module
 */

#include <stdlib.h>

#include "list.h"
#include "nid_log.h"
#include "dev_if.h"
#include "rgdev_if.h"


struct dev_private {
	int	p_dev_type;
	void	*p_real_device;
};

static int 
dev_open(struct dev_interface *dev_p)
{
	char *log_header = "dev_open";
	struct dev_private *priv_p = (struct dev_private *)dev_p->d_private;
	struct rgdev_interface *rgdev_p;
	ssize_t rc;

	switch(priv_p->p_dev_type) {
	case DEV_TYPE_RG:
		rgdev_p = (struct rgdev_interface *) priv_p->p_real_device;
		rc = rgdev_p->d_op->d_open(rgdev_p);
		break;

	case DEV_TYPE_DA:
		break;

	default:
		nid_log_error("%s: wrong dev_type", log_header);
	}

	return rc;
}

static ssize_t
dev_pread(struct dev_interface *dev_p,  void *buf, size_t count, off_t offset)
{
	char *log_header = "dev_pread";
	struct dev_private *priv_p = (struct dev_private *)dev_p->d_private;
	struct rgdev_interface *rgdev_p;
	ssize_t rc;

	switch(priv_p->p_dev_type) {
	case DEV_TYPE_RG:
		rgdev_p = (struct rgdev_interface *) priv_p->p_real_device;
		rc = rgdev_p->d_op->d_pread(rgdev_p, buf, count, offset);
		break;

	case DEV_TYPE_DA:
		break;

	default:
		nid_log_error("%s: wrong dev_type", log_header);
	}
	return rc;
}

static ssize_t
dev_pwrite(struct dev_interface *dev_p, const void *buf, size_t count, off_t offset)
{
	char *log_header = "dev_pwrite";
	struct dev_private *priv_p = (struct dev_private *)dev_p->d_private;
	struct rgdev_interface *rgdev_p;
	ssize_t rc;

	switch(priv_p->p_dev_type) {
	case DEV_TYPE_RG:
		rgdev_p = (struct rgdev_interface *)priv_p->p_real_device;
		rc = rgdev_p->d_op->d_pwrite(rgdev_p, buf, count, offset);
		break;

	case DEV_TYPE_DA:
		break;

	default:
		nid_log_error("%s: wrong dev_type", log_header);
	}
	return rc;
}

struct dev_operations dev_op = {
	.d_open = dev_open,
	.d_pread = dev_pread,
	.d_pwrite = dev_pwrite,
};

int
dev_initialization(struct dev_interface *dev_p, struct dev_setup *setup)
{
	char *log_header = "dev_initialization";
	struct dev_private *priv_p;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);
	dev_p->d_op = &dev_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dev_p->d_private = priv_p;

	priv_p->p_dev_type = setup->type;
	priv_p->p_real_device = setup->real_dev;

	return 0;
}
