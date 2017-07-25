/*
 * util_bdev.c
 * 	Implementation common function for block device
 */
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nid_log.h"

int
util_bdev_getinfo(int fd, uint64_t *dev_size, uint32_t *sec_size, uint8_t *is_dev)
{
	char *log_header = "util_bdev_getinfo";
	int rc = 0, ret, l_errno;
	struct stat sbuf;

	*dev_size = 0;
	*sec_size = 0;

	fstat(fd, &sbuf);
	if(S_ISBLK(sbuf.st_mode)) {
		// real block device
		// get device size in bytes and sector size from target device
		ret = ioctl(fd, BLKGETSIZE64, dev_size);
		l_errno = errno;
		if (ret && l_errno != 25) {
			nid_log_error("%s: failed to call ioctl function with return code %d and error: %s",
							log_header, ret, strerror(l_errno));
			rc = 1;
			goto out;
		}

		ret = ioctl(fd, BLKSSZGET, sec_size);
		l_errno = errno;
		if (ret && l_errno != 25) {
			nid_log_error("%s: failed to call ioctl function with return code %d and error: %s",
							log_header, ret, strerror(l_errno));
			rc = 1;
			goto out;
		}
		*is_dev = 1;
	} else {
		// only a normal file
		*dev_size = sbuf.st_size;
		*sec_size = sbuf.st_blksize;
		*is_dev = 0;
	}

out:
	return rc;
}

int
util_bdev_trim(int fd, off_t offset, size_t len, uint64_t bdev_size, uint32_t bsec_size, uint8_t is_dev)
{
	char *log_header = "util_bdev_trim";
	int ret = -1;
	uint64_t end, range[2];
	char *dbuf = NULL;

	// TODO: remove it after trim can work
	return 0;

	// trim command can only used in real block device, skip file faked block device
	if(!bdev_size || !bsec_size) {
		return 0;
	}

	range[0] = offset;
	range[1] = len;
	assert(len > 0 && offset >= 0);

	// check offset alignment to the sector size/
	if (range[0] % bsec_size) {
		nid_log_error("%s: offset: %lu is not aligned to sector size %d",
				log_header, offset, bsec_size);
		goto out;
	}

	// check offset lie in the range of device
	if (range[0] > bdev_size) {
		nid_log_error("%s: offset: %lu is bigger than device size %lu",
				log_header, offset, bdev_size);
		goto out;
	}

	// adjust trim range when it not lies in the range of device
	end = range[0] + range[1];
	if (end > bdev_size)
		end = bdev_size;

	range[1] = end - range[0];

	if(is_dev) {
		// send trim command to block device
		ret = ioctl(fd, BLKDISCARD, &range);
	} else {
		ret = lseek(fd, range[0], SEEK_SET);
		if(ret != -1) {
			dbuf = (char *)calloc(1, range[1]);
			if(!dbuf) {
				nid_log_error("%s: failed in allocate memory", log_header);
				goto out;
			}
			ret = write(fd, dbuf, range[1]);
			if(!dbuf) {
				nid_log_error("%s: failed in fill zero", log_header);
				goto out;
			}
		}
	}
	if(ret) {
		nid_log_error("%s: failed to apply trim command on block device, with error: %s",
						log_header, strerror(errno));
	}

out:
	if(dbuf) {
		free(dbuf);
	}
	return ret;
}

