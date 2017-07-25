# $Id $
#ident "$Source $"

# $Copyright $

CC = gcc
KERNDIR := $(shell uname -r)
KERNDIR ?= "/lib/modules/$(KERN_VERSION)/build" 
 
SYMLINKS = version.h nid_log.h nid.h nid_shared.h mpk_if.h \
	mq_if.h nl_if.h dat_if.h dev_if.h drv_if.h \
	nid_mod.c

BASEDIR = ../../..

all:	$(SYMLINKS)
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) modules

version.h:
	ln -s ../../../../../build/version.h version.h

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid.h:
	ln -f -s ../../../nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

dat_if.h:
	ln -f -s ../../../dat_if.h dat_if.h

mq_if.h:
	ln -f -s ../../../mq_if.h mq_if.h

nl_if.h:
	ln -f -s ../../../nl_if.h nl_if.h

dev_if.h:
	ln -f -s ../../../dev_if.h dev_if.h

drv_if.h:
	ln -f -s ../../../drv_if.h drv_if.h

nid_mod.c:
	ln -f -s ../../../nid_mod.c nid_mod.c

clean:
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) clean

realclean: clean
	rm -f $(SYMLINKS)
