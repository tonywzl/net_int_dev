DEV_SRC = devg.c rgdevg.c dev.c rgdev.c

DEV_SYMLINKS = dev_if.h dev.c devg_if.h devg.c \
	rgdev_if.h rgdev.c rgdevg_if.h rgdevg.c \

dev_if.h:
	ln -f -s $(DEV_BASEDIR)/dev_if.h dev_if.h

dev.c:
	ln -f -s $(DEV_BASEDIR)/dev.c dev.c

devg_if.h:
	ln -f -s $(DEV_BASEDIR)/devg_if.h devg_if.h

devg.c:
	ln -f -s $(DEV_BASEDIR)/devg.c devg.c

rgdev_if.h:
	ln -f -s $(DEV_BASEDIR)/rgdev_if.h rgdev_if.h

rgdev.c:
	ln -f -s $(DEV_BASEDIR)/rgdev.c rgdev.c

rgdevg_if.h:
	ln -f -s $(DEV_BASEDIR)/rgdevg_if.h rgdevg_if.h

rgdevg.c:
	ln -f -s $(DEV_BASEDIR)/rgdevg.c rgdevg.c

