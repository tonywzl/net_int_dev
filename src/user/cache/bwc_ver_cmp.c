/*
 * bwc_ver_cmp.c
 * 	Executable that checks the write cache compatibility of the version of current binary
 * 	and that of the write cache of the given path.
 */

#include "bwc.h"

static void
usage()
{
	printf("usage: bwc_ver_cmp [write cache path]\n");
}

int
main(int argc, char* argv[])
{
	char *path;
	int fd = -1;
	struct bwc_mdz_header *mhp = NULL;
	ssize_t len;
	unsigned char checksum[16];
	int ret = 0;

	if (argc != 2) {
		usage();
		ret = 1;
		goto out;
	}

	path = argv[1];
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		printf("Failed to open %s, errno:%d\n", path, errno);
		ret = 1;
		goto out;
	}

	mhp = calloc(1, sizeof(*mhp));
	if (!mhp) {
		printf("Failed to allcate memory\n");
		ret = 1;
		goto out;
	}

	len = read(fd, mhp, sizeof(*mhp));
	if (len != sizeof(*mhp)) {
		printf("Failed to read, len:%ld, errno:%d\n", len, errno);
		ret = 1;
		goto out;
	}

	MD5(( void *)mhp->md_magic_ver, sizeof(*mhp) - sizeof(mhp->md_checksum), checksum);
	if (memcmp(mhp->md_checksum, checksum, 16)) {
		/* Checksum has been zeroed by dd */
		printf("Magic destroyed\n");
		goto out;
	}

	if (__bwc_ver_cmp_check(mhp->md_magic_ver, BWC_DEV_MDZ_VER)) {
		printf("Version not compatible\n");
		goto out;
	} else {
		printf("Version compatible\n");
	}

out:
	if (fd >= 0)
		close(fd);
	if (mhp)
		free(mhp);
	return ret;
}
