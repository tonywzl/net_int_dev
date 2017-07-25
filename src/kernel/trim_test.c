#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdio.h>
#include <linux/fs.h>
#include <unistd.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <sys/uio.h>


int
util_bdev_getinfo(int fd, uint64_t *dev_size, uint32_t *sec_size)
{
	char *log_header = "util_bdev_getinfo";
	int rc = 0, ret, l_errno;

	*dev_size = 0;
	*sec_size = 0;

	// get device size in bytes and sector size from target device
	ret = ioctl(fd, BLKGETSIZE64, dev_size);
	l_errno = errno;
	if (ret && l_errno != 25) {
		printf("%s: failed to call ioctl function with return code %d and error: %s\n",
						log_header, ret, strerror(l_errno));
		rc = 1;
		goto out;
	}

	ret = ioctl(fd, BLKSSZGET, sec_size);
	l_errno = errno;
	if (ret && l_errno != 25) {
		printf("%s: failed to call ioctl function with return code %d and error: %s\n",
						log_header, ret, strerror(l_errno));
		rc = 1;
		goto out;
	}

out:
	return rc;
}

int
util_bdev_trim(int fd, off_t offset, size_t len, uint64_t bdev_size, uint32_t bsec_size)
{
	char *log_header = "util_bdev_trim";
	int ret = -1;
	uint64_t end, range[2];

	// trim command can only used in real block device, skip file faked block device
	if(!bdev_size || !bsec_size) {
		return 0;
	}

	range[0] = offset;
	range[1] = len;
	assert(len > 0 && offset >= 0);

	// check offset alignment to the sector size/
	if (range[0] % bsec_size) {
		printf("%s: offset: %lu is not aligned to sector size %d\n",
				log_header, offset, bsec_size);
		goto out;
	}

	// check offset lie in the range of device
	if (range[0] > bdev_size) {
		printf("%s: offset: %lu is bigger than device size %lu\n",
				log_header, offset, bdev_size);
		goto out;
	}

	// adjust trim range when it not lies in the range of device
	end = range[0] + range[1];
	if (end > bdev_size)
		end = bdev_size;

	range[1] = end - range[0];

	// send trim command to block device
	ret = ioctl(fd, BLKDISCARD, &range);
	if(ret) {
		printf("%s: failed to apply trim command on block device, with error: %s\n",
						log_header, strerror(errno));
	}

out:
	return ret;
}

int main() {
	uint64_t dev_size;
	uint32_t sec_size;
	int rc = -1;

	int fd = open("/dev/nid2", O_RDWR | O_SYNC, 0x600);
	if(fd < 0) {
		printf("test failed\n");
		goto out;
	}
//	util_bdev_trim(fd, 0, 16*1024, 4096, 1024*1024);
//til_bdev_getinfo();
/*	if(ioctl(fd, BLKGETSIZE64, &dev_size)) {
		printf("failed to get bdev size\n");
		goto out;
	}
	printf("get bdev size %lu\n", dev_size);*/
	if (ioctl(fd, BLKSSZGET, &sec_size)) {
		printf("failed to get bdev sector size\n");
		goto out;
	}
	printf("get bdev sector size %u\n", sec_size);
	rc = 0;

out:

	close(fd);
	printf("test done\n"); 
	return rc;
}
