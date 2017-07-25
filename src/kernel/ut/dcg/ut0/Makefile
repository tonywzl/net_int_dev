# $Id $
#ident "$Source $"

# $Copyright $

CC = gcc
KERNDIR := $(shell uname -r)
KERNDIR ?= "3.10.3"
 
SYMLINKS = nid_log.h nid_shared.h mpk_if.h \
	chan_if.h drv_if.h \
	dcg_if.h dcg.c

BASEDIR = ../../..

all:	$(SYMLINKS)
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) modules

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

chan_if.h:
	ln -f -s ../../../chan_if.h chan_if.h

drv_if.h:
	ln -f -s ../../../drv_if.h drv_if.h

dcg_if.h:
	ln -f -s ../../../dcg_if.h dcg_if.h

dcg.c:
	ln -f -s ../../../dcg.c dcg.c

clean:
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) clean

realclean: clean
	rm -f $(SYMLINKS)
