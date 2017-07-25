# $Id $
#ident "$Source $"

# $Copyright $

CC = gcc
KERNDIR := $(shell uname -r)
KERNDIR ?= "3.10.3"
 
SYMLINKS = nid_log.h nid_shared.h mpk_if.h \
	drv_if.h dev_if.h \
	ddg_if.h ddg.c

BASEDIR = ../../..

all:	$(SYMLINKS)
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) modules


nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

drv_if.h:
	ln -f -s ${BASEDIR}/drv_if.h drv_if.h

dev_if.h:
	ln -f -s ${BASEDIR}/dev_if.h dev_if.h

ddg_if.h:
	ln -f -s ../../../ddg_if.h ddg_if.h

ddg.c:
	ln -f -s ../../../ddg.c ddg.c

clean:
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) clean

realclean: clean
	rm -f $(SYMLINKS)
