# $Id $
#ident "$Source $"

# $Copyright $

CC = gcc
KERNDIR := $(shell uname -r)
KERNDIR ?= "3.10.3"
 
SYMLINKS = nid_log.h nid.h nid_shared.h mpk_if.h \
	mq_if.h nl_if.h ddg_if.h \
	dev_if.h dev.c

BASEDIR = ../../..

all:	$(SYMLINKS)
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) modules

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid.h:
	ln -f -s ../../../nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

mq_if.h:
	ln -f -s ../../../mq_if.h mq_if.h

nl_if.h:
	ln -f -s ../../../nl_if.h nl_if.h

ddg_if.h:
	ln -f -s ../../../ddg_if.h ddg_if.h

dev_if.h:
	ln -f -s ../../../dev_if.h dev_if.h

dev.c:
	ln -f -s ../../../dev.c dev.c

clean:
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) clean

realclean: clean
	rm -f $(SYMLINKS)
